/*
    $Id: bincue.c,v 1.22 2004/06/19 19:15:15 rocky Exp $

    Copyright (C) 2002, 2003, 2004 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>

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

static const char _rcsid[] = "$Id: bincue.c,v 1.22 2004/06/19 19:15:15 rocky Exp $";

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
  track_t       i_tracks;    /* number of tracks in image */
  track_t       i_first_track;   /* track number of first track */
  bool have_cue;
} _img_private_t;

static bool     _bincue_image_read_cue (_img_private_t *env);
static uint32_t _stat_size_bincue (void *user_data);

#include "image_common.h"

/*!
  Initialize image structures.
 */
static bool
_bincue_init (_img_private_t *env)
{
  lsn_t lead_lsn;

  if (env->gen.init)
    return false;

  if (!(env->gen.data_source = cdio_stdio_new (env->gen.source_name))) {
    cdio_warn ("init failed");
    return false;
  }

  /* Have to set init before calling _stat_size_bincue() or we will
     get into infinite recursion calling passing right here.
   */
  env->gen.init = true;  

  lead_lsn = _stat_size_bincue( (_img_private_t *) env);

  if (-1 == lead_lsn) 
    return false;

  /* Read in CUE sheet. */
  if ((env->cue_name != NULL)) {
    env->have_cue = _bincue_image_read_cue(env);
  }

  if (!env->have_cue ) {
    /* Time to fake things...
       Make one big track, track 0 and 1 are the same. 
       We are guessing stuff starts at msf 00:04:00 - 2 for the 150
       sector pregap and 2 for the cue information.
     */
    track_info_t  *this_track=&(env->tocent[0]);
    int blocksize = env->sector_2336 
      ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

    env->i_tracks           = 2;
    env->i_first_track      = 1;
    this_track->start_msf.m = to_bcd8(0);
    this_track->start_msf.s = to_bcd8(4);
    this_track->start_msf.f = to_bcd8(0);
    this_track->start_lba   = cdio_msf_to_lba(&this_track->start_msf);
    this_track->blocksize   = blocksize;
    this_track->track_format= TRACK_FORMAT_XA;
    this_track->track_green = true;


    env->tocent[1] = env->tocent[0];
  }
  
  /* Fake out leadout track and sector count for last track*/
  cdio_lsn_to_msf (lead_lsn, &env->tocent[env->i_tracks].start_msf);
  env->tocent[env->i_tracks].start_lba = cdio_lsn_to_lba(lead_lsn);
  env->tocent[env->i_tracks - env->i_first_track].sec_count = 
    cdio_lsn_to_lba(lead_lsn - env->tocent[env->i_tracks - env->i_first_track].start_lba);

  return true;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Would be libc's seek() but we have to adjust for the extra track header 
  information in each sector.
*/
static off_t
_lseek_bincue (void *user_data, off_t offset, int whence)
{
  _img_private_t *env = user_data;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. I'm guessing
     the 1st 24 bytes of a bin file are used for something.
  */
  off_t real_offset=0;

  unsigned int i;

  env->pos.lba = 0;
  for (i=0; i<env->i_tracks; i++) {
    track_info_t  *this_track=&(env->tocent[i]);
    env->pos.index = i;
    if ( (this_track->sec_count*this_track->datasize) >= offset) {
      int blocks            = offset / this_track->datasize;
      int rem               = offset % this_track->datasize;
      int block_offset      = blocks * this_track->blocksize;
      real_offset          += block_offset + rem;
      env->pos.buff_offset = rem;
      env->pos.lba        += blocks;
      break;
    }
    real_offset   += this_track->sec_count*this_track->blocksize;
    offset        -= this_track->sec_count*this_track->datasize;
    env->pos.lba += this_track->sec_count;
  }

  if (i==env->i_tracks) {
    cdio_warn ("seeking outside range of disk image");
    return -1;
  } else {
    real_offset += env->tocent[i].datastart;
    return cdio_stream_seek(env->gen.data_source, real_offset, whence);
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
_read_bincue (void *user_data, void *data, size_t size)
{
  _img_private_t *env = user_data;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  char *p = data;
  ssize_t final_size=0;
  ssize_t this_size;
  track_info_t  *this_track=&(env->tocent[env->pos.index]);
  ssize_t skip_size = this_track->datastart + this_track->endsize;

  while (size > 0) {
    long int rem = this_track->datasize - env->pos.buff_offset;
    if ((long int) size <= rem) {
      this_size = cdio_stream_read(env->gen.data_source, buf, size, 1);
      final_size += this_size;
      memcpy (p, buf, this_size);
      break;
    }

    /* Finish off reading this sector. */
    cdio_warn ("Reading across block boundaries not finished");

    size -= rem;
    this_size = cdio_stream_read(env->gen.data_source, buf, rem, 1);
    final_size += this_size;
    memcpy (p, buf, this_size);
    p += this_size;
    this_size = cdio_stream_read(env->gen.data_source, buf, rem, 1);
    
    /* Skip over stuff at end of this sector and the beginning of the next.
     */
    cdio_stream_read(env->gen.data_source, buf, skip_size, 1);

    /* Get ready to read another sector. */
    env->pos.buff_offset=0;
    env->pos.lba++;

    /* Have gone into next track. */
    if (env->pos.lba >= env->tocent[env->pos.index+1].start_lba) {
      env->pos.index++;
      this_track=&(env->tocent[env->pos.index]);
      skip_size = this_track->datastart + this_track->endsize;
    }
  }
  return final_size;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_stat_size_bincue (void *user_data)
{
  _img_private_t *env = user_data;
  long size;
  int blocksize = env->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  size = cdio_stream_stat (env->gen.data_source);

  if (size % blocksize)
    {
      cdio_warn ("image %s size (%ld) not multiple of blocksize (%d)", 
		 env->gen.source_name, size, blocksize);
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
_bincue_image_read_cue (_img_private_t *env)
{
  FILE *fp;
  char line[MAXLINE];

  int i_track;
  int min,sec,frame;
  int blocksize;
  int start_index;
  bool seen_first_index_for_track=false;

  if ( env == NULL ||  env->cue_name == NULL ) return false;

  fp = fopen (env->cue_name, "r");
  if (fp == NULL) return false;

  env->i_tracks=0;
  env->i_first_track=1;
  env->mcn=NULL;
  
  while ((fgets(line, MAXLINE, fp)) != NULL) {
    char s[80];
    char *p;
    /*printf("Retrieved line of length %zu :\n", read);
      printf("%s", line); */
    for (p=line; isspace(*p); p++) ;
    if (1==sscanf(p, "FILE \"%80s[^\"]", s)) {
      /* Should expand file name based on cue file basename.
      free(env->bin_file);
      env->bin_file = strdup(s);
      */
      /* printf("Found file name %s\n", s); */
    } else if (1==sscanf(p, "CATALOG %80s", s)) {
      env->mcn = strdup(s);
    } else if (2==sscanf(p, "TRACK %d MODE2/%d", &i_track, &blocksize)) {
      track_info_t  *this_track=&(env->tocent[env->i_tracks]);
      this_track->track_num   = i_track;
      this_track->num_indices = 0;
      this_track->track_format= TRACK_FORMAT_XA;
      this_track->track_green = true;
      env->i_tracks++;
      seen_first_index_for_track=false;
      /*printf("Added track %d with blocksize %d\n", i_track, blocksize);*/

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
	if (env->sector_2336) {
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

    } else if (2==sscanf(p, "TRACK %d MODE1/%d", &i_track, &blocksize)) {
      track_info_t *this_track=&(env->tocent[env->i_tracks]);
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

      this_track->track_num      = i_track;
      this_track->num_indices    = 0;
      this_track->track_format   = TRACK_FORMAT_DATA;
      this_track->track_green    = false;
      env->i_tracks++;
      seen_first_index_for_track=false;
      /*printf("Added track %d with blocksize %d\n", i_track, blocksize);*/

    } else if (1==sscanf(p, "TRACK %d AUDIO", &i_track)) {
      track_info_t  *this_track=&(env->tocent[env->i_tracks]);
      this_track->blocksize      = CDIO_CD_FRAMESIZE_RAW;
      this_track->datasize       = CDIO_CD_FRAMESIZE_RAW;
      this_track->datastart      = 0;
      this_track->endsize        = 0;
      this_track->track_num      = i_track;
      this_track->num_indices    = 0;
      this_track->track_format   = TRACK_FORMAT_AUDIO;
      this_track->track_green    = false;
      env->i_tracks++;
      seen_first_index_for_track=false;

    } else if (4==sscanf(p, "INDEX %d %d:%d:%d", 
			 &start_index, &min, &sec, &frame)) {
      track_info_t  *this_track=&(env->tocent[env->i_tracks - env->i_first_track]);
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

	if (env->i_tracks > 1) {
	  /* Figure out number of sectors for previous track */
	  track_info_t  *prev_track=&(env->tocent[env->i_tracks-2]);
	  if ( this_track->start_lba < prev_track->start_lba ) {
	    cdio_warn("track %d at LBA %lu starts before track %d at LBA %lu", 
		     env->i_tracks,   
		      (unsigned long int) this_track->start_lba, 
		      env->i_tracks, 
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
		       env->i_tracks);
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
  env->have_cue = env->i_tracks != 0;

  fclose (fp);
  return true;
}

/*!
   Reads a single audio sector from CD device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_audio_sectors_bincue (void *user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int ret;

  /* Why the adjustment of 272, I don't know. It seems to work though */
  if (lsn != 0) {
    ret = cdio_stream_seek (env->gen.data_source, 
			    (lsn * CDIO_CD_FRAMESIZE_RAW) - 272, SEEK_SET);
    if (ret!=0) return ret;

    ret = cdio_stream_read (env->gen.data_source, data, 
			    CDIO_CD_FRAMESIZE_RAW, nblocks);
  } else {
    /* We need to pad out the first 272 bytes with 0's */
    BZERO(data, 272);
    
    ret = cdio_stream_seek (env->gen.data_source, 0, SEEK_SET);

    if (ret!=0) return ret;

    ret = cdio_stream_read (env->gen.data_source, (uint8_t *) data+272, 
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
_read_mode1_sector_bincue (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *env = user_data;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int blocksize = env->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  ret = cdio_stream_seek (env->gen.data_source, lsn * blocksize, SEEK_SET);
  if (ret!=0) return ret;

  /* FIXME: Not completely sure the below is correct. */
  ret = cdio_stream_read (env->gen.data_source,
			  env->sector_2336 
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
_read_mode1_sectors_bincue (void *user_data, void *data, lsn_t lsn, 
			    bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_bincue (env, 
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
_read_mode2_sector_bincue (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *env = user_data;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

  /* NOTE: The logic below seems a bit wrong and convoluted
     to me, but passes the regression tests. (Perhaps it is why we get
     valgrind errors in vcdxrip). Leave it the way it was for now.
     Review this sector 2336 stuff later.
  */

  int blocksize = env->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  ret = cdio_stream_seek (env->gen.data_source, lsn * blocksize, SEEK_SET);
  if (ret!=0) return ret;

  ret = cdio_stream_read (env->gen.data_source,
			  env->sector_2336 
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
_read_mode2_sectors_bincue (void *user_data, void *data, lsn_t lsn, 
			    bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode2_sector_bincue (env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

#define free_if_notnull(obj) \
  if (NULL != obj) { free(obj); obj=NULL; };

static void 
_free_bincue (void *user_data) 
{
  _img_private_t *env = user_data;

  if (NULL == env) return;
  free_if_notnull(env->mcn);
  free_if_notnull(env->cue_name);
  cdio_generic_stdio_free(env);
  free(env);
}

/*!
  Eject media -- there's nothing to do here except free resources.
  We always return 2.
 */
static int
_eject_media_bincue(void *user_data)
{
  _free_bincue (user_data);
  return 2;
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" to set the source device in I/O operations 
  is the only valid key.

  0 is returned if no error was found, and nonzero if there as an error.
*/
static int
_set_arg_bincue (void *user_data, const char key[], const char value[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source"))
    {
      free_if_notnull (env->gen.source_name);

      if (!value)
	return -2;

      env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "sector"))
    {
      if (!strcmp (value, "2336"))
	env->sector_2336 = true;
      else if (!strcmp (value, "2352"))
	env->sector_2336 = false;
      else
	return -2;
    }
  else if (!strcmp (key, "cue"))
    {
      free_if_notnull (env->cue_name);

      if (!value)
	return -2;

      env->cue_name = strdup (value);
    }
  else
    return -1;

  return 0;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_bincue (void *user_data, const char key[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source")) {
    return env->gen.source_name;
  } else if (!strcmp (key, "cue")) {
    return env->cue_name;
  } else if (!strcmp(key, "access-mode")) {
    return "image";
  } 
  return NULL;
}

/*!
  Return an array of strings giving possible BIN/CUE disk images.
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
  cdio_free_device_list(drives);
  return drive;
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static cdio_drive_cap_t
_get_drive_cap_bincue (const void *user_data) {

  /* There may be more in the future but these we can handle now. 
     Also, we know we can't handle 
     LOCK, OPEN_TRAY, CLOSE_TRAY, SELECT_SPEED, SELECT_DISC
  */
  return CDIO_DRIVE_CAP_FILE | CDIO_DRIVE_CAP_MCN | CDIO_DRIVE_CAP_CD_AUDIO ;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
_get_track_format_bincue(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  
  if (i_track > env->i_tracks || i_track == 0) 
    return TRACK_FORMAT_ERROR;

  return env->tocent[i_track-env->i_first_track].track_format;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_get_track_green_bincue(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  
  if (i_track > env->i_tracks || i_track == 0) 
    return false;

  return env->tocent[i_track-env->i_first_track].track_green;
}

/*!  
  Return the starting LSN track number
  i_track in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lba_t
_get_lba_track_bincue(void *user_data, track_t i_track)
{
  _img_private_t *env = user_data;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = env->i_tracks+1;

  if (i_track <= env->i_tracks + env->i_first_track && i_track != 0) {
    return env->tocent[i_track-env->i_first_track].start_lba;
  } else 
    return CDIO_INVALID_LBA;
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
  
  if (cue_name == NULL) return NULL;

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
  
  if (bin_name == NULL) return NULL;

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

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_bincue (const char *psz_source_name, const char *psz_access_mode)
{
  if (psz_access_mode != NULL)
    cdio_warn ("there is only one access mode for bincue. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_bincue(psz_source_name);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
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

  cdio_funcs _funcs = {
    .eject_media        = _eject_media_bincue,
    .free               = _free_bincue,
    .get_arg            = _get_arg_bincue,
    .get_devices        = cdio_get_devices_bincue,
    .get_default_device = cdio_get_default_device_bincue,
    .get_drive_cap      = _get_drive_cap_bincue,
    .get_first_track_num= _get_first_track_num_image,
    .get_mcn            = _get_mcn_image,
    .get_num_tracks     = _get_num_tracks_image,
    .get_track_format   = _get_track_format_bincue,
    .get_track_green    = _get_track_green_bincue,
    .get_track_lba      = _get_lba_track_bincue, 
    .get_track_msf      = _get_track_msf_image,
    .lseek              = _lseek_bincue,
    .read               = _read_bincue,
    .read_audio_sectors = _read_audio_sectors_bincue,
    .read_mode1_sector  = _read_mode1_sector_bincue,
    .read_mode1_sectors = _read_mode1_sectors_bincue,
    .read_mode2_sector  = _read_mode2_sector_bincue,
    .read_mode2_sectors = _read_mode2_sectors_bincue,
    .set_arg            = _set_arg_bincue,
    .stat_size          = _stat_size_bincue
  };

  if (NULL == cue_name) return NULL;

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
  
  _set_arg_bincue (_data, "cue", cue_name);
  _set_arg_bincue (_data, "source", bin_name);
  free(bin_name);

  if (_bincue_init(_data)) {
    return ret;
  } else {
    _free_bincue(_data);
    free(ret);
    return NULL;
  }
}

bool
cdio_have_bincue (void)
{
  return true;
}
