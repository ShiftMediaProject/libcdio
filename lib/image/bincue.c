/*
    $Id: bincue.c,v 1.7 2004/03/20 21:54:38 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002, 2003, 2004 Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* This code implements low-level access functions for a CD images
   residing inside a disk file (*.bin) and its associated cue sheet.
   (*.cue).
*/

static const char _rcsid[] = "$Id: bincue.c,v 1.7 2004/03/20 21:54:38 rocky Exp $";

#include "cdio_assert.h"
#include "cdio_private.h"
#include "_cdio_stdio.h"

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif
#include <ctype.h>

/* FIXME: should put in a common definition somewhere. */
#ifdef HAVE_MEMSET
#define BZERO(ptr, size) memset(ptr, 0, size)
#elif  HAVE_BZERO
#define BZERO(ptr, size) bzero(ptr, size)
#else 
  Error -- you need either memset or bzero
#endif

/* reader */

#define DEFAULT_CDIO_DEVICE "videocd.bin"
#define DEFAULT_CDIO_CUE    "videocd.cue"

typedef struct {
  track_t        track_num;   /* Probably is index+1 */
  msf_t          start_msf;
  lba_t          start_lba;
  int            start_index;
  int            sec_count;  /* Number of sectors in this track. Does not
				include pregap */
  int            num_indices;
  int            flags;      /* "DCP", "4CH", "PRE" */
  track_format_t track_format;
  bool           track_green;
  uint16_t  datasize;        /* How much is in the portion we return back? */
  uint16_t  datastart;       /* Offset from begining that data starts */
  uint16_t  endsize;         /* How much stuff at the end to skip over. This
			       stuff may have error correction (EDC, or ECC).*/
  uint16_t  blocksize;       /* total block size = start + size + end */

  
} track_info_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  internal_position_t pos; 
  
  bool sector_2336;              /* Playstation (PSX) uses 2336-byte sectors */

  char         *cue_name;
  char         *mcn;             /* Media catalog number. */
  track_info_t  tocent[100];     /* entry info for each track */
  track_t       total_tracks;    /* number of tracks in image */
  track_t       first_track_num; /* track number of first track */
  bool have_cue;
} _img_private_t;

static bool     _cdio_image_read_cue (_img_private_t *_obj);
static uint32_t _cdio_stat_size (void *env);

/*!
  Initialize image structures.
 */
static bool
_cdio_init (_img_private_t *_obj)
{
  lsn_t lead_lsn;

  if (_obj->gen.init)
    return false;

  if (!(_obj->gen.data_source = cdio_stdio_new (_obj->gen.source_name))) {
    cdio_warn ("init failed");
    return false;
  }

  /* Have to set init before calling _cdio_stat_size() or we will
     get into infinite recursion calling passing right here.
   */
  _obj->gen.init = true;  

  lead_lsn = _cdio_stat_size( (_img_private_t *) _obj);

  if (-1 == lead_lsn) 
    return false;

  /* Read in CUE sheet. */
  if ((_obj->cue_name != NULL)) {
    _obj->have_cue = _cdio_image_read_cue(_obj);
  }

  if (!_obj->have_cue ) {
    /* Time to fake things...
       Make one big track, track 0 and 1 are the same. 
       We are guessing stuff starts at msf 00:04:00 - 2 for the 150
       sector pregap and 2 for the cue information.
     */
    track_info_t  *this_track=&(_obj->tocent[0]);
    int blocksize = _obj->sector_2336 
      ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

    _obj->total_tracks = 2;
    _obj->first_track_num = 1;
    this_track->start_msf.m = to_bcd8(0);
    this_track->start_msf.s = to_bcd8(4);
    this_track->start_msf.f = to_bcd8(0);
    this_track->start_lba   = cdio_msf_to_lba(&this_track->start_msf);
    this_track->blocksize   = blocksize;
    this_track->track_format= TRACK_FORMAT_XA;
    this_track->track_green = true;


    _obj->tocent[1] = _obj->tocent[0];
  }
  
  /* Fake out leadout track and sector count for last track*/
  cdio_lsn_to_msf (lead_lsn, &_obj->tocent[_obj->total_tracks].start_msf);
  _obj->tocent[_obj->total_tracks].start_lba = cdio_lsn_to_lba(lead_lsn);
  _obj->tocent[_obj->total_tracks-1].sec_count = 
    cdio_lsn_to_lba(lead_lsn - _obj->tocent[_obj->total_tracks-1].start_lba);

  return true;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Would be libc's seek() but we have to adjust for the extra track header 
  information in each sector.
*/
static off_t
_cdio_lseek (void *env, off_t offset, int whence)
{
  _img_private_t *_obj = env;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. I'm guessing
     the 1st 24 bytes of a bin file are used for something.
  */
  off_t real_offset=0;

  unsigned int i;

  _obj->pos.lba = 0;
  for (i=0; i<_obj->total_tracks; i++) {
    track_info_t  *this_track=&(_obj->tocent[i]);
    _obj->pos.index = i;
    if ( (this_track->sec_count*this_track->datasize) >= offset) {
      int blocks            = offset / this_track->datasize;
      int rem               = offset % this_track->datasize;
      int block_offset      = blocks * this_track->blocksize;
      real_offset          += block_offset + rem;
      _obj->pos.buff_offset = rem;
      _obj->pos.lba        += blocks;
      break;
    }
    real_offset   += this_track->sec_count*this_track->blocksize;
    offset        -= this_track->sec_count*this_track->datasize;
    _obj->pos.lba += this_track->sec_count;
  }

  if (i==_obj->total_tracks) {
    cdio_warn ("seeking outside range of disk image");
    return -1;
  } else {
    real_offset += _obj->tocent[i].datastart;
    return cdio_stream_seek(_obj->gen.data_source, real_offset, whence);
  }
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  FIXME: 
   At present we assume a read doesn't cross sector or track
   boundaries.
*/
static ssize_t
_cdio_read (void *env, void *data, size_t size)
{
  _img_private_t *_obj = env;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  char *p = data;
  ssize_t final_size=0;
  ssize_t this_size;
  track_info_t  *this_track=&(_obj->tocent[_obj->pos.index]);
  ssize_t skip_size = this_track->datastart + this_track->endsize;

  while (size > 0) {
    int rem = this_track->datasize - _obj->pos.buff_offset;
    if (size <= rem) {
      this_size = cdio_stream_read(_obj->gen.data_source, buf, size, 1);
      final_size += this_size;
      memcpy (p, buf, this_size);
      break;
    }

    /* Finish off reading this sector. */
    cdio_warn ("Reading across block boundaries not finished");

    size -= rem;
    this_size = cdio_stream_read(_obj->gen.data_source, buf, rem, 1);
    final_size += this_size;
    memcpy (p, buf, this_size);
    p += this_size;
    this_size = cdio_stream_read(_obj->gen.data_source, buf, rem, 1);
    
    /* Skip over stuff at end of this sector and the beginning of the next.
     */
    cdio_stream_read(_obj->gen.data_source, buf, skip_size, 1);

    /* Get ready to read another sector. */
    _obj->pos.buff_offset=0;
    _obj->pos.lba++;

    /* Have gone into next track. */
    if (_obj->pos.lba >= _obj->tocent[_obj->pos.index+1].start_lba) {
      _obj->pos.index++;
      this_track=&(_obj->tocent[_obj->pos.index]);
      skip_size = this_track->datastart + this_track->endsize;
    }
  }
  return final_size;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_cdio_stat_size (void *env)
{
  _img_private_t *_obj = env;
  long size;
  int blocksize = _obj->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  _cdio_init (_obj);
  
  size = cdio_stream_stat (_obj->gen.data_source);

  if (size % blocksize)
    {
      cdio_warn ("image %s size (%ld) not multiple of blocksize (%d)", 
		 _obj->gen.source_name, size, blocksize);
      if (size % M2RAW_SECTOR_SIZE == 0)
	cdio_warn ("this may be a 2336-type disc image");
      else if (size % CDIO_CD_FRAMESIZE_RAW == 0)
	cdio_warn ("this may be a 2352-type disc image");
      /* exit (EXIT_FAILURE); */
    }

  size /= blocksize;

  return size;
}

#define MAXLINE 512

static bool
_cdio_image_read_cue (_img_private_t *_obj)
{
  FILE *fp;
  char line[MAXLINE];

  int track_num;
  int min,sec,frame;
  int blocksize;
  int start_index;
  bool seen_first_index_for_track=false;

  if ( _obj == NULL ||  _obj->cue_name == NULL ) return false;

  fp = fopen (_obj->cue_name, "r");
  if (fp == NULL) return false;

  _obj->total_tracks=0;
  _obj->first_track_num=1;
  _obj->mcn=NULL;
  
  while ((fgets(line, MAXLINE, fp)) != NULL) {
    char s[80];
    char *p;
    /*printf("Retrieved line of length %zu :\n", read);
      printf("%s", line); */
    for (p=line; isspace(*p); p++) ;
    if (1==sscanf(p, "FILE \"%80s[^\"]", s)) {
      /* Should expand file name based on cue file basename.
      free(_obj->bin_file);
      _obj->bin_file = strdup(s);
      */
      /* printf("Found file name %s\n", s); */
    } else if (1==sscanf(p, "CATALOG %80s", s)) {
      _obj->mcn = strdup(s);
    } else if (2==sscanf(p, "TRACK %d MODE2/%d", &track_num, &blocksize)) {
      track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
      this_track->track_num   = track_num;
      this_track->num_indices = 0;
      this_track->track_format= TRACK_FORMAT_XA;
      this_track->track_green = true;
      _obj->total_tracks++;
      seen_first_index_for_track=false;
      /*printf("Added track %d with blocksize %d\n", track_num, blocksize);*/

      this_track->blocksize   = blocksize;
      switch(blocksize) {
      case 2336:
	this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE;
	this_track->datasize  = M2RAW_SECTOR_SIZE;  
	this_track->endsize   = 0;
	break;
      default:
	cdio_warn ("Unknown MODE2 size %d. Assuming 2352", blocksize);
      case 2352:
	if (_obj->sector_2336) {
	  this_track->datastart = 0;          
	  this_track->datasize  = M2RAW_SECTOR_SIZE;
	  this_track->endsize   = blocksize - 2336;
	} else {
	  this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE +
	    CDIO_CD_SUBHEADER_SIZE;
	  this_track->datasize  = CDIO_CD_FRAMESIZE;
	  this_track->endsize   = CDIO_CD_SYNC_SIZE + CDIO_CD_ECC_SIZE;
	}
	break;
      }

    } else if (2==sscanf(p, "TRACK %d MODE1/%d", &track_num, &blocksize)) {
      track_info_t *this_track=&(_obj->tocent[_obj->total_tracks]);
      this_track->blocksize   = blocksize;
      switch(blocksize) {
      case 2048:
	/* Is the below correct? */
	this_track->datastart = 0;         
	this_track->datasize  = CDIO_CD_FRAMESIZE;
	this_track->endsize   = 0;  
	break;
      default:
	cdio_warn ("Unknown MODE1 size %d. Assuming 2352", blocksize);
      case 2352:
	this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE;
	this_track->datasize  = CDIO_CD_FRAMESIZE; 
	this_track->endsize   = CDIO_CD_EDC_SIZE + CDIO_CD_M1F1_ZERO_SIZE 
	  + CDIO_CD_ECC_SIZE;
      }

      this_track->track_num      = track_num;
      this_track->num_indices    = 0;
      this_track->track_format   = TRACK_FORMAT_DATA;
      this_track->track_green    = false;
      _obj->total_tracks++;
      seen_first_index_for_track=false;
      /*printf("Added track %d with blocksize %d\n", track_num, blocksize);*/

    } else if (1==sscanf(p, "TRACK %d AUDIO", &track_num)) {
      track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
      this_track->blocksize      = CDIO_CD_FRAMESIZE_RAW;
      this_track->datasize       = CDIO_CD_FRAMESIZE_RAW;
      this_track->datastart      = 0;
      this_track->endsize        = 0;
      this_track->track_num      = track_num;
      this_track->num_indices    = 0;
      this_track->track_format   = TRACK_FORMAT_AUDIO;
      this_track->track_green    = false;
      _obj->total_tracks++;
      seen_first_index_for_track=false;

    } else if (4==sscanf(p, "INDEX %d %d:%d:%d", 
			 &start_index, &min, &sec, &frame)) {
      track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks-1]);
      /* FIXME! all of this is a big hack. 
	 If start_index == 0, then this is the "last_cue" information.
	 The +2 below seconds is to adjust for the 150 pregap.
      */
      if (start_index != 0) {
	if (!seen_first_index_for_track) {
	  this_track->start_index = start_index;
	  sec += 2;
	  if (sec >= 60) {
	    min++;
	    sec -= 60;
	  }
	  this_track->start_msf.m = to_bcd8 (min);
	  this_track->start_msf.s = to_bcd8 (sec);
	  this_track->start_msf.f = to_bcd8 (frame);
	  this_track->start_lba   = cdio_msf_to_lba(&this_track->start_msf);
	  seen_first_index_for_track=true;
	}

	if (_obj->total_tracks > 1) {
	  /* Figure out number of sectors for previous track */
	  track_info_t  *prev_track=&(_obj->tocent[_obj->total_tracks-2]);
	  if ( this_track->start_lba < prev_track->start_lba ) {
	    cdio_warn("track %d at LBA %lu starts before track %d at LBA %lu", 
		     _obj->total_tracks,   
		      (unsigned long int) this_track->start_lba, 
		      _obj->total_tracks-1, 
		      (unsigned long int) prev_track->start_lba);
	    prev_track->sec_count = 0;
	  } else if ( this_track->start_lba >= prev_track->start_lba 
		      + CDIO_PREGAP_SECTORS ) {
	    prev_track->sec_count = this_track->start_lba - 
	      prev_track->start_lba - CDIO_PREGAP_SECTORS ;
	  } else {
	    cdio_warn ("%lu fewer than pregap (%d) sectors in track %d", 
		       (long unsigned int) 
		         this_track->start_lba - prev_track->start_lba,
		       CDIO_PREGAP_SECTORS,
		       _obj->total_tracks-1);
	    /* Include pregap portion in sec_count. Maybe the pregap
	     was omitted. */
	    prev_track->sec_count = this_track->start_lba - 
	      prev_track->start_lba;
	  }
	}
	this_track->num_indices++;
      }
    }
  }
  _obj->have_cue = _obj->total_tracks != 0;

  fclose (fp);
  return true;
}

/*!
   Reads a single audio sector from CD device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_audio_sectors (void *env, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *_obj = env;
  int ret;

  _cdio_init (_obj);

  /* Why the adjustment of 272, I don't know. It seems to work though */
  if (lsn != 0) {
    ret = cdio_stream_seek (_obj->gen.data_source, 
			    (lsn * CDIO_CD_FRAMESIZE_RAW) - 272, SEEK_SET);
    if (ret!=0) return ret;

    ret = cdio_stream_read (_obj->gen.data_source, data, 
			    CDIO_CD_FRAMESIZE_RAW, nblocks);
  } else {
    /* We need to pad out the first 272 bytes with 0's */
    BZERO(data, 272);
    
    ret = cdio_stream_seek (_obj->gen.data_source, 0, SEEK_SET);

    if (ret!=0) return ret;

    ret = cdio_stream_read (_obj->gen.data_source, (uint8_t *) data+272, 
			    CDIO_CD_FRAMESIZE_RAW - 272, nblocks);
  }

  /* ret is number of bytes if okay, but we need to return 0 okay. */
  return ret == 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sector (void *env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *_obj = env;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int blocksize = _obj->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  _cdio_init (_obj);

  ret = cdio_stream_seek (_obj->gen.data_source, lsn * blocksize, SEEK_SET);
  if (ret!=0) return ret;

  /* FIXME: Not completely sure the below is correct. */
  ret = cdio_stream_read (_obj->gen.data_source,
			  _obj->sector_2336 
			  ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE) 
			  : buf,
			  blocksize, 1);
  if (ret==0) return ret;

  memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);

  return 0;
}

/*!
   Reads nblocks of mode1 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sectors (void *env, void *data, uint32_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *_obj = env;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _cdio_read_mode1_sector (_obj, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sector (void *env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *_obj = env;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

  /* NOTE: The logic below seems a bit wrong and convoluted
     to me, but passes the regression tests. (Perhaps it is why we get
     valgrind errors in vcdxrip). Leave it the way it was for now.
     Review this sector 2336 stuff later.
  */

  int blocksize = _obj->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  _cdio_init (_obj);

  ret = cdio_stream_seek (_obj->gen.data_source, lsn * blocksize, SEEK_SET);
  if (ret!=0) return ret;

  ret = cdio_stream_read (_obj->gen.data_source,
			  _obj->sector_2336 
			  ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE) 
			  : buf,
			  blocksize, 1);
  if (ret==0) return ret;


  /* See NOTE above. */
  if (b_form2)
    memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	    M2RAW_SECTOR_SIZE);
  else
    memcpy (data, buf + CDIO_CD_XA_SYNC_HEADER, CDIO_CD_FRAMESIZE);

  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sectors (void *env, void *data, uint32_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *_obj = env;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _cdio_read_mode2_sector (_obj, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

#define free_if_notnull(obj) \
  if (NULL != obj) { free(obj); obj=NULL; };

static void 
_cdio_bincue_destroy (void *obj) 
{
  _img_private_t *env = obj;

  if (NULL == env) return;
  free_if_notnull(env->mcn);
  free_if_notnull(env->cue_name);
  cdio_generic_stdio_free(env);
  free(env);
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" to set the source device in I/O operations 
  is the only valid key.

  0 is returned if no error was found, and nonzero if there as an error.
*/
static int
_cdio_set_arg (void *env, const char key[], const char value[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source"))
    {
      free_if_notnull (_obj->gen.source_name);

      if (!value)
	return -2;

      _obj->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "sector"))
    {
      if (!strcmp (value, "2336"))
	_obj->sector_2336 = true;
      else if (!strcmp (value, "2352"))
	_obj->sector_2336 = false;
      else
	return -2;
    }
  else if (!strcmp (key, "cue"))
    {
      free_if_notnull (_obj->cue_name);

      if (!value)
	return -2;

      _obj->cue_name = strdup (value);
    }
  else
    return -1;

  return 0;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_cdio_get_arg (void *env, const char key[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source")) {
    return _obj->gen.source_name;
  } else if (!strcmp (key, "cue")) {
    return _obj->cue_name;
  } 
  return NULL;
}

/*!
  Return an array of strings giving possible NRG disk images.
 */
char **
cdio_get_devices_bincue (void)
{
  char **drives = NULL;
  unsigned int num_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  glob("*.cue", GLOB_DOOFFS, NULL, &globbuf);
  for (i=0; i<globbuf.gl_pathc; i++) {
    cdio_add_device_list(&drives, globbuf.gl_pathv[i], &num_files);
  }
  globfree(&globbuf);
#else
  cdio_add_device_list(&drives, DEFAULT_CDIO_DEVICE, &num_files);
#endif /*HAVE_GLOB_H*/
  cdio_add_device_list(&drives, NULL, &num_files);
  return drives;
}

/*!
  Return a string containing the default CD device.
 */
char *
cdio_get_default_device_bincue(void)
{
  char **drives = cdio_get_devices_nrg();
  char *drive = (drives[0] == NULL) ? NULL : strdup(drives[0]);
  cdio_free_device_list(&drives);
  return drive;
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *env) 
{
  _img_private_t *_obj = env;
  
  _cdio_init (_obj);

  return _obj->first_track_num;
}

/*!
  Return the media catalog number (MCN) from the CD or NULL if there
  is none or we don't have the ability to get it.

  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  */
static char *
_cdio_get_mcn(void *env)
{
  _img_private_t *_obj = env;
  
  _cdio_init (_obj);

  if (NULL == _obj->mcn) return NULL;
  return strdup(_obj->mcn);
}

/*! 
  Return the number of tracks in the current medium.
  If no cuesheet is available, We fake it an just say there's
  one big track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_num_tracks(void *env) 
{
  _img_private_t *_obj = env;
  _cdio_init (_obj);

  return _obj->have_cue && _obj->total_tracks > 0 ? _obj->total_tracks : 1;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
_cdio_get_track_format(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.init) _cdio_init(_obj);

  if (track_num > _obj->total_tracks || track_num == 0) 
    return TRACK_FORMAT_ERROR;

  return _obj->tocent[track_num-1].track_format;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_cdio_get_track_green(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.init) _cdio_init(_obj);

  if (track_num > _obj->total_tracks || track_num == 0) 
    return false;

  return _obj->tocent[track_num-1].track_green;
}

/*!  
  Return the starting LSN track number
  track_num in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lba_t
_cdio_get_track_lba(void *env, track_t track_num)
{
  _img_private_t *_obj = env;
  _cdio_init (_obj);

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

  if (track_num <= _obj->total_tracks+1 && track_num != 0) {
    return _obj->tocent[track_num-1].start_lba;
  } else 
    return CDIO_INVALID_LBA;
}

/*!  
  Return the starting MSF (minutes/secs/frames) for the track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.

*/
static bool
_cdio_get_track_msf(void *env, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = env;
  _cdio_init (_obj);

  if (NULL == msf) return false;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

  if (track_num <= _obj->total_tracks+1 && track_num != 0) {
    *msf = _obj->tocent[track_num-1].start_msf;
    return true;
  } else 
    return false;
}

/*! 
  Return corresponding BIN file if cue_name is a cue file or NULL
  if not a CUE file.

*/
/* Later we'll probably parse the entire file. For now though, this gets us 
   started for now.
*/
char *
cdio_is_cuefile(const char *cue_name) 
{
  int   i;
  char *bin_name;
  
  if (cue_name == NULL) return false;

  bin_name=strdup(cue_name);
  i=strlen(bin_name)-strlen("cue");
  
  if (i>0) {
    if (cue_name[i]=='c' && cue_name[i+1]=='u' && cue_name[i+2]=='e') {
      bin_name[i++]='b'; bin_name[i++]='i'; bin_name[i++]='n';
      return bin_name;
    } 
    else if (cue_name[i]=='C' && cue_name[i+1]=='U' && cue_name[i+2]=='E') {
      bin_name[i++]='B'; bin_name[i++]='I'; bin_name[i++]='N';
      return bin_name;
    }
  }
  free(bin_name);
  return NULL;
}

/*! 
  Return corresponding CUE file if bin_name is a bin file or NULL
  if not a BIN file.

*/
/* Later we'll probably do better. For now though, this gets us 
   started for now.
*/
char *
cdio_is_binfile(const char *bin_name) 
{
  int   i;
  char *cue_name;
  
  if (bin_name == NULL) return false;

  cue_name=strdup(bin_name);
  i=strlen(bin_name)-strlen("bin");
  
  if (i>0) {
    if (bin_name[i]=='b' && bin_name[i+1]=='i' && bin_name[i+2]=='n') {
      cue_name[i++]='c'; cue_name[i++]='u'; cue_name[i++]='e';
      return cue_name;
    } 
    else if (bin_name[i]=='B' && bin_name[i+1]=='I' && bin_name[i+2]=='N') {
      cue_name[i++]='C'; cue_name[i++]='U'; cue_name[i++]='E';
      return cue_name;
    }
  }
  free(cue_name);
  return NULL;
}

CdIo *
cdio_open_bincue (const char *source_name)
{
  char *bin_name = cdio_is_cuefile(source_name);

  if (NULL != bin_name) {
    free(bin_name);
    return cdio_open_cue(source_name);
  } else {
    char *cue_name = cdio_is_binfile(source_name);
    CdIo *cdio = cdio_open_cue(cue_name);
    free(cue_name);
    return cdio;
  }
}

CdIo *
cdio_open_cue (const char *cue_name)
{
  CdIo *ret;
  _img_private_t *_data;
  char *bin_name;

  if (NULL == cue_name) return NULL;

  cdio_funcs _funcs = {
    .eject_media        = cdio_generic_bogus_eject_media,
    .free               = _cdio_bincue_destroy,
    .get_arg            = _cdio_get_arg,
    .get_default_device = cdio_get_default_device_bincue,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_mcn            = _cdio_get_mcn,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = _cdio_get_track_lba, 
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = _cdio_lseek,
    .read               = _cdio_read,
    .read_audio_sectors = _cdio_read_audio_sectors,
    .read_mode1_sector  = _cdio_read_mode1_sector,
    .read_mode1_sectors = _cdio_read_mode1_sectors,
    .read_mode2_sector  = _cdio_read_mode2_sector,
    .read_mode2_sectors = _cdio_read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  (_data)->gen.init    = false;
  (_data)->sector_2336 = false;
  (_data)->cue_name    = NULL;

  ret = cdio_new (_data, &_funcs);

  if (ret == NULL) return NULL;

  bin_name = cdio_is_cuefile(cue_name);

  if (NULL == bin_name) {
    cdio_error ("source name %s is not recognized as a CUE file", cue_name);
  }
  
  _cdio_set_arg (_data, "cue", cue_name);
  _cdio_set_arg (_data, "source", bin_name);
  free(bin_name);

  if (_cdio_init(_data)) {
    return ret;
  } else {
    _cdio_bincue_destroy(_data);
    free(ret);
    return NULL;
  }
}

bool
cdio_have_bincue (void)
{
  return true;
}
