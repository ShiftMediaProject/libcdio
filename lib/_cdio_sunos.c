/*
    $Id: _cdio_sunos.c,v 1.43 2004/07/08 05:19:27 rocky Exp $

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

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/scsi_mmc.h>
#include "cdio_assert.h"
#include "cdio_private.h"

#define DEFAULT_CDIO_DEVICE "/vol/dev/aliases/cdrom0"

#ifdef HAVE_SOLARIS_CDROM

static const char _rcsid[] = "$Id: _cdio_sunos.c,v 1.43 2004/07/08 05:19:27 rocky Exp $";

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

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

#define TOTAL_TRACKS    (env->tochdr.cdth_trk1)
#define FIRST_TRACK_NUM (env->tochdr.cdth_trk0)

/* reader */

typedef  enum {
    _AM_NONE,
    _AM_SUN_CTRL_ATAPI,
    _AM_SUN_CTRL_SCSI
#if FINISHED
    _AM_READ_CD,
    _AM_READ_10
#endif
} access_mode_t;


typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  
  access_mode_t access_mode;

  /* Track information */
  struct cdrom_tochdr    tochdr;
  /* Entry info for each track, add 1 for leadout. */
  struct cdrom_tocentry  tocent[CDIO_CD_MAX_TRACKS+1]; 

} _img_private_t;

static access_mode_t 
str_to_access_mode_sunos(const char *psz_access_mode) 
{
  const access_mode_t default_access_mode = _AM_SUN_CTRL_SCSI;

  if (NULL==psz_access_mode) return default_access_mode;
  
  if (!strcmp(psz_access_mode, "ATAPI"))
    return _AM_SUN_CTRL_SCSI; /* force ATAPI to be SCSI */
  else if (!strcmp(psz_access_mode, "SCSI"))
    return _AM_SUN_CTRL_SCSI;
  else {
    cdio_warn ("unknown access type: %s. Default SCSI used.", 
	       psz_access_mode);
    return default_access_mode;
  }
}


/*!
  Initialize CD device.
 */
static bool
_cdio_init (_img_private_t *env)
{

  if (!cdio_generic_init(env)) return false;
  
  env->access_mode = _AM_SUN_CTRL_SCSI;    

  return true;
}

/*!
   Reads audio sectors from CD device into data starting from lsn.
   Returns 0 if no error. 

   May have to check size of nblocks. There may be a limit that
   can be read in one go, e.g. 25 blocks.
*/

static int
_cdio_read_audio_sectors (void *user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;
  struct cdrom_cdda cdda;

  _img_private_t *env = user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0   = from_bcd8(_msf.m);
  msf->cdmsf_sec0   = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);
  
  if (env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (env->gen.ioctls_debugged < 75 
      || (env->gen.ioctls_debugged < (30 * 75)  
	  && env->gen.ioctls_debugged % 75 == 0)
      || env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %d", lsn);
  
  env->gen.ioctls_debugged++;
  
  cdda.cdda_addr   = lsn;
  cdda.cdda_length = nblocks;
  cdda.cdda_data   = (caddr_t) data;
  if (ioctl (env->gen.fd, CDROMCDDA, &cdda) == -1) {
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

#if FIXED
  do something here. 
#else
  return cdio_generic_read_form1_sector(env, data, lsn);
#endif
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sectors (void *user_data, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _cdio_read_mode1_sector (env, 
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
_cdio_read_mode2_sector (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;
  int offset = 0;
  struct cdrom_cdxa cd_read;

  _img_private_t *env = user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);
  
  if (env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");
  
  if (env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (env->gen.ioctls_debugged < 75 
      || (env->gen.ioctls_debugged < (30 * 75)  
	  && env->gen.ioctls_debugged % 75 == 0)
      || env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %2.2d:%2.2d:%2.2d",
	       msf->cdmsf_min0, msf->cdmsf_sec0, msf->cdmsf_frame0);
  
  env->gen.ioctls_debugged++;
  
  /* Using CDROMXA ioctl will actually use the same uscsi command
   * as ATAPI, except we don't need to be root
   */      
  offset = CDIO_CD_XA_SYNC_HEADER;
  cd_read.cdxa_addr = lsn;
  cd_read.cdxa_data = buf;
  cd_read.cdxa_length = 1;
  cd_read.cdxa_format = CDROM_XA_SECTOR_DATA;
  if (ioctl (env->gen.fd, CDROMCDXA, &cd_read) == -1) {
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
_cdio_read_mode2_sectors (void *user_data, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _cdio_read_mode2_sector (env, 
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
_cdio_stat_size (void *user_data)
{
  _img_private_t *env = user_data;

  struct cdrom_tocentry tocent;
  uint32_t size;

  tocent.cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDROM_LBA;
  if (ioctl (env->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
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
_set_arg_solaris (void *user_data, const char key[], const char value[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source"))
    {
      if (!value)
	return -2;

      free (env->gen.source_name);
      
      env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      env->access_mode = str_to_access_mode_sunos(key);
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
_cdio_read_toc (_img_private_t *env) 
{
  int i;

  /* read TOC header */
  if ( ioctl(env->gen.fd, CDROMREADTOCHDR, &env->tochdr) == -1 ) {
    cdio_warn("%s: %s\n", 
            "error in ioctl CDROMREADTOCHDR", strerror(errno));
    return false;
  }

  /* read individual tracks */
  for (i=env->tochdr.cdth_trk0; i<=env->tochdr.cdth_trk1; i++) {
    env->tocent[i-1].cdte_track = i;
    env->tocent[i-1].cdte_format = CDROM_MSF;
    if ( ioctl(env->gen.fd, CDROMREADTOCENTRY, &env->tocent[i-1]) == -1 ) {
      cdio_warn("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }
  }

  /* read the lead-out track */
  env->tocent[env->tochdr.cdth_trk1].cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  env->tocent[env->tochdr.cdth_trk1].cdte_format = CDROM_MSF;

  if (ioctl(env->gen.fd, CDROMREADTOCENTRY, 
	    &env->tocent[env->tochdr.cdth_trk1]) == -1 ) {
    cdio_warn("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
    return false;
  }

  env->gen.toc_init = true;
  return true;
}

/*!
  Eject media in CD drive. If successful, as a side effect we 
  also free obj.
 */
static int 
_cdio_eject_media (void *user_data) {

  _img_private_t *env = user_data;
  int ret;

  close(env->gen.fd);
  env->gen.fd = -1;
  if (env->gen.fd > -1) {
    if ((ret = ioctl(env->gen.fd, CDROMEJECT)) != 0) {
      cdio_generic_free((void *) env);
      cdio_warn ("CDROMEJECT failed: %s\n", strerror(errno));
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
    cdio_warn("malloc() failed: %s", strerror(errno));
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
  _img_private_t *env = user_data;

  if (!strcmp (key, "source")) {
    return env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (env->access_mode) {
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
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static cdio_drive_cap_t
_cdio_get_drive_cap_solaris (const void *user_data)
{
#if 1
  struct uscsi_cmd my_cmd;
  int32_t i_drivetype = 0;
  uint8_t buf[192] = { 0, };
  unsigned char my_rq_buf[26];
  unsigned char my_scsi_cdb[6];
  const _img_private_t *env = user_data;
  int rc;
  
  memset(&my_scsi_cdb, 0, sizeof(my_scsi_cdb));
  memset(&my_rq_buf,   0, sizeof(my_rq_buf));
  
  /* Initialize my_scsi_cdb as a Mode Select(6) */
  CDIO_MMC_SET_COMMAND(my_scsi_cdb, CDIO_MMC_GPCMD_MODE_SENSE);
  my_scsi_cdb[1] = 0x0;  
  my_scsi_cdb[2] = CDIO_MMC_CAPABILITIES_PAGE; 
  my_scsi_cdb[3] = 0;    /* Not used */
  my_scsi_cdb[4] = 128; 
  my_scsi_cdb[5] = 0;    /* Not used */
  
  my_cmd.uscsi_flags = (USCSI_READ|USCSI_RQENABLE);  /* We're going get data */
  my_cmd.uscsi_timeout = 15;             /* Allow 15 seconds for completion */
  my_cmd.uscsi_cdb = my_scsi_cdb;        /* We'll be using the array above for the CDB */
  my_cmd.uscsi_bufaddr = buf;   /* The block and page data is stored here */
  my_cmd.uscsi_buflen = sizeof(buf); 
  my_cmd.uscsi_cdblen = 6;               /* The CDB is 6 bytes long */
  my_cmd.uscsi_rqlen =  24;              /* The request sense buffer (only valid on a check condition) is 26 bytes long */
  my_cmd.uscsi_rqbuf = my_rq_buf;        /* Pointer to the request sense buffer */
  
  rc = ioctl(env->gen.fd, USCSICMD, &my_cmd);
  if(rc == 0) {
    unsigned int n=buf[3]+4;
    i_drivetype |= cdio_get_drive_cap_mmc(&(buf[n]));
  } else {
    cdio_info("%s: %s\n", 
            "error in ioctl USCSICMD MODE_SELECT", strerror(errno));
    i_drivetype = CDIO_DRIVE_CAP_CD_AUDIO | CDIO_DRIVE_CAP_UNKNOWN;
  }
  return i_drivetype;

#else
  return CDIO_DRIVE_CAP_UNKNOWN | CDIO_DRIVE_CAP_CD_AUDIO 
    | CDIO_DRIVE_CAP_CD_RW;
#endif
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
_cdio_get_mcn_solaris (const void *user_data)
{
#if 0
  struct uscsi_cmd my_cmd;
  char buf[192] = { 0, };
  unsigned char my_rq_buf[32];
  unsigned char my_scsi_cdb[6];
  const _img_private_t *env = user_data;
  int rc;
  
  memset(&my_scsi_cdb, 0, sizeof(my_scsi_cdb));
  memset(&my_rq_buf,   0, sizeof(my_rq_buf));
  
  CDIO_MMC_SET_COMMAND(my_scsi_cdb, CDIO_MMC_GPCMD_READ_SUBCHANNEL);
  my_scsi_cdb[1] = 0x0;  
  my_scsi_cdb[2] = 0x40; 
  my_scsi_cdb[3] = 02;    /* Give media catalog number. */
  my_scsi_cdb[4] = 0;    /* Not used */
  my_scsi_cdb[5] = 0;    /* Not used */
  my_scsi_cdb[6] = 0;    /* Not used */
  my_scsi_cdb[7] = 0;    /* Not used */
  my_scsi_cdb[8] = 28; 
  my_scsi_cdb[9] = 0;    /* Not used */
  
  my_cmd.uscsi_flags = (USCSI_READ|USCSI_RQENABLE);  /* We're going get data */
  my_cmd.uscsi_timeout = 15;             /* Allow 15 seconds for completion */
  my_cmd.uscsi_cdb = my_scsi_cdb;        /* We'll be using the array above for the CDB */
  my_cmd.uscsi_bufaddr = buf;   /* The block and page data is stored here */
  my_cmd.uscsi_buflen = sizeof(buf); 
  my_cmd.uscsi_cdblen = 6;               /* The CDB is 6 bytes long */
  my_cmd.uscsi_rqlen =  24;              /* The request sense buffer (only valid on a check condition) is 26 bytes long */
  my_cmd.uscsi_rqbuf = my_rq_buf;        /* Pointer to the request sense buffer */
  
  rc = ioctl(env->gen.fd, USCSICMD, &my_cmd);
  if(rc == 0) {
    return strdup(&buf[9]);
  }
  return NULL;
#else
  return NULL;
#endif
}


/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *user_data) 
{
  _img_private_t *env = user_data;
  
  if (!env->gen.toc_init) _cdio_read_toc (env) ;

  return FIRST_TRACK_NUM;
}


/*!
  Return the number of tracks in the current medium.
*/
static track_t
_cdio_get_num_tracks(void *user_data)
{
  _img_private_t *env = user_data;
  
  return TOTAL_TRACKS;
}

/*!  
  Get format of track. 
*/
static track_format_t
_cdio_get_track_format(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  
  if ( (i_track > TOTAL_TRACKS+FIRST_TRACK_NUM) || i_track < FIRST_TRACK_NUM)
    return TRACK_FORMAT_ERROR;

  i_track -= FIRST_TRACK_NUM;

  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
   */
  if (env->tocent[i_track].cdte_ctrl & CDROM_DATA_TRACK) {
    if (env->tocent[i_track].cdte_format == CDIO_CDROM_CDI_TRACK)
      return TRACK_FORMAT_CDI;
    else if (env->tocent[i_track].cdte_format == CDIO_CDROM_XA_TRACK) 
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
_cdio_get_track_green(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  
  if (!env->gen.init) _cdio_init(env);
  if (!env->gen.toc_init) _cdio_read_toc (env) ;

  if (i_track > TOTAL_TRACKS+FIRST_TRACK_NUM || i_track < FIRST_TRACK_NUM)
    return false;

  i_track -= FIRST_TRACK_NUM;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cd-info for a while.
   */
  return ((env->tocent[i_track].cdte_ctrl & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers usually start at something 
  greater than 0, usually 1.

  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no entry.
*/
static bool
_cdio_get_track_msf(void *user_data, track_t i_track, msf_t *msf)
{
  _img_private_t *env = user_data;

  if (NULL == msf) return false;

  if (!env->gen.init) _cdio_init(env);
  if (!env->gen.toc_init) _cdio_read_toc (env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) 
    i_track = TOTAL_TRACKS + FIRST_TRACK_NUM;

  if (i_track > (TOTAL_TRACKS+FIRST_TRACK_NUM) || i_track < FIRST_TRACK_NUM) {
    return false;
  } else {
    struct cdrom_tocentry *msf0 = &env->tocent[i_track-1];
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
cdio_open_solaris (const char *psz_source_name)
{
  return cdio_open_am_solaris(psz_source_name, NULL);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_solaris (const char *psz_orig_source, const char *access_mode)
{

#ifdef HAVE_SOLARIS_CDROM
  CdIo *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = cdio_generic_free,
    .get_arg            = _cdio_get_arg,
    .get_devices        = cdio_get_devices_solaris,
    .get_default_device = cdio_get_default_device_solaris,
    .get_drive_cap      = _cdio_get_drive_cap_solaris,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_mcn            = _cdio_get_mcn_solaris,
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
    .set_arg            = _set_arg_solaris
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));

  _data->access_mode    = _AM_SUN_CTRL_SCSI;
  _data->gen.init       = false;
  _data->gen.fd         = -1;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_solaris();
    if (NULL == psz_source) return NULL;
    _set_arg_solaris(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_solaris(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is a not a device", psz_orig_source);
#endif
      return NULL;
    }
  }

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
