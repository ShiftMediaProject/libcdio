/*
    $Id: _cdio_freebsd.c,v 1.2 2003/03/29 17:32:00 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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

/* This file contains Linux-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_freebsd.c,v 1.2 2003/03/29 17:32:00 rocky Exp $";

#include "cdio_assert.h"
#include "cdio_private.h"
#include "sector.h"
#include "util.h"

#ifdef HAVE_FREEBSD_CDROM

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h>
#endif
#include <sys/cdrio.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

/* for FreeBSD make a link to the right devnode, like /dev/acd0c */
#define DEFAULT_CDIO_DEVICE "/dev/cdrom"

#define TOTAL_TRACKS    (_obj->tochdr.ending_track \
			- obj->tochdr.starting_track + 1)
#define FIRST_TRACK_NUM (_obj->tochdr.starting_track)

typedef struct {
  int fd;

  int ioctls_debugged; /* for debugging */

  enum {
    _AM_NONE,
    _AM_IOCTL,
  } access_mode;

  char *source_name;
  
  bool init;

  /* Track information */
  bool toc_init;                         /* if true, info below is valid. */
  struct ioc_toc_header  tochdr;
  struct ioc_read_toc_seinge_entry tocent[100]; /* entry info for each track */

} _img_private_t;

/*!
  Initialize CD device.
 */
static bool
_cdio_init (_img_private_t *_obj)
{
  if (_obj->init) {
    cdio_error ("init called more than once");
    return false;
  }
  
  _obj->fd = open (_obj->source_name, O_RDONLY, 0);

  if (_obj->fd < 0)
    {
      cdio_error ("open (%s): %s", _obj->source_name, strerror (errno));
      return false;
    }

  _obj->init = true;
  _obj->toc_init = false;
  return true;
}

/*!
  Release and free resources associated with cd. 
 */
static void
_cdio_free (void *user_data)
{
  _img_private_t *_obj = user_data;

  if (NULL == _obj) return;
  free (_obj->source_name);

  if (_obj->fd >= 0) close (_obj->fd);
  free (_obj);
}

static int 
_set_bsize (int fd, unsigned int bsize)
{
  struct cdrom_generic_command cgc;

  if (ioctl (fd, CDRIOCSETBLOCKSIZE, &bsize) == -1) {
    cdio_error("error in ioctl(CDRIOCSETBLOCKSIZE): %s\n", strerror(errno));
    return 0;
  }
  return 1;
}

static int
_read_mode2 (int fd, void *buf, lba_t lba, unsigned nblocks, 
	     bool _workaround)
{
  unsigned l = 0;
  int retval = 0;

  while (nblocks > 0)
    {
      const unsigned nblocks2 = (nblocks > 25) ? 25 : nblocks;
      void *buf2 = ((char *)buf ) + (l * 2336);
      
      retval |= __read_mode2 (fd, buf2, lba + l, nblocks2, _workaround);

      if (retval)
	break;

      nblocks -= nblocks2;
      l += nblocks2;
    }

  return retval;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_mode2_sector (void *user_data, void *data, lsn_t lsn, 
		    bool mode2_form2)
{
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *_obj = user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);

  if (_obj->ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");

  if (_obj->ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (_obj->ioctls_debugged < 75 
      || (_obj->ioctls_debugged < (30 * 75)  
	  && _obj->ioctls_debugged % 75 == 0)
      || _obj->ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %2.2d:%2.2d:%2.2d",
	       msf->cdmsf_min0, msf->cdmsf_sec0, msf->cdmsf_frame0);
  
  _obj->ioctls_debugged++;
 
 retry:
  switch (_obj->access_mode)
    {
    case _AM_NONE:
      cdio_error ("no way to read mode2");
      return 1;
      break;
      
    case _AM_IOCTL:
      if (ioctl (_obj->fd, CDROMREADMODE2, &buf) == -1)
	{
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
      break;
    }

  if (mode2_form2)
    memcpy (data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)data), buf + 8, FORM1_DATA_SIZE);
  
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_read_mode2_sectors (void *user_data, void *data, lsn_t lsn, 
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
	      M2F1_SECTOR_SIZE);
    }
  }
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_cdio_stat_size (void *user_data)
{
  _img_private_t *_obj = user_data;

  struct cdrom_tocentry tocent;
  uint32_t size;

  tocent.cdte_track = CDROM_LEADOUT;
  tocent.cdte_format = CDROM_LBA;
  if (ioctl (_obj->fd, CDROMREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  size = tocent.cdte_addr.lba;

  return size;
}

/*!
  Set the key "arg" to "value" in source device.
*/
static int
_cdio_set_arg (void *user_data, const char key[], const char value[])
{
  _img_private_t *_obj = user_data;

  if (!strcmp (key, "source"))
    {
      if (!value)
	return -2;

      free (_obj->source_name);
      
      _obj->source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      if (!strcmp(value, "IOCTL"))
	_obj->access_mode = _AM_IOCTL;
      else if (!strcmp(value, "READ_CD"))
	_obj->access_mode = _AM_READ_CD;
      else if (!strcmp(value, "READ_10"))
	_obj->access_mode = _AM_READ_10;
      else
	cdio_error ("unknown access type: %s. ignored.", value);
    }
  else 
    return -1;

  return 0;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return false if successful or true if an error.
*/
static bool
_cdio_read_toc (_img_private_t *_obj) 
{
  int i;
  struct ioc_read_toc_entry te;

  /* read TOC header */
  if ( ioctl(_obj->fd, CDROMREADTOCHDR, &_obj->tochdr) == -1 ) {
    cdio_error("error in ioctl(CDROMREADTOCHDR): %s\n", strerror(errno));
    return false;
  }

  te.address_format = CD_MSF_FORMAT;
  te.starting_track = 0;
  te.data_len = (TOTAL_TRACKS+1) * sizeof(struct cd_toc_entry);

  te.data = _obj->tocent;
  
  if ( ioctl(_obj->fd, CDIOREADTOCENTRYS, &te) == -1 ) {
    cdio_error("%s %d: %s\n",
	       "error in ioctl CDROMREADTOCENTRY for track", 
	       i, strerror(errno));
    return false;
  }

  return true;
}

/*!
  Eject media in CD drive. If successful, as a side effect we 
  also free obj.
 */
static int 
_cdio_eject_media (void *user_data) {

  _img_private_t *_obj = user_data;
  int ret=2;
  int status;
  int fd;

  if ((fd = open(_obj->source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    ret = 1;
    if (ioctl(fd, CDIOCALLOW) == -1) {
      cdio_error("ioctl(fd, CDIOCALLOW) failed: %s\n", strerror(errno));
    } else if (ioctl(fd, CDIOCEJECT) == -1) {
      cdio_error("ioctl(CDIOCEJECT) failed: %s\n" strerror(errno));
    } else {
      ret = 0;
    }
    close(fd);
    _cdio_free((void *) _obj);
  }

  return ret;
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
  } else if (!strcmp (key, "access-mode")) {
    switch (_obj->access_mode) {
    case _AM_IOCTL:
      return "ioctl";
    case _AM_NONE:
      return "no access method";
    }
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
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return FIRST_TRACK_NUM;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_num_tracks(void *user_data) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return TOTAL_TRACKS;
}

/*!  
  Get format of track. 
*/
static track_format_t
_cdio_get_track_format(void *user_data, track_t track_num) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num > TOTAL_TRACKS || track_num == 0)
    return TRACK_FORMAT_ERROR;

  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
   */
  if (_obj->tocent[track_num-1].cdte_ctrl & CDROM_DATA_TRACK) {
    if (_obj->tocent[track_num-1].cdte_format == 0x10)
      return TRACK_FORMAT_CDI;
    else if (_obj->tocent[track_num-1].cdte_format == 0x20) 
      return TRACK_FORMAT_XA;
    else
      return TRACK_FORMAT_DATA;
  } else
    return TRACK_FORMAT_AUDIO;
  
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
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

  if (track_num > TOTAL_TRACKS+1 || track_num == 0)
    return false;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cdinfo for a while.
   */
  return ((_obj->tocent[track_num-1].cdte_ctrl & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
_cdio_get_track_msf(void *user_data, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = user_data;

  if (NULL == msf) return false;

  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

  if (track_num > TOTAL_TRACKS+1 || track_num == 0) {
    return false;
  } else {
    struct cdrom_msf0  *msf0= &_obj->tocent[track_num-1].cdte_addr.msf;
    msf->m = to_bcd8(msf0->minute);
    msf->s = to_bcd8(msf0->second);
    msf->f = to_bcd8(msf0->frame);
    return true;
  }
}

#endif /* HAVE_FREEBSD_CDROM */

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_freebsd (const char *source_name)
{

#ifdef HAVE_FREEBSD_CDROM
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
    .get_track_lba      = NULL, /* This could be implemented if need be. */
    .get_track_msf      = _cdio_get_track_msf,
    .read_mode2_sector  = _read_mode2_sector,
    .read_mode2_sectors = _read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->access_mode    = _AM_READ_CD;
  _data->init           = false;
  _data->fd             = -1;

  _cdio_set_arg(_data, "source", (NULL == source_name) 
		? DEFAULT_CDIO_DEVICE: source_name);

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (_cdio_init(_data))
    return ret;
  else {
    _cdio_free (_data);
    return NULL;
  }
  
#else 
  return NULL;
#endif /* HAVE_FREEBSD_CDROM */

}

bool
cdio_have_freebsd (void)
{
#ifdef HAVE_FREEBSD_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_FREEBSD_CDROM */
}

