/*
    $Id: _cdio_sunos.c,v 1.1 2003/03/24 19:01:09 rocky Exp $

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "cdio_assert.h"
#include "cdio_private.h"
#include "logging.h"
#include "sector.h"
#include "util.h"

#ifdef HAVE_SOLARIS_CDROM

static const char _rcsid[] = "$Id: _cdio_sunos.c,v 1.1 2003/03/24 19:01:09 rocky Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define DEFAULT_CDIO_DEVICE "/vol/dev/aliases/cdrom0"

#define TOTAL_TRACKS    (_obj->tochdr.cdth_trk1)
#define FIRST_TRACK_NUM (_obj->tochdr.cdth_trk0)

/* reader */

typedef struct {
  int fd;

  int ioctls_debugged; /* for debugging */

  enum {
    _AM_NONE,
    _AM_SUN_CTRL_ATAPI,
    _AM_SUN_CTRL_SCSI
#if FINISHED
    _AM_READ_CD,
    _AM_READ_10
#endif
  } access_mode;

  char *source_name;
  
  bool init;

  /* Track information */
  bool toc_init;                         /* if true, info below is valid. */
  struct cdrom_tochdr    tochdr;
  struct cdrom_tocentry  tocent[100];    /* entry info for each track */

} _img_private_t;

/*!
  Initialize CD device.
 */
static bool
_cdio_init (_img_private_t *_obj)
{

  struct dk_cinfo cinfo;

  _obj->fd = open (_obj->source_name, O_RDONLY, 0);

  if (_obj->fd < 0)
    {
      cdio_error ("open (%s): %s", _obj->source_name, strerror (errno));
      return false;
    }

  /*
   * CDROMCDXA/CDROMREADMODE2 are broken on IDE/ATAPI devices.
   * Try to send MMC3 SCSI commands via the uscsi interface on
   * ATAPI devices.
   */
  if ( ioctl(_obj->fd, DKIOCINFO, &cinfo) == 0
       && ((strcmp(cinfo.dki_cname, "ide") == 0) 
	   || (strncmp(cinfo.dki_cname, "pci", 3) == 0)) ) {
      _obj->access_mode = _AM_SUN_CTRL_ATAPI;
  } else {
      _obj->access_mode = _AM_SUN_CTRL_SCSI;    
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

  if (_obj->fd >= 0)
    close (_obj->fd);

  free (_obj);
}

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
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
  
  switch (_obj->access_mode)
    {
    case _AM_NONE:
      cdio_error ("No way to read CD mode2.");
      return 1;
      break;
      
    case _AM_SUN_CTRL_SCSI:
      if (ioctl (_obj->fd, CDROMREADMODE2, &buf) == -1) {
	perror ("ioctl(..,CDROMREADMODE2,..)");
	return 1;
	/* exit (EXIT_FAILURE); */
      }
      break;
      
    case _AM_SUN_CTRL_ATAPI:
      {
	struct uscsi_cmd sc;
	union scsi_cdb cdb;
	int blocks = 1;
	int sector_type;
	int sync, header_code, user_data, edc_ecc, error_field;
	int sub_channel;
	
	sector_type = 0;		/* all types */
	/*sector_type = 1;*/	/* CD-DA */
	/*sector_type = 2;*/	/* mode1 */
	/*sector_type = 3;*/	/* mode2 */
	/*sector_type = 4;*/	/* mode2/form1 */
	/*sector_type = 5;*/	/* mode2/form2 */
	sync = 0;
	header_code = 2;
	user_data = 1;
	edc_ecc = 0;
	error_field = 0;
	sub_channel = 0;
	
	memset(&cdb, 0, sizeof(cdb));
	memset(&sc, 0, sizeof(sc));
	cdb.scc_cmd = 0xBE;
	cdb.cdb_opaque[1] = (sector_type) << 2;
	cdb.cdb_opaque[2] = (lsn >> 24) & 0xff;
	cdb.cdb_opaque[3] = (lsn >> 16) & 0xff;
	cdb.cdb_opaque[4] = (lsn >>  8) & 0xff;
	cdb.cdb_opaque[5] =  lsn & 0xff;
	cdb.cdb_opaque[6] = (blocks >> 16) & 0xff;
	cdb.cdb_opaque[7] = (blocks >>  8) & 0xff;
	cdb.cdb_opaque[8] =  blocks & 0xff;
	cdb.cdb_opaque[9] = (sync << 7) |
	  (header_code << 5) |
	  (user_data << 4) |
	  (edc_ecc << 3) |
	  (error_field << 1);
	cdb.cdb_opaque[10] = sub_channel;
	
	sc.uscsi_cdb = (caddr_t)&cdb;
	sc.uscsi_cdblen = 12;
	sc.uscsi_bufaddr = (caddr_t) buf;
	sc.uscsi_buflen = M2RAW_SECTOR_SIZE;
	sc.uscsi_flags = USCSI_ISOLATE | USCSI_READ;
	sc.uscsi_timeout = 20;
	if (ioctl(_obj->fd, USCSICMD, &sc)) {
	  perror("USCSICMD: READ CD");
	  return 1;
	}
	if (sc.uscsi_status) {
	  cdio_error("SCSI command failed with status %d\n", 
		    sc.uscsi_status);
	  return 1;
	}
	break;
      }
    }
  
  if (mode2_form2)
    memcpy (data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)data), buf + 8, M2F1_SECTOR_SIZE);
  
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
      if (!strcmp(value, "ATAPI"))
	_obj->access_mode = _AM_SUN_CTRL_ATAPI;
      else if (!strcmp(value, "SCSI"))
	_obj->access_mode = _AM_SUN_CTRL_SCSI;
      else
	cdio_error ("unknown access type: %s. ignored.", value);
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
  if ( ioctl(_obj->fd, CDROMREADTOCHDR, &_obj->tochdr) == -1 ) {
    cdio_error("%s: %s\n", 
            "error in ioctl CDROMREADTOCHDR", strerror(errno));
    return false;
  }

  /* read individual tracks */
  for (i=_obj->tochdr.cdth_trk0; i<=_obj->tochdr.cdth_trk1; i++) {
    _obj->tocent[i-1].cdte_track = i;
    _obj->tocent[i-1].cdte_format = CDROM_MSF;
    if ( ioctl(_obj->fd, CDROMREADTOCENTRY, &_obj->tocent[i-1]) == -1 ) {
      cdio_error("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }
  }

  /* read the lead-out track */
  _obj->tocent[_obj->tochdr.cdth_trk1].cdte_track = CDROM_LEADOUT;
  _obj->tocent[_obj->tochdr.cdth_trk1].cdte_format = CDROM_MSF;

  if (ioctl(_obj->fd, CDROMREADTOCENTRY, 
	    &_obj->tocent[_obj->tochdr.cdth_trk1]) == -1 ) {
    cdio_error("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
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
  int ret;

  if (_obj->fd > -1) {
    if ((ret = ioctl(_obj->fd, CDROMEJECT)) != 0) {
      _cdio_free((void *) _obj);
      cdio_error ("CDROMEJECT failed: %s\n", strerror(errno));
      return 1;
    } else {
      _cdio_free((void *) _obj);
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
_cdio_get_arg (void *user_data, const char key[])
{
  _img_private_t *_obj = user_data;

  if (!strcmp (key, "source")) {
    return _obj->source_name;
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
  Return a string containing the default VCD device if none is specified.
 */
static char *
_cdio_get_default_device(void)
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
_cdio_get_first_track_num(void *user_data) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return FIRST_TRACK_NUM;
}

/*!
  Return the number of tracks in the current medium.
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
  
  if (!_obj->init) _cdio_init(_obj);
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
  
  if (!_obj->init) _cdio_init(_obj);
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
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  NULL is returned if there is no entry.
*/
static bool
_cdio_get_track_msf(void *user_data, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = user_data;

  if (NULL == msf) return false;

  if (!_obj->init) _cdio_init(_obj);
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

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

#endif /* HAVE_SOLARIS_CDROM */

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
    .free               = _cdio_free,
    .get_arg            = _cdio_get_arg,
    .get_default_device = _cdio_get_default_device,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_msf      = _cdio_get_track_msf,
    .read_mode2_sector  = _read_mode2_sector,
    .read_mode2_sectors = _read_mode2_sectors,
    .stat_size          = _cdio_stat_size,
    .set_arg            = _cdio_set_arg
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
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

