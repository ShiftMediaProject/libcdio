/*
    $Id: _cdio_freebsd.c,v 1.25 2004/04/30 06:54:15 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

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

/* This file contains FreeBSD-specific code and implements low-level
   control of the CD drive. Culled initially I think from xine's or
   mplayer's FreeBSD code with lots of modifications.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_freebsd.c,v 1.25 2004/04/30 06:54:15 rocky Exp $";

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"

/* Is this the right default? */
#define DEFAULT_CDIO_DEVICE "/dev/acd0c"

#include <string.h>

#ifdef HAVE_FREEBSD_CDROM

#include <stdio.h>
#include <stdlib.h>
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

#define TOTAL_TRACKS    ( _obj->tochdr.ending_track \
			- _obj->tochdr.starting_track + 1)
#define FIRST_TRACK_NUM (_obj->tochdr.starting_track)

typedef enum {
  _AM_NONE,
  _AM_IOCTL,
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  char *source_name;
  
  bool init;

  /* Track information */
  bool toc_init;                         /* if true, info below is valid. */
  struct ioc_toc_header  tochdr;
  struct cd_toc_entry tocent[100];       /* entry info for each track */

} _img_private_t;

/* Check a drive to see if it is a CD-ROM 
   Return 1 if a CD-ROM. 0 if it exists but isn't a CD-ROM drive
   and -1 if no device exists .
*/
static bool
cdio_is_cdrom(char *drive, char *mnttype)
{
  bool is_cd=false;
  int cdfd;
  struct ioc_toc_header    tochdr;
  
  /* If it doesn't exist, return -1 */
  if ( !cdio_is_device_quiet_generic(drive) ) {
    return(false);
  }
  
  /* If it does exist, verify that it's an available CD-ROM */
  cdfd = open(drive, (O_RDONLY|O_EXCL|O_NONBLOCK), 0);

  /* Should we want to test the condition in more detail:
     ENOENT is the error for /dev/xxxxx does not exist;
     ENODEV means there's no drive present. */

  if ( cdfd >= 0 ) {
    if ( ioctl(cdfd, CDIOREADTOCHEADER, &tochdr) != -1 ) {
      is_cd = true;
    }
    close(cdfd);
    }
  /* Even if we can't read it, it might be mounted */
  else if ( mnttype && (strcmp(mnttype, "iso9660") == 0) ) {
    is_cd = true;
  }
  return(is_cd);
}

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
_read_audio_sectors_freebsd (void *env, void *data, lsn_t lsn,
			  unsigned int nblocks)
{
  _img_private_t *_obj = env;
  unsigned char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct ioc_read_audio cdda;

  cdda.address.lba    = lsn;
  cdda.buffer         = buf;
  cdda.nframes        = nblocks;
  cdda.address_format = CD_LBA_FORMAT;

  /* read a frame */
  if(ioctl(_obj->gen.fd, CDIOCREADAUDIO, &cdda) < 0) {
    perror("CDIOCREADAUDIO");
    return 1;
  }
  memcpy (data, buf, CDIO_CD_FRAMESIZE_RAW);

  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_mode2_sector_freebsd (void *env, void *data, lsn_t lsn, 
			    bool b_form2)
{
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  int retval;

  if ( (retval = _read_audio_sectors_freebsd (env, buf, lsn, 1)) )
    return retval;
    
  if (b_form2)
    memcpy (data, buf + CDIO_CD_XA_SYNC_HEADER, M2RAW_SECTOR_SIZE);
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
_read_mode2_sectors_freebsd (void *env, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *_obj = env;
  int i;
  int retval;

  for (i = 0; i < nblocks; i++) {
    if (b_form2) {
      if ( (retval = _read_mode2_sector_freebsd (_obj, 
					  ((char *)data) + (M2RAW_SECTOR_SIZE * i),
					  lsn + i, true)) )
	return retval;
    } else {
      char buf[M2RAW_SECTOR_SIZE] = { 0, };
      if ( (retval = _read_mode2_sector_freebsd (_obj, buf, lsn + i, true)) )
	return retval;
      
      memcpy (((char *)data) + (CDIO_CD_FRAMESIZE * i), 
	      buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    }
  }
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_stat_size_freebsd (void *env)
{
  _img_private_t *_obj = env;

  struct ioc_read_toc_single_entry tocent;
  uint32_t size;

  tocent.track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.address_format = CD_LBA_FORMAT;
  if (ioctl (_obj->gen.fd, CDIOREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  size = tocent.entry.addr.lba;

  return size;
}

/*!
  Set the key "arg" to "value" in source device.
*/
static int
_set_arg_freebsd (void *env, const char key[], const char value[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source"))
    {
      if (!value)
	return -2;

      free (_obj->gen.source_name);
      
      _obj->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      if (!strcmp(value, "IOCTL"))
	_obj->access_mode = _AM_IOCTL;
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
  if ( ioctl(_obj->gen.fd, CDIOREADTOCHEADER, &_obj->tochdr) == -1 ) {
    cdio_error("error in ioctl(CDIOREADTOCHEADER): %s\n", strerror(errno));
    return false;
  }

  te.address_format = CD_LBA_FORMAT;
  te.starting_track = 0;
  te.data_len = (TOTAL_TRACKS+1) * sizeof(struct cd_toc_entry);

  te.data = _obj->tocent;
  
  if ( ioctl(_obj->gen.fd, CDIOREADTOCENTRYS, &te) == -1 ) {
    cdio_error("%s %d: %s\n",
	       "error in ioctl CDROMREADTOCENTRYS for track", 
	       i, strerror(errno));
    return false;
  }

  return true;
}

/*!
  Eject media. Return 1 if successful, 0 otherwise.
 */
static int 
_eject_media_freebsd (void *env) 
{
  _img_private_t *_obj = env;
  int ret=2;
  int fd;

  if ((fd = open(_obj->gen.source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    ret = 1;
    if (ioctl(fd, CDIOCALLOW) == -1) {
      cdio_error("ioctl(fd, CDIOCALLOW) failed: %s\n", strerror(errno));
    } else if (ioctl(fd, CDIOCEJECT) == -1) {
      cdio_error("ioctl(CDIOCEJECT) failed: %s\n", strerror(errno));
    } else {
      ret = 0;
    }
    close(fd);
  }

  return ret;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_freebsd (void *env, const char key[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source")) {
    return _obj->gen.source_name;
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
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_get_first_track_num_freebsd(void *env) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return FIRST_TRACK_NUM;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

  FIXME: This is just a guess. 

 */
static char *
_get_mcn_freebsd (const void *env) {

  const _img_private_t *_obj = env;
  struct ioc_read_subchannel subchannel;
  struct cd_sub_channel_info subchannel_info;

  subchannel.address_format = CD_LBA_FORMAT;
  subchannel.data_format    = CD_MEDIA_CATALOG;
  subchannel.track          = 0;
  subchannel.data_len       = 1;
  subchannel.data           = &subchannel_info;

  if(ioctl(_obj->gen.fd, CDIOCREADSUBCHANNEL, &subchannel) < 0) {
    perror("CDIOCREADSUBCHANNEL");
    return NULL;
  }

  /* Probably need a loop over tracks rather than give up if we 
     can't find in track 0.
   */
  if (subchannel_info.what.media_catalog.mc_valid)
    return strdup(subchannel_info.what.media_catalog.mc_number);
  else 
    return NULL;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_get_num_tracks_freebsd(void *env) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return TOTAL_TRACKS;
}

/*!  
  Get format of track. 

  FIXME: We're just guessing this from the GNU/Linux code.
  
*/
static track_format_t
_get_track_format_freebsd(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  struct ioc_read_subchannel subchannel;
  struct cd_sub_channel_info subchannel_info;

  subchannel.address_format = CD_LBA_FORMAT;
  subchannel.data_format    = CD_CURRENT_POSITION;
  subchannel.track          = track_num;
  subchannel.data_len       = 1;
  subchannel.data           = &subchannel_info;

  if(ioctl(_obj->gen.fd, CDIOCREADSUBCHANNEL, &subchannel) < 0) {
    perror("CDIOCREADSUBCHANNEL");
    return 1;
  }
  
  if (subchannel_info.what.position.control == 0x04) {
    if (subchannel_info.what.position.data_format == 0x10)
      return TRACK_FORMAT_CDI;
    else if (subchannel_info.what.position.data_format == 0x20) 
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
_get_track_green_freebsd(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  struct ioc_read_subchannel subchannel;
  struct cd_sub_channel_info subchannel_info;

  subchannel.address_format = CD_LBA_FORMAT;
  subchannel.data_format    = CD_CURRENT_POSITION;
  subchannel.track          = track_num;
  subchannel.data_len       = 1;
  subchannel.data           = &subchannel_info;

  if(ioctl(_obj->gen.fd, CDIOCREADSUBCHANNEL, &subchannel) < 0) {
    perror("CDIOCREADSUBCHANNEL");
    return 1;
  }
  
  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cdinfo for a while.
   */
  return (subchannel_info.what.position.control & 2) != 0;
}

/*!  
  Return the starting LSN track number
  track_num in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lba_t
_get_track_lba_freebsd(void *env, track_t track_num)
{
  _img_private_t *_obj = env;

  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

  if (track_num > TOTAL_TRACKS+1 || track_num == 0) {
    return CDIO_INVALID_LBA;
  } else {
    return _obj->tocent[track_num-1].addr.lba;
  }
}

#endif /* HAVE_FREEBSD_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_freebsd (void)
{
#ifndef HAVE_FREEBSD_CDROM
  return NULL;
#else
  char drive[40];
  char **drives = NULL;
  unsigned int num_drives=0;
  bool exists=true;
  char c;
  
  /* Scan the system for CD-ROM drives.
  */

#ifdef USE_ETC_FSTAB

  struct fstab *fs;
  setfsent();
  
  /* Check what's in /etc/fstab... */
  while ( (fs = getfsent()) )
    {
      if (strncmp(fs->fs_spec, "/dev/sr", 7))
	cdio_add_device_list(&drives, fs->fs_spec, &num_drives);
    }
  
#endif

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for ( c='0'; exists && c <='9'; c++ ) {
    sprintf(drive, "/dev/acd%cc", c);
    exists = cdio_is_cdrom(drive, NULL);
    if ( exists ) {
      cdio_add_device_list(&drives, drive, &num_drives);
    }
  }
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /*HAVE_FREEBSD_CDROM*/
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char *
cdio_get_default_device_freebsd()
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_freebsd (const char *psz_source_name, const char *psz_access_mode)
{
  if (psz_access_mode != NULL)
    cdio_warn ("there is only one access mode for freebsd. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_freebsd(psz_source_name);
}


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
    .eject_media        = _eject_media_freebsd,
    .free               = cdio_generic_free,
    .get_arg            = _get_arg_freebsd,
    .get_default_device = cdio_get_default_device_freebsd,
    .get_devices        = cdio_get_devices_freebsd,
    .get_drive_cap      = NULL,
    .get_first_track_num= _get_first_track_num_freebsd,
    .get_mcn            = _get_mcn_freebsd,
    .get_num_tracks     = _get_num_tracks_freebsd,
    .get_track_format   = _get_track_format_freebsd,
    .get_track_green    = _get_track_green_freebsd,
    .get_track_lba      = _get_track_lba_freebsd, 
    .get_track_msf      = NULL,
    .lseek              = cdio_generic_lseek,
    .read               = cdio_generic_read,
    .read_audio_sectors = _read_audio_sectors_freebsd,
    .read_mode2_sector  = _read_mode2_sector_freebsd,
    .read_mode2_sectors = _read_mode2_sectors_freebsd,
    .set_arg            = _set_arg_freebsd,
    .stat_size          = _stat_size_freebsd
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->access_mode    = _AM_IOCTL;
  _data->gen.init       = false;
  _data->gen.fd         = -1;

  _set_arg_freebsd(_data, "source", (NULL == source_name) 
		   ? DEFAULT_CDIO_DEVICE: source_name);

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (cdio_generic_init(_data))
    return ret;
  else {
    cdio_generic_free (_data);
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
