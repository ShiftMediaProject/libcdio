/*
    $Id: _cdio_sunos.c,v 1.25 2004/03/16 12:18:32 rocky Exp $

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"
#include "scsi_mmc.h"

#define DEFAULT_CDIO_DEVICE "/vol/dev/aliases/cdrom0"

#ifdef HAVE_SOLARIS_CDROM

static const char _rcsid[] = "$Id: _cdio_sunos.c,v 1.25 2004/03/16 12:18:32 rocky Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef HAVE_SYS_CDIO_H
# include <sys/cdio.h> /* CDIOCALLOW etc... */
#else 
#error "You need <sys/cdio.h> to have CDROM support"
#endif

#include <sys/dkio.h>
#include <sys/scsi/generic/commands.h>
#include <sys/scsi/impl/uscsi.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define TOTAL_TRACKS    (_obj->tochdr.cdth_trk1)
#define FIRST_TRACK_NUM (_obj->tochdr.cdth_trk0)

/* reader */

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  
  enum {
    _AM_NONE,
    _AM_SUN_CTRL_ATAPI,
    _AM_SUN_CTRL_SCSI
#if FINISHED
    _AM_READ_CD,
    _AM_READ_10
#endif
  } access_mode;


  /* Track information */
  struct cdrom_tochdr    tochdr;
  struct cdrom_tocentry  tocent[100];    /* entry info for each track */

} _img_private_t;

/*!
  Initialize CD device.
 */
static bool
_cdio_init (_img_private_t *_obj)
{

  if (!cdio_generic_init(_obj)) return false;
  
  _obj->access_mode = _AM_SUN_CTRL_SCSI;    

  return true;
}

/*!
   Reads audio sectors from CD device into data starting from lsn.
   Returns 0 if no error. 

   May have to check size of nblocks. There may be a limit that
   can be read in one go, e.g. 25 blocks.
*/

static int
_cdio_read_audio_sectors (void *env, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;
  struct cdrom_cdda cdda;

  _img_private_t *_obj = env;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0   = from_bcd8(_msf.m);
  msf->cdmsf_sec0   = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);
  
  if (_obj->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (_obj->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (_obj->gen.ioctls_debugged < 75 
      || (_obj->gen.ioctls_debugged < (30 * 75)  
	  && _obj->gen.ioctls_debugged % 75 == 0)
      || _obj->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %d", lsn);
  
  _obj->gen.ioctls_debugged++;
  
  cdda.cdda_addr   = lsn;
  cdda.cdda_length = nblocks;
  cdda.cdda_data   = (caddr_t) data;
  if (ioctl (_obj->gen.fd, CDROMCDDA, &cdda) == -1) {
    perror ("ioctl(..,CDROMCDDA,..)");
	return 1;
	/* exit (EXIT_FAILURE); */
  }
  memcpy (data, buf, CDIO_CD_FRAMESIZE_RAW);
  
  return 0;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sector (void *env, void *data, lsn_t lsn, 
			 bool b_form2)
{

  char buf[M2RAW_SECTOR_SIZE] = { 0, };
#if FIXED
  do something here. 
#else
  if (0 > cdio_generic_lseek(env, CDIO_CD_FRAMESIZE*lsn, SEEK_SET))
    return -1;
  if (0 > cdio_generic_read(env, buf, CDIO_CD_FRAMESIZE))
    return -1;
  memcpy (data, buf, b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
#endif
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sectors (void *env, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *_obj = env;
  unsigned int i;
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
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sector (void *env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;
  int offset = 0;
  struct cdrom_cdxa cd_read;

  _img_private_t *_obj = env;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);
  
  if (_obj->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (_obj->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (_obj->gen.ioctls_debugged < 75 
      || (_obj->gen.ioctls_debugged < (30 * 75)  
	  && _obj->gen.ioctls_debugged % 75 == 0)
      || _obj->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %2.2d:%2.2d:%2.2d",
	       msf->cdmsf_min0, msf->cdmsf_sec0, msf->cdmsf_frame0);
  
  _obj->gen.ioctls_debugged++;
  
  /* Using CDROMXA ioctl will actually use the same uscsi command
   * as ATAPI, except we don't need to be root
   */      
  offset = CDIO_CD_XA_SYNC_HEADER;
  cd_read.cdxa_addr = lsn;
  cd_read.cdxa_data = buf;
  cd_read.cdxa_length = 1;
  cd_read.cdxa_format = CDROM_XA_SECTOR_DATA;
  if (ioctl (_obj->gen.fd, CDROMCDXA, &cd_read) == -1) {
    perror ("ioctl(..,CDROMCDXA,..)");
    return 1;
    /* exit (EXIT_FAILURE); */
  }
  
  if (b_form2)
    memcpy (data, buf + (offset-CDIO_CD_SUBHEADER_SIZE), M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)data), buf + offset, CDIO_CD_FRAMESIZE);
  
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sectors (void *env, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *_obj = env;
  unsigned int i;
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


/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_cdio_stat_size (void *env)
{
  _img_private_t *_obj = env;

  struct cdrom_tocentry tocent;
  uint32_t size;

  tocent.cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDROM_LBA;
  if (ioctl (_obj->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
    {
      perror ("ioctl(CDROMREADTOCENTRY)");
      exit (EXIT_FAILURE);
    }

  size = tocent.cdte_addr.lba;

  return size;
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" and "access-mode" are valid keys.
  "source" sets the source device in I/O operations 
  "access-mode" sets the the method of CD access 

  0 is returned if no error was found, and nonzero if there as an error.
*/
static int
_cdio_set_arg (void *env, const char key[], const char value[])
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
      if (!strcmp(value, "ATAPI"))
	_obj->access_mode = _AM_SUN_CTRL_SCSI; /* force ATAPI to be SCSI */
      else if (!strcmp(value, "SCSI"))
	_obj->access_mode = _AM_SUN_CTRL_SCSI;
      else
	cdio_warn ("unknown access type: %s. ignored.", value);
    }
  else 
    return -1;

  return 0;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
static bool
_cdio_read_toc (_img_private_t *_obj) 
{
  int i;

  /* read TOC header */
  if ( ioctl(_obj->gen.fd, CDROMREADTOCHDR, &_obj->tochdr) == -1 ) {
    cdio_error("%s: %s\n", 
            "error in ioctl CDROMREADTOCHDR", strerror(errno));
    return false;
  }

  /* read individual tracks */
  for (i=_obj->tochdr.cdth_trk0; i<=_obj->tochdr.cdth_trk1; i++) {
    _obj->tocent[i-1].cdte_track = i;
    _obj->tocent[i-1].cdte_format = CDROM_MSF;
    if ( ioctl(_obj->gen.fd, CDROMREADTOCENTRY, &_obj->tocent[i-1]) == -1 ) {
      cdio_error("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }
  }

  /* read the lead-out track */
  _obj->tocent[_obj->tochdr.cdth_trk1].cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  _obj->tocent[_obj->tochdr.cdth_trk1].cdte_format = CDROM_MSF;

  if (ioctl(_obj->gen.fd, CDROMREADTOCENTRY, 
	    &_obj->tocent[_obj->tochdr.cdth_trk1]) == -1 ) {
    cdio_error("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
    return false;
  }

  _obj->gen.toc_init = true;
  return true;
}

/*!
  Eject media in CD drive. If successful, as a side effect we 
  also free obj.
 */
static int 
_cdio_eject_media (void *env) {

  _img_private_t *_obj = env;
  int ret;

  close(_obj->gen.fd);
  _obj->gen.fd = -1;
  if (_obj->gen.fd > -1) {
    if ((ret = ioctl(_obj->gen.fd, CDROMEJECT)) != 0) {
      cdio_generic_free((void *) _obj);
      cdio_error ("CDROMEJECT failed: %s\n", strerror(errno));
      return 1;
    } else {
      return 0;
    }
  }
  return 2;
}


static void *
_cdio_malloc_and_zero(size_t size) {
  void *ptr;

  if( !size ) size++;
    
  if((ptr = malloc(size)) == NULL) {
    cdio_error("malloc() failed: %s", strerror(errno));
    return NULL;
  }

  memset(ptr, 0, size);
  return ptr;
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
  } else if (!strcmp (key, "access-mode")) {
    switch (_obj->access_mode) {
    case _AM_SUN_CTRL_ATAPI:
      return "ATAPI";
    case _AM_SUN_CTRL_SCSI:
      return "SCSI";
    case _AM_NONE:
      return "no access method";
    }
  } 
  return NULL;
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char *
cdio_get_default_device_solaris(void)
{
  char *volume_device;
  char *volume_name;
  char *volume_action;
  char *device;
  struct stat stb;

  if ((volume_device = getenv("VOLUME_DEVICE")) != NULL &&
      (volume_name   = getenv("VOLUME_NAME"))   != NULL &&
      (volume_action = getenv("VOLUME_ACTION")) != NULL &&
      strcmp(volume_action, "insert") == 0) {

    device = _cdio_malloc_and_zero(strlen(volume_device) 
				  + strlen(volume_name) + 2);
    if (device == NULL)
      return strdup(DEFAULT_CDIO_DEVICE);
    sprintf(device, "%s/%s", volume_device, volume_name);
    if (stat(device, &stb) != 0 || !S_ISCHR(stb.st_mode)) {
      free(device);
      return strdup(DEFAULT_CDIO_DEVICE);
    }
    return device;
  }
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *env) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

  return FIRST_TRACK_NUM;
}

/*!
  Return the number of tracks in the current medium.
*/
static track_t
_cdio_get_num_tracks(void *env) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

  return TOTAL_TRACKS;
}

/*!  
  Get format of track. 
*/
static track_format_t
_cdio_get_track_format(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.init) _cdio_init(_obj);
  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

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
_cdio_get_track_green(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.init) _cdio_init(_obj);
  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

  if (track_num > TOTAL_TRACKS+1 || track_num == 0)
    return false;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cdinfo for a while.
   */
  return ((_obj->tocent[track_num-1].cdte_ctrl & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no entry.
*/
static bool
_cdio_get_track_msf(void *env, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = env;

  if (NULL == msf) return false;

  if (!_obj->gen.init) _cdio_init(_obj);
  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

  if (track_num > TOTAL_TRACKS+1 || track_num == 0) {
    return false;
  } else {
    struct cdrom_tocentry *msf0 = &_obj->tocent[track_num-1];
    msf->m = to_bcd8(msf0->cdte_addr.msf.minute);
    msf->s = to_bcd8(msf0->cdte_addr.msf.second);
    msf->f = to_bcd8(msf0->cdte_addr.msf.frame);
    return true;
  }
}

#else 
/*!
  Return a string containing the default VCD device if none is specified.
 */
char *
cdio_get_default_device_solaris(void)
{
  return strdup(DEFAULT_CDIO_DEVICE);
}
#endif /* HAVE_SOLARIS_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_solaris (void)
{
#ifndef HAVE_SOLARIS_CDROM
  return NULL;
#else
  char **drives = NULL;
  unsigned int num_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  glob("/vol/dev/aliases/cdrom*", GLOB_DOOFFS, NULL, &globbuf);
  for (i=0; i<globbuf.gl_pathc; i++) {
    cdio_add_device_list(&drives, globbuf.gl_pathv[i], &num_files);
  }
  globfree(&globbuf);
#else
  cdio_add_device_list(&drives, DEFAULT_CDIO_DEVICE, &num_files);
#endif /*HAVE_GLOB_H*/
  cdio_add_device_list(&drives, NULL, &num_files);
  return drives;
#endif /*HAVE_SOLARIS_CDROM*/
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_solaris (const char *source_name)
{

#ifdef HAVE_SOLARIS_CDROM
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = cdio_generic_free,
    .get_arg            = _cdio_get_arg,
    .get_devices        = cdio_get_devices_solaris,
    .get_default_device = cdio_get_default_device_solaris,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_mcn            = NULL,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = NULL, /* This could be implemented if need be. */
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = cdio_generic_lseek,
    .read               = cdio_generic_read,
    .read_audio_sectors = _cdio_read_audio_sectors,
    .read_mode1_sector  = _cdio_read_mode1_sector,
    .read_mode1_sectors = _cdio_read_mode1_sectors,
    .read_mode2_sector  = _cdio_read_mode2_sector,
    .read_mode2_sectors = _cdio_read_mode2_sectors,
    .stat_size          = _cdio_stat_size,
    .set_arg            = _cdio_set_arg
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->gen.fd         = -1;

  _cdio_set_arg(_data, "source", (NULL == source_name) 
		? DEFAULT_CDIO_DEVICE: source_name);

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (_cdio_init(_data))
    return ret;
  else {
    cdio_generic_free (_data);
    return NULL;
  }

#else 
  return NULL;
#endif /* HAVE_SOLARIS_CDROM */

}

bool
cdio_have_solaris (void)
{
#ifdef HAVE_SOLARIS_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_SOLARIS_CDROM */
}

