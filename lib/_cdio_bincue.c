/*
    $Id: _cdio_bincue.c,v 1.2 2003/03/29 17:32:00 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002,2003 Rocky Bernstein <rocky@panix.com>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_bincue.c,v 1.2 2003/03/29 17:32:00 rocky Exp $";

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "cdio_assert.h"
#include "cdio_private.h"
#include "logging.h"
#include "sector.h"
#include "util.h"
#include "_cdio_stdio.h"

/* reader */

#define DEFAULT_CDIO_DEVICE "videocd.bin"
#define DEFAULT_CDIO_CUE    "videocd.cue"

typedef struct {
  int            blocksize;
  int            track_num;   /* Probably is index+1 */
  msf_t          start_msf;
  lba_t          start_lba;
  int            start_index;
  int            sec_count;  /* Number of sectors in this track. Does not
				include pregap */
  int            num_indices;
  int            flags; /* "DCP", "4CH", "PRE" */
  track_format_t track_format;
  bool           track_green;
  
} track_info_t;


typedef struct {
  char         *source_name;
  bool init;
  bool sector_2336_flag;

  CdioDataSource *data_source;
  char         *cue_name;
  track_info_t  tocent[100];     /* entry info for each track */
  track_t       total_tracks;    /* number of tracks in image */
  track_t       first_track_num; /* track number of first track */
  bool have_cue;
} _img_private_t;

static bool     _cdio_image_read_cue (_img_private_t *_obj);
static uint32_t _cdio_stat_size (void *user_data);

/*!
  Initialize image structures.
 */
static bool
_cdio_init (_img_private_t *_obj)
{
  lsn_t lead_lsn;

  if (_obj->init)
    return false;

  if (!(_obj->data_source = cdio_stdio_new (_obj->source_name))) {
    cdio_error ("init failed");
    return false;
  }

  /* Have to set init before calling _cdio_stat_size() or we will
     get into infinite recursion calling passing right here.
   */
  _obj->init = true;  

  lead_lsn = _cdio_stat_size( (_img_private_t *) _obj);

  /* Read in CUE sheet. */
  if ((_obj->cue_name != NULL)) {
    _obj->have_cue == _cdio_image_read_cue(_obj);
  }

  if (!_obj->have_cue ) {
    /* Time to fake things...
       Make one big track, track 0 and 1 are the same. 
       We are guessing stuff starts at msf 00:04:00 - 2 for the 150
       sector pregap and 2 for the cue information.
     */
    track_info_t  *this_track=&(_obj->tocent[0]);
    int blocksize = _obj->sector_2336_flag 
      ? M2RAW_SECTOR_SIZE : CD_RAW_SECTOR_SIZE;

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
_cdio_lseek (void *user_data, off_t offset, int whence)
{
  _img_private_t *_obj = user_data;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. I'm guessing
     the 1st 24 bytes of a bin file are used for something.
  */
  off_t real_offset=24;

  unsigned int i;
  unsigned int user_datasize;

  for (i=0; i<_obj->total_tracks; i++) {
    track_info_t  *this_track=&(_obj->tocent[i]);
    switch (this_track->track_format) {
    case TRACK_FORMAT_AUDIO:
      user_datasize=CDDA_SECTOR_SIZE;
      break;
    case TRACK_FORMAT_CDI:
      user_datasize=FORM1_DATA_SIZE;
      break;
    case TRACK_FORMAT_XA:
      user_datasize=FORM1_DATA_SIZE;
      break;
    default:
      cdio_warn ("track %d has unknown format %d",
		 i+1, this_track->track_format);
    }

    if ( (this_track->sec_count*user_datasize) >= offset) {
      int blocks = offset / user_datasize;
      int rem    = offset % user_datasize;
      int block_offset = blocks * CD_RAW_SECTOR_SIZE;
      real_offset += block_offset + rem;
      break;
    }
    real_offset += this_track->sec_count*CD_RAW_SECTOR_SIZE;
    offset -= this_track->sec_count*user_datasize;
  }

  if (i==_obj->total_tracks) {
    cdio_warn ("seeking outside range of disk image");
    return -1;
  } else
    return cdio_stream_seek(_obj->data_source, real_offset, whence);
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  FIXME: 
   At present we assume a read doesn't cross sector or track
   boundaries.
*/
static ssize_t
_cdio_read (void *user_data, void *buf, size_t size)
{
  _img_private_t *_obj = user_data;
  return cdio_stream_read(_obj->data_source, buf, size, 1);
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_cdio_stat_size (void *user_data)
{
  _img_private_t *_obj = user_data;
  long size;
  int blocksize = _obj->sector_2336_flag 
    ? M2RAW_SECTOR_SIZE : CD_RAW_SECTOR_SIZE;

  _cdio_init (_obj);
  
  size = cdio_stream_stat (_obj->data_source);

  if (size % blocksize)
    {
      cdio_warn ("image file not multiple of blocksize (%d)", 
                blocksize);
      if (size % M2RAW_SECTOR_SIZE == 0)
	cdio_warn ("this may be a 2336-type disc image");
      else if (size % CD_RAW_SECTOR_SIZE == 0)
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
  while ((fgets(line, MAXLINE, fp)) != NULL) {
    char *s=NULL;
    char *p;
    /*printf("Retrieved line of length %zu :\n", read);
      printf("%s", line); */
    for (p=line; isspace(*p); p++) ;
    if (1==sscanf(p, "FILE \"%a[^\"]", &s)) {
      /* Should expand file name based on cue file basename.
      free(_obj->bin_file);
      _obj->bin_file = s;
      */
      /* printf("Found file name %s\n", s); */
    } else if (2==sscanf(p, "TRACK %d MODE2/%d", &track_num, &blocksize)) {
      track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
      this_track->blocksize   = blocksize;
      this_track->track_num   = track_num;
      this_track->num_indices = 0;
      this_track->track_format= TRACK_FORMAT_XA;
      this_track->track_green = true;
      _obj->total_tracks++;
      seen_first_index_for_track=false;
      /*printf("Added track %d with blocksize %d\n", track_num, blocksize);*/

    } else if (2==sscanf(p, "TRACK %d MODE1/%d", &track_num, &blocksize)) {
      track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
      this_track->blocksize   = blocksize;
      this_track->track_num   = track_num;
      this_track->num_indices = 0;
      this_track->track_format= TRACK_FORMAT_CDI;
      this_track->track_green = false;
      _obj->total_tracks++;
      seen_first_index_for_track=false;
      /*printf("Added track %d with blocksize %d\n", track_num, blocksize);*/

    } else if (1==sscanf(p, "TRACK %d AUDIO", &track_num)) {
      track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
      this_track->blocksize   = CDDA_SECTOR_SIZE;
      this_track->track_num   = track_num;
      this_track->num_indices = 0;
      this_track->track_format= TRACK_FORMAT_AUDIO;
      this_track->track_green = false;
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
	  this_track->start_msf.m = to_bcd8 (min);
	  this_track->start_msf.s = to_bcd8 (sec)+2;
	  this_track->start_msf.f = to_bcd8 (frame);
	  this_track->start_lba   = cdio_msf_to_lba(&this_track->start_msf);
	  seen_first_index_for_track=true;
	}

	if (_obj->total_tracks > 1) {
	  /* Figure out number of sectors for previous track */
	  track_info_t  *prev_track=&(_obj->tocent[_obj->total_tracks-2]);
	  if ( this_track->start_lba < prev_track->start_lba ) {
	    cdio_warn ("track %d at LBA %d starts before track %d at LBA %d", 
		       _obj->total_tracks,   this_track->start_lba, 
		       _obj->total_tracks-1, prev_track->start_lba);
	    prev_track->sec_count = 0;
	  } else if ( this_track->start_lba >= prev_track->start_lba 
		      + CDIO_PREGAP_SECTORS ) {
	    prev_track->sec_count = this_track->start_lba - 
	      prev_track->start_lba - CDIO_PREGAP_SECTORS ;
	  } else {
	    cdio_warn ("%d fewer than pregap (%d) sectors in track %d", 
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
  Release and free resources used here.
 */
static void
_cdio_free (void *user_data)
{
  _img_private_t *_obj = user_data;

  free (_obj->source_name);

  if (_obj->data_source)
    cdio_stream_destroy (_obj->data_source);

  free (_obj);
}

static int
_read_mode2_sector (void *user_data, void *data, uint32_t lsn, 
		    bool mode2_form2)
{
  _img_private_t *_obj = user_data;
  char buf[CD_RAW_SECTOR_SIZE] = { 0, };
  int blocksize = _obj->sector_2336_flag 
    ? M2RAW_SECTOR_SIZE : CD_RAW_SECTOR_SIZE;

  _cdio_init (_obj);

  cdio_stream_seek (_obj->data_source, lsn * blocksize, SEEK_SET);

  cdio_stream_read (_obj->data_source,
		    _obj->sector_2336_flag ? (buf + 12 + 4) : buf,
		    blocksize, 1);

  if (mode2_form2)
    memcpy (data, buf + 12 + 4, M2RAW_SECTOR_SIZE);
  else
    memcpy (data, buf + 12 + 4 + 8, FORM1_DATA_SIZE);

  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_read_mode2_sectors (void *user_data, void *data, uint32_t lsn, 
		     bool mode2_form2, unsigned nblocks)
{
  _img_private_t *_obj = user_data;
  int i;
  int retval;

  for (i = 0; i < nblocks; i++) {
    if (mode2_form2) {
      if ( (retval = _read_mode2_sector (_obj, 
					 ((char *)data) + (M2RAW_SECTOR_SIZE * i),
					 lsn + i, true)) )
	return retval;
    } else {
      char buf[M2RAW_SECTOR_SIZE] = { 0, };
      if ( (retval = _read_mode2_sector (_obj, buf, lsn + i, true)) )
	return retval;
      
      memcpy (((char *)data) + (M2F1_SECTOR_SIZE * i), buf + 8, 
	      FORM1_DATA_SIZE);
    }
  }
  return 0;
}

/*!
  Set the device to use in I/O operations.
*/
static int
_cdio_set_arg (void *user_data, const char key[], const char value[])
{
  _img_private_t *_obj = user_data;

  if (!strcmp (key, "source"))
    {
      free (_obj->source_name);

      if (!value)
	return -2;

      _obj->source_name = strdup (value);
    }
  else if (!strcmp (key, "sector"))
    {
      if (!strcmp (value, "2336"))
	_obj->sector_2336_flag = true;
      else if (!strcmp (value, "2352"))
	_obj->sector_2336_flag = false;
      else
	return -2;
    }
  else if (!strcmp (key, "cue"))
    {
      free (_obj->cue_name);

      if (!value)
	return -2;

      _obj->cue_name = strdup (value);
    }
  else
    return -1;

  return 0;
}

/*!
  Eject media -- there's nothing to do here. We always return 2.
  also free obj.
 */
static int 
_cdio_eject_media (void *user_data) {
  /* Sort of a stub here. Perhaps log a message? */
  return 2;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_cdio_get_arg (void *user_data, const char key[])
{
  _img_private_t *_obj = user_data;

  if (!strcmp (key, "source")) {
    return _obj->source_name;
  } else if (!strcmp (key, "cue")) {
    return _obj->cue_name;
  } 
  return NULL;
}

/*!
  Return a string containing the default VCD device if none is specified.
 */
static char *
_cdio_get_default_device()
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *user_data) 
{
  _img_private_t *_obj = user_data;
  
  _cdio_init (_obj);

  return _obj->first_track_num;
}

/*! 
  Return the number of tracks in the current medium.
  If no cuesheet is available, We fake it an just say there's
  one big track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_num_tracks(void *user_data) 
{
  _img_private_t *_obj = user_data;
  _cdio_init (_obj);

  return _obj->have_cue && _obj->total_tracks > 0 ? _obj->total_tracks : 1;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
_cdio_get_track_format(void *user_data, track_t track_num) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->init) _cdio_init(_obj);

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
_cdio_get_track_green(void *user_data, track_t track_num) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->init) _cdio_init(_obj);

  if (track_num > _obj->total_tracks || track_num == 0) 
    return false;

  return _obj->tocent[track_num-1].track_green;
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  1 is returned on error.
*/
static lba_t
_cdio_get_track_lba(void *user_data, track_t track_num)
{
  _img_private_t *_obj = user_data;
  _cdio_init (_obj);

  if (track_num == CDIO_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

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
_cdio_get_track_msf(void *user_data, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = user_data;
  _cdio_init (_obj);

  if (NULL == msf) return false;

  if (track_num == CDIO_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

  if (track_num <= _obj->total_tracks+1 && track_num != 0) {
    *msf = _obj->tocent[track_num-1].start_msf;
    return true;
  } else 
    return false;
}

CdIo *
cdio_open_bincue (const char *source_name)
{
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = _cdio_free,
    .get_arg            = _cdio_get_arg,
    .get_default_device = _cdio_get_default_device,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = _cdio_get_track_lba, 
    .get_track_msf      = _cdio_get_track_msf,
    .read_mode2_sector  = _read_mode2_sector,
    .read_mode2_sectors = _read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->init           = false;

  /* FIXME: should set cue initially.  */
  _data->cue_name       = NULL;

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  _cdio_set_arg(_data, "source", (NULL == source_name) 
		? DEFAULT_CDIO_DEVICE: source_name);

  return ret;
}

CdIo *
cdio_open_cue (const char *cue_name)
{
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = _cdio_free,
    .get_arg            = _cdio_get_arg,
    .get_default_device = _cdio_get_default_device,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = _cdio_get_track_lba, 
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = _cdio_lseek,
    .read               = _cdio_read,
    .read_mode2_sector  = _read_mode2_sector,
    .read_mode2_sectors = _read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->init           = false;

  if (cue_name != NULL) {
    char *source_name=strdup(cue_name);
    int i=strlen(source_name)-strlen("cue");

    if (i>0) {
      if (cue_name[i]=='c' && cue_name[i+1]=='u' && cue_name[i+2]=='e') {
	source_name[i++]='b'; source_name[i++]='i'; source_name[i++]='n';
      } 
      else if (cue_name[i]=='C' && cue_name[i+1]=='U' && cue_name[i+2]=='E') {
	source_name[i++]='B'; source_name[i++]='I'; source_name[i++]='N';
      }
    }
    _cdio_set_arg (_data, "cue", cue_name);
    _cdio_set_arg (_data, "source", source_name);
    free(source_name);
  }

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (_cdio_init(_data))
    return ret;
  else {
    _cdio_free (_data);
    return NULL;
  }
}

bool
cdio_have_bincue (void)
{
  return true;
}
