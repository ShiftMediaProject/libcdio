/*
    $Id: _cdio_linux.c,v 1.23 2003/09/27 23:29:29 rocky Exp $

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

/* This file contains Linux-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_linux.c,v 1.23 2003/09/27 23:29:29 rocky Exp $";

#include <string.h>

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"
#include "scsi_mmc.h"

#ifdef HAVE_LINUX_CDROM

#if defined(HAVE_LINUX_VERSION_H)
# include <linux/version.h>
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,2,16)
#   define __CDIO_LINUXCD_BUILD
# else
#  error "You need a kernel greater than 2.2.16 to have CDROM support"
# endif
#else 
#  error "You need <linux/version.h> to have CDROM support"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <mntent.h>

#include <linux/cdrom.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <scsi/scsi_ioctl.h>
#include <sys/mount.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define TOTAL_TRACKS    (_obj->tochdr.cdth_trk1)
#define FIRST_TRACK_NUM (_obj->tochdr.cdth_trk0)

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  enum {
    _AM_NONE,
    _AM_IOCTL,
    _AM_READ_CD,
    _AM_READ_10
  } access_mode;

  /* Track information */
  struct cdrom_tochdr    tochdr;
  struct cdrom_tocentry  tocent[100];    /* entry info for each track */

} _img_private_t;

/* Some ioctl() errno values which occur when the tray is empty */
#define ERRNO_TRAYEMPTY(errno)	\
	((errno == EIO) || (errno == ENOENT) || (errno == EINVAL))


/* Check a drive to see if it is a CD-ROM 
   Return 1 if a CD-ROM. 0 if it exists but isn't a CD-ROM drive
   and -1 if no device exists .
*/
static int
cdio_is_cdrom(char *drive, char *mnttype)
{
  bool is_cd=false;
  int cdfd;
  struct cdrom_tochdr    tochdr;
  
  /* If it doesn't exist, return -1 */
  if ( !cdio_is_device_quiet_generic(drive) ) {
    return(false);
  }
  
  /* If it does exist, verify that it's an available CD-ROM */
  cdfd = open(drive, (O_RDONLY|O_EXCL|O_NONBLOCK), 0);
  if ( cdfd >= 0 ) {
    if ( ioctl(cdfd, CDROMREADTOCHDR, &tochdr) != -1 ) {
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

static char *
cdio_check_mounts(const char *mtab)
{
  FILE *mntfp;
  struct mntent *mntent;
  
  mntfp = setmntent(mtab, "r");
  if ( mntfp != NULL ) {
    char *tmp;
    char *mnt_type;
    char *mnt_dev;
    
    while ( (mntent=getmntent(mntfp)) != NULL ) {
      mnt_type = malloc(strlen(mntent->mnt_type) + 1);
      if (mnt_type == NULL)
	continue;  /* maybe you'll get lucky next time. */
      
      mnt_dev = malloc(strlen(mntent->mnt_fsname) + 1);
      if (mnt_dev == NULL) {
	free(mnt_type);
	continue;
      }
      
      strcpy(mnt_type, mntent->mnt_type);
      strcpy(mnt_dev, mntent->mnt_fsname);
      
      /* Handle "supermount" filesystem mounts */
      if ( strcmp(mnt_type, "supermount") == 0 ) {
	tmp = strstr(mntent->mnt_opts, "fs=");
	if ( tmp ) {
	  free(mnt_type);
	  mnt_type = strdup(tmp + strlen("fs="));
	  if ( mnt_type ) {
	    tmp = strchr(mnt_type, ',');
	    if ( tmp ) {
	      *tmp = '\0';
	    }
	  }
	}
	tmp = strstr(mntent->mnt_opts, "dev=");
	if ( tmp ) {
	  free(mnt_dev);
	  mnt_dev = strdup(tmp + strlen("dev="));
	  if ( mnt_dev ) {
	    tmp = strchr(mnt_dev, ',');
	    if ( tmp ) {
	      *tmp = '\0';
	    }
	  }
	}
      }
      if ( strcmp(mnt_type, "iso9660") == 0 ) {
	if (cdio_is_cdrom(mnt_dev, mnt_type) > 0) {
	  free(mnt_dev);
	  free(mnt_type);
	  endmntent(mntfp);
	  return strdup(mnt_dev);
	}
      }
      free(mnt_dev);
      free(mnt_type);
    }
    endmntent(mntfp);
  }
  return NULL;
}

static int 
_set_bsize (int fd, unsigned int bsize)
{
  struct cdrom_generic_command cgc;

  struct
  {
    uint8_t reserved1;
    uint8_t medium;
    uint8_t reserved2;
    uint8_t block_desc_length;
    uint8_t density;
    uint8_t number_of_blocks_hi;
    uint8_t number_of_blocks_med;
    uint8_t number_of_blocks_lo;
    uint8_t reserved3;
    uint8_t block_length_hi;
    uint8_t block_length_med;
    uint8_t block_length_lo;
  } mh;

  memset (&mh, 0, sizeof (mh));
  memset (&cgc, 0, sizeof (struct cdrom_generic_command));
  
  cgc.cmd[0] = 0x15;
  cgc.cmd[1] = 1 << 4;
  cgc.cmd[4] = 12;
  
  cgc.buflen = sizeof (mh);
  cgc.buffer = (void *) &mh;

  cgc.data_direction = CGC_DATA_WRITE;

  mh.block_desc_length = 0x08;
  mh.block_length_hi   = (bsize >> 16) & 0xff;
  mh.block_length_med  = (bsize >>  8) & 0xff;
  mh.block_length_lo   = (bsize >>  0) & 0xff;

  return ioctl (fd, CDROM_SEND_PACKET, &cgc);
}

/* Packet driver to read mode2 sectors. 
   Can read only up to 25 blocks.
*/
static int
_cdio_mmc_read_sectors (int fd, void *buf, lba_t lba, int sector_type, 
			unsigned int nblocks)
{
  typedef struct cdrom_generic_command cgc_t;
  cgc_t cgc;

  memset (&cgc, 0, sizeof (cgc_t));

  cgc.cmd[0] = CDIO_MMC_GPCMD_READ_CD;
  CDIO_MMC_SET_READ_TYPE  (cgc.cmd, sector_type);
  CDIO_MMC_SET_READ_LBA   (cgc.cmd, lba);
  CDIO_MMC_SET_READ_LENGTH(cgc.cmd, nblocks);
  CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(cgc.cmd, CDIO_MMC_MCSB_ALL_HEADERS);

  cgc.buflen = CDIO_CD_FRAMESIZE_RAW * nblocks; 
  cgc.buffer = buf;

#ifdef HAVE_LINUX_CDROM_TIMEOUT
  cgc.timeout = 500;
#endif
  cgc.data_direction = CGC_DATA_READ;

  return ioctl (fd, CDROM_SEND_PACKET, &cgc);

  return 0;
}

/* MMC driver to read audio sectors. 
   Can read only up to 25 blocks.
*/
static int
_cdio_read_audio_sectors (void *env, void *buf, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *_obj = env;
  return _cdio_mmc_read_sectors( _obj->gen.fd, buf, lsn, 
				 CDIO_MMC_READ_TYPE_CDDA, nblocks);
}

/* Packet driver to read mode2 sectors. 
   Can read only up to 25 blocks.
*/
static int
__read_packet_mode2_sectors (int fd, void *buf, lba_t lba, 
			     unsigned int nblocks, bool use_read_10)
{
  struct cdrom_generic_command cgc;

  memset (&cgc, 0, sizeof (struct cdrom_generic_command));

  cgc.cmd[0] = use_read_10 ? GPCMD_READ_10 : CDIO_MMC_GPCMD_READ_CD;

  CDIO_MMC_SET_READ_LBA(cgc.cmd, lba);
  CDIO_MMC_SET_READ_LENGTH(cgc.cmd, nblocks);

  if (!use_read_10)
    {
      cgc.cmd[1] = 0; /* sector size mode2 */

      cgc.cmd[9] = 0x58; /* 2336 mode2 */
    }

  cgc.buflen = 2336 * nblocks;
  cgc.buffer = buf;

#ifdef HAVE_LINUX_CDROM_TIMEOUT
  cgc.timeout = 500;
#endif
  cgc.data_direction = CGC_DATA_READ;

  if (use_read_10)
    {
      int retval;

      if ((retval = _set_bsize (fd, 2336)))
	return retval;

      if ((retval = ioctl (fd, CDROM_SEND_PACKET, &cgc)))
	{
	  _set_bsize (fd, 2048);
	  return retval;
	}

      if ((retval = _set_bsize (fd, 2048)))
	return retval;
    }
  else
    return ioctl (fd, CDROM_SEND_PACKET, &cgc);

  return 0;
}

static int
_read_packet_mode2_sectors (int fd, void *buf, lba_t lba, 
			    unsigned int nblocks, bool use_read_10)
{
  unsigned int l = 0;
  int retval = 0;

  while (nblocks > 0)
    {
      const unsigned nblocks2 = (nblocks > 25) ? 25 : nblocks;
      void *buf2 = ((char *)buf ) + (l * 2336);
      
      retval |= __read_packet_mode2_sectors (fd, buf2, lba + l, nblocks2, 
					     use_read_10);

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
_cdio_read_mode2_sector (void *env, void *data, lsn_t lsn, 
			 bool mode2_form2)
{
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *_obj = env;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);

 retry:
  switch (_obj->access_mode)
    {
    case _AM_NONE:
      cdio_error ("no way to read mode2");
      return 1;
      break;
      
    case _AM_IOCTL:
      if (ioctl (_obj->gen.fd, CDROMREADMODE2, &buf) == -1)
	{
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
      break;
      
    case _AM_READ_CD:
    case _AM_READ_10:
      if (_read_packet_mode2_sectors (_obj->gen.fd, buf, lsn, 1, 
				      (_obj->access_mode == _AM_READ_10)))
	{
	  perror ("ioctl()");
	  if (_obj->access_mode == _AM_READ_CD)
	    {
	      cdio_info ("READ_CD failed; switching to READ_10 mode...");
	      _obj->access_mode = _AM_READ_10;
	      goto retry;
	    }
	  else
	    {
	      cdio_info ("READ_10 failed; switching to ioctl(CDROMREADMODE2) mode...");
	      _obj->access_mode = _AM_IOCTL;
	      goto retry;
	    }
	  return 1;
	}
      break;
    }

  if (mode2_form2)
    memcpy (data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
  
  return 0;
}

/*!
   Reads a single audio sector from CD device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_audio_sector (void *env, void *data, lsn_t lsn)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *_obj = env;

  cdio_lsn_to_msf (lsn, &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);

 retry:
  switch (_obj->access_mode)
    {
    case _AM_NONE:
      cdio_error ("no way to read audio");
      return 1;
      break;

    case _AM_IOCTL:
      if (ioctl (_obj->gen.fd, CDROMREADRAW, &buf) == -1) {
	perror ("ioctl()");
	return 1;
	/* exit (EXIT_FAILURE); */
      }
      break;

    case _AM_READ_CD:
    case _AM_READ_10:
      if (_cdio_mmc_read_sectors (_obj->gen.fd, buf, lsn, 
				  CDIO_MMC_READ_TYPE_ANY, 1)) {
	perror ("ioctl()");
	if (_obj->access_mode == _AM_READ_CD) {
	  cdio_info ("READ_CD failed; switching to READ_10 mode...");
	  _obj->access_mode = _AM_READ_10;
	  goto retry;
	} else {
	  cdio_info ("READ_10 failed; switching to ioctl(CDROMREADAUDIO) mode...");
	  _obj->access_mode = _AM_IOCTL;
	  goto retry;
	}
	return 1;
      }
      break;
    }

  memcpy (data, buf, CDIO_CD_FRAMESIZE_RAW);
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sectors (void *env, void *data, lsn_t lsn, 
			  bool mode2_form2, unsigned int nblocks)
{
  _img_private_t *_obj = env;
  unsigned int i;
  int retval;

  for (i = 0; i < nblocks; i++) {
    if (mode2_form2) {
      if ( (retval = _cdio_read_mode2_sector (_obj, 
					      ((char *)data) 
					      + (M2RAW_SECTOR_SIZE * i),
					      lsn + i, true)) )
	return retval;
    } else {
      char buf[M2RAW_SECTOR_SIZE] = { 0, };
      if ( (retval = _cdio_read_mode2_sector (_obj, buf, lsn + i, true)) )
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
      if (!strcmp(value, "IOCTL"))
	_obj->access_mode = _AM_IOCTL;
      else if (!strcmp(value, "READ_CD"))
	_obj->access_mode = _AM_READ_CD;
      else if (!strcmp(value, "READ_10"))
	_obj->access_mode = _AM_READ_10;
      else {
	cdio_warn ("unknown access type: %s. ignored.", value);
      }
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

  /* read TOC header */
  if ( ioctl(_obj->gen.fd, CDROMREADTOCHDR, &_obj->tochdr) == -1 ) {
    cdio_error("%s: %s\n", 
            "error in ioctl CDROMREADTOCHDR", strerror(errno));
    return false;
  }

  /* read individual tracks */
  for (i= FIRST_TRACK_NUM; i<=TOTAL_TRACKS; i++) {
    _obj->tocent[i-1].cdte_track = i;
    _obj->tocent[i-1].cdte_format = CDROM_MSF;
    if ( ioctl(_obj->gen.fd, CDROMREADTOCENTRY, &_obj->tocent[i-1]) == -1 ) {
      cdio_error("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }
    /****
    struct cdrom_msf0 *msf= &_obj->tocent[i-1].cdte_addr.msf;
    
    fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	     i, msf->minute, msf->second, msf->frame);
    ****/

  }

  /* read the lead-out track */
  _obj->tocent[TOTAL_TRACKS].cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  _obj->tocent[TOTAL_TRACKS].cdte_format = CDROM_MSF;

  if (ioctl(_obj->gen.fd, CDROMREADTOCENTRY, 
	    &_obj->tocent[TOTAL_TRACKS]) == -1 ) {
    cdio_error("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
    return false;
  }

  /*
  struct cdrom_msf0 *msf= &_obj->tocent[TOTAL_TRACKS].cdte_addr.msf;

  fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	   i, msf->minute, msf->second, msf->frame);
  */

  _obj->gen.toc_init = true;
  return true;
}

/*
 * Eject using SCSI commands. Return 1 if successful, 0 otherwise.
 */
static int 
_cdio_eject_scsi(int fd)
{
  int status;
  struct sdata {
    int  inlen;
    int  outlen;
    char cmd[256];
  } scsi_cmd;
  
  scsi_cmd.inlen	= 0;
  scsi_cmd.outlen = 0;
  scsi_cmd.cmd[0] = ALLOW_MEDIUM_REMOVAL;
  scsi_cmd.cmd[1] = 0;
  scsi_cmd.cmd[2] = 0;
  scsi_cmd.cmd[3] = 0;
  scsi_cmd.cmd[4] = 0;
  scsi_cmd.cmd[5] = 0;
  status = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, (void *)&scsi_cmd);
  if (status != 0)
    return 0;
  
  scsi_cmd.inlen  = 0;
  scsi_cmd.outlen = 0;
  scsi_cmd.cmd[0] = START_STOP;
  scsi_cmd.cmd[1] = 0;
  scsi_cmd.cmd[2] = 0;
  scsi_cmd.cmd[3] = 0;
  scsi_cmd.cmd[4] = 1;
  scsi_cmd.cmd[5] = 0;
  status = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, (void *)&scsi_cmd);
  if (status != 0)
    return 0;
  
  scsi_cmd.inlen  = 0;
  scsi_cmd.outlen = 0;
  scsi_cmd.cmd[0] = START_STOP;
  scsi_cmd.cmd[1] = 0;
  scsi_cmd.cmd[2] = 0;
  scsi_cmd.cmd[3] = 0;
  scsi_cmd.cmd[4] = 2;
  scsi_cmd.cmd[5] = 0;
  status = ioctl(fd, SCSI_IOCTL_SEND_COMMAND, (void *)&scsi_cmd);
  if (status != 0)
    return 0;
  
  /* force kernel to reread partition table when new disc inserted */
  status = ioctl(fd, BLKRRPART);
  return (status == 0);
}

/*!
  Eject media in CD drive. 
  Return 0 if success and 1 for failure, and 2 if no routine.
 */
static int 
_cdio_eject_media (void *env) {

  _img_private_t *_obj = env;
  int ret=2;
  int status;
  int fd;

  close(_obj->gen.fd);
  _obj->gen.fd = -1;
  if ((fd = open (_obj->gen.source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    if((status = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT)) > 0) {
      switch(status) {
      case CDS_TRAY_OPEN:
	if((ret = ioctl(fd, CDROMCLOSETRAY)) != 0) {
	  cdio_error ("ioctl CDROMCLOSETRAY failed: %s\n", strerror(errno));  
	  ret = 1;
	}
	break;
      case CDS_DISC_OK:
	if((ret = ioctl(fd, CDROMEJECT)) != 0) {
	  int eject_error = errno;
	  /* Try ejecting the SCSI way... */
	  ret = _cdio_eject_scsi(fd);
	  if (0 != ret) {
	    cdio_error("ioctl CDROMEJECT failed: %s\n", strerror(eject_error));
	    ret = 1;
	  }
	}
	break;
      default:
	cdio_error ("Unknown CD-ROM (%d)\n", status);
	ret = 1;
      }
    } else {
      cdio_error ("CDROM_DRIVE_STATUS failed: %s\n", strerror(errno));
      ret=1;
    }
    close(fd);
    return ret;
  }
  return 2;
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
    case _AM_IOCTL:
      return "ioctl";
    case _AM_READ_CD:
      return "READ_CD";
    case _AM_READ_10:
      return "READ_10";
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
_cdio_get_first_track_num(void *env) 
{
  _img_private_t *_obj = env;
  
  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

  return FIRST_TRACK_NUM;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
_cdio_get_mcn (void *env) {

  struct cdrom_mcn mcn;
  _img_private_t *_obj = env;
  if (ioctl(_obj->gen.fd, CDROM_GET_MCN, &mcn) != 0)
    return NULL;
  return strdup(mcn.medium_catalog_number);
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
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
  track_num in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
_cdio_get_track_msf(void *env, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = env;

  if (NULL == msf) return false;

  if (!_obj->gen.toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = TOTAL_TRACKS+1;

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

#endif /* HAVE_LINUX_CDROM */

/*!
  Return a string containing the default VCD device if none is specified.
 */
char *
cdio_get_default_device_linux(void)
{
#ifndef HAVE_LINUX_CDROM
  return NULL;
  
#else
  /* checklist: /dev/cdrom, /dev/dvd /dev/hd?, /dev/scd? /dev/sr? */
  static char checklist1[][40] = {
    {"cdrom"}, {"dvd"}, {""}
  };
  static char checklist2[][40] = {
    {"?a hd?"}, {"?0 scd?"}, {"?0 sr?"}, {""}
  };
  unsigned int i;
  char drive[40];
  int exists;
  char *ret_drive;

  /* Scan the system for CD-ROM drives.
  */
  for ( i=0; strlen(checklist1[i]) > 0; ++i ) {
    sprintf(drive, "/dev/%s", checklist1[i]);
    if ( (exists=cdio_is_cdrom(drive, NULL)) > 0 ) {
      return strdup(drive);
    }
  }

  /* Now check the currently mounted CD drives */
  if (NULL != (ret_drive = cdio_check_mounts("/etc/mtab")))
    return ret_drive;
  
  /* Finally check possible mountable drives in /etc/fstab */
  if (NULL != (ret_drive = cdio_check_mounts("/etc/fstab")))
    return ret_drive;

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for ( i=0; strlen(checklist2[i]) > 0; ++i ) {
    unsigned int j;
    char *insert;
    exists = 1;
    for ( j=checklist2[i][1]; exists; ++j ) {
      sprintf(drive, "/dev/%s", &checklist2[i][3]);
      insert = strchr(drive, '?');
      if ( insert != NULL ) {
	*insert = j;
      }
      if ( (exists=cdio_is_cdrom(drive, NULL)) > 0 ) {
	return(strdup(drive));
      }
    }
  }
  return NULL;
#endif /*HAVE_LINUX_CDROM*/
}
/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_linux (const char *orig_source_name)
{

#ifdef HAVE_LINUX_CDROM
  CdIo *ret;
  _img_private_t *_data;
  char *source_name;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = cdio_generic_free,
    .get_arg            = _cdio_get_arg,
    .get_default_device = cdio_get_default_device_linux,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_mcn            = _cdio_get_mcn,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = NULL, /* This could be implemented if need be. */
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = cdio_generic_lseek,
    .read               = cdio_generic_read,
    .read_audio_sectors = _cdio_read_audio_sectors,
    .read_mode2_sector  = _cdio_read_mode2_sector,
    .read_mode2_sectors = _cdio_read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->access_mode    = _AM_READ_CD;
  _data->gen.init       = false;
  _data->gen.fd         = -1;

  if (NULL == orig_source_name) {
    source_name=cdio_get_default_device_linux();
    if (NULL == source_name) return NULL;
    _cdio_set_arg(_data, "source", source_name);
    free(source_name);
  } else 
    _cdio_set_arg(_data, "source", orig_source_name);

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
#endif /* HAVE_LINUX_CDROM */

}

bool
cdio_have_linux (void)
{
#ifdef HAVE_LINUX_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_LINUX_CDROM */
}


