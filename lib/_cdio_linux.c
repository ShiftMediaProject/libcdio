/*
    $Id: _cdio_linux.c,v 1.72 2004/07/22 09:52:17 rocky Exp $

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

/* This file contains Linux-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_linux.c,v 1.72 2004/07/22 09:52:17 rocky Exp $";

#include <string.h>

#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/types.h>
#include <cdio/scsi_mmc.h>
#include <cdio/cdtext.h>
#include "cdtext_private.h"
#include "cdio_assert.h"
#include "cdio_private.h"

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

/* Default value in milliseconds we will wait for a command to 
   complete. */
#define DEFAULT_TIMEOUT 500

#define TOTAL_TRACKS    (p_env->tochdr.cdth_trk1)
#define FIRST_TRACK_NUM (p_env->tochdr.cdth_trk0)

typedef enum {
  _AM_NONE,
  _AM_IOCTL,
  _AM_READ_CD,
  _AM_READ_10
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  /* Entry info for each track, add 1 for leadout. */
  struct cdrom_tocentry  tocent[CDIO_CD_MAX_TRACKS+1]; 

  cdtext_t      cdtext;	         /* CD-TEXT */

  access_mode_t access_mode;
  bool b_cdtext_init;
  bool b_cdtext_error;

  /* Some of the more OS specific things. */
  cdtext_t      cdtext_track[CDIO_CD_MAX_TRACKS+1]; /*CD-TEXT for each track*/
  struct cdrom_tochdr    tochdr;

} _img_private_t;

/* Some ioctl() errno values which occur when the tray is empty */
#define ERRNO_TRAYEMPTY(errno)	\
	((errno == EIO) || (errno == ENOENT) || (errno == EINVAL))

static access_mode_t 
str_to_access_mode_linux(const char *psz_access_mode) 
{
  const access_mode_t default_access_mode = _AM_IOCTL;

  if (NULL==psz_access_mode) return default_access_mode;
  
  if (!strcmp(psz_access_mode, "IOCTL"))
    return _AM_IOCTL;
  else if (!strcmp(psz_access_mode, "READ_CD"))
    return _AM_READ_CD;
  else if (!strcmp(psz_access_mode, "READ_10"))
    return _AM_READ_10;
  else {
    cdio_warn ("unknown access type: %s. Default IOCTL used.", 
	       psz_access_mode);
    return default_access_mode;
  }
}

/* Check a drive to see if it is a CD-ROM 
   Return 1 if a CD-ROM. 0 if it exists but isn't a CD-ROM drive
   and -1 if no device exists .
*/
static bool
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
  cdfd = open(drive, (O_RDONLY|O_NONBLOCK), 0);
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

/*! 
  Get disc tyhpe associated with cd_obj.
*/
static discmode_t
_get_discmode_linux (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  int32_t i_discmode;
  i_discmode = ioctl (p_env->gen.fd, CDROM_DISC_STATUS);
  
  if (i_discmode < 0) return CDIO_DISC_MODE_ERROR;

  /* FIXME Need to add getting DVD types. */
  switch(i_discmode) {
  case CDS_AUDIO:
    return CDIO_DISC_MODE_CD_DA;
  case CDS_DATA_1:
    return CDIO_DISC_MODE_CD_DATA_1;
  case CDS_DATA_2:
    return CDIO_DISC_MODE_CD_DATA_2;
  case CDS_MIXED:
    return CDIO_DISC_MODE_CD_MIXED;
  case CDS_XA_2_1:
    return CDIO_DISC_MODE_CD_XA_2_1;
  case CDS_XA_2_2:
    return CDIO_DISC_MODE_CD_XA_2_2;
  case CDS_NO_INFO:
    return CDIO_DISC_MODE_NO_INFO;
  default:
    return CDIO_DISC_MODE_ERROR;
  }
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
	  free(mnt_type);
	  endmntent(mntfp);
	  return mnt_dev;
	}
      }
      free(mnt_dev);
      free(mnt_type);
    }
    endmntent(mntfp);
  }
  return NULL;
}

/*!
  Run a SCSI MMC command. 
 
  cdio	        CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return true if command completed successfully and false if not.
 */
static int
scsi_mmc_run_cmd_linux( const _img_private_t *p_env, int i_timeout,
			unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
			scsi_mmc_direction_t e_direction, 
			unsigned int i_buf, /*out*/ void *p_buf )
{
  struct cdrom_generic_command cgc;
  memset (&cgc, 0, sizeof (struct cdrom_generic_command));
  memcpy(&cgc.cmd, p_cdb, i_cdb);
  cgc.buflen = i_buf;
  cgc.buffer = p_buf;
  cgc.data_direction = SCSI_MMC_DATA_READ ? CGC_DATA_READ : CGC_DATA_WRITE;

#ifdef HAVE_LINUX_CDROM_TIMEOUT
  if (i_timeout >= 0)
    cgc.timeout = i_timeout;
#endif

  return ioctl (p_env->gen.fd, CDROM_SEND_PACKET, &cgc);
}

static int 
_set_bsize (_img_private_t *p_env, unsigned int bsize)
{
  scsi_mmc_cdb_t cdb;

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
  mh.block_desc_length = 0x08;
  mh.block_length_hi   = (bsize >> 16) & 0xff;
  mh.block_length_med  = (bsize >>  8) & 0xff;
  mh.block_length_lo   = (bsize >>  0) & 0xff;

  memset (&cdb, 0, sizeof (cdb));
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SELECT_6);

  cdb.field[1] = 1 << 4;
  cdb.field[4] = 12;
  
  return scsi_mmc_run_cmd_linux (p_env, DEFAULT_TIMEOUT,
				 scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_WRITE, sizeof(mh), &mh);
}

/* Packet driver to read mode2 sectors. 
   Can read only up to 25 blocks.
*/
static int
_read_sectors_mmc (_img_private_t *p_env, void *p_buf, lba_t lba, 
		   int sector_type, unsigned int nblocks)
{
  scsi_mmc_cdb_t cdb;

  memset (&cdb, 0, sizeof (scsi_mmc_cdb_t));

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);
  CDIO_MMC_SET_READ_TYPE  (cdb.field, sector_type);
  CDIO_MMC_SET_READ_LBA   (cdb.field, lba);
  CDIO_MMC_SET_READ_LENGTH(cdb.field, nblocks);
  CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(cdb.field, 
					   CDIO_MMC_MCSB_ALL_HEADERS);

  return scsi_mmc_run_cmd_linux (p_env, DEFAULT_TIMEOUT, 
				 scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_READ, 
				 CDIO_CD_FRAMESIZE_RAW * nblocks,
				 p_buf);
}

/* MMC driver to read audio sectors. 
   Can read only up to 25 blocks.
*/
static int
_read_audio_sectors_linux (void *p_user_data, void *buf, lsn_t lsn, 
			   unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  return _read_sectors_mmc( p_env, buf, lsn, 
			    CDIO_MMC_READ_TYPE_CDDA, nblocks);
}

/* Packet driver to read mode2 sectors. 
   Can read only up to 25 blocks.
*/
static int
_read_mode2_sectors_mmc (_img_private_t *p_env, void *p_buf, lba_t lba, 
			 unsigned int nblocks, bool b_read_10)
{
  scsi_mmc_cdb_t cdb;

  memset (&cdb, 0, sizeof (scsi_mmc_cdb_t));

  CDIO_MMC_SET_COMMAND(cdb.field, b_read_10 
		       ? CDIO_MMC_GPCMD_READ_10 : CDIO_MMC_GPCMD_READ_CD);

  CDIO_MMC_SET_READ_LBA(cdb.field, lba);
  CDIO_MMC_SET_READ_LENGTH(cdb.field, nblocks);

  if (!b_read_10) {
    cdb.field[1] = 0; /* sector size mode2 */
    cdb.field[9] = 0x58; /* 2336 mode2 */
  }

  if (b_read_10) {
    int retval;
    
    if ((retval = _set_bsize (p_env, M2RAW_SECTOR_SIZE)))
      return retval;
    
    if ((retval = scsi_mmc_run_cmd_linux (p_env, 0, 
				     scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				     SCSI_MMC_DATA_READ,
					  M2RAW_SECTOR_SIZE * nblocks, p_buf)))
      {
	_set_bsize (p_env, CDIO_CD_FRAMESIZE);
	return retval;
      }
    
    if ((retval = _set_bsize (p_env, CDIO_CD_FRAMESIZE)))
      return retval;
  } else
    return scsi_mmc_run_cmd_linux (p_env, 0, 
				   scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				   SCSI_MMC_DATA_READ,
				   M2RAW_SECTOR_SIZE * nblocks, p_buf);

  return 0;
}

static int
_read_mode2_sectors (_img_private_t *p_env, void *p_buf, lba_t lba, 
		     unsigned int nblocks, bool b_read_10)
{
  unsigned int l = 0;
  int retval = 0;

  while (nblocks > 0)
    {
      const unsigned nblocks2 = (nblocks > 25) ? 25 : nblocks;
      void *p_buf2 = ((char *)p_buf ) + (l * M2RAW_SECTOR_SIZE);
      
      retval |= _read_mode2_sectors_mmc (p_env, p_buf2, lba + l, 
					 nblocks2, b_read_10);

      if (retval)
	break;

      nblocks -= nblocks2;
      l += nblocks2;
    }

  return retval;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_mode1_sector_linux (void *p_user_data, void *p_data, lsn_t lsn, 
			 bool b_form2)
{

#if FIXED
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  struct cdrom_msf *p_msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);

 retry:
  switch (_obj->access_mode)
    {
    case _AM_NONE:
      cdio_warn ("no way to read mode1");
      return 1;
      break;
      
    case _AM_IOCTL:
      if (ioctl (p_env->gen.fd, CDROMREADMODE1, &buf) == -1)
	{
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
      break;
      
    case _AM_READ_CD:
    case _AM_READ_10:
      if (_read_mode2_sectors (p_env->gen.fd, buf, lsn, 1, 
				      (p_env->access_mode == _AM_READ_10)))
	{
	  perror ("ioctl()");
	  if (p_env->access_mode == _AM_READ_CD)
	    {
	      cdio_info ("READ_CD failed; switching to READ_10 mode...");
	      p_env->access_mode = _AM_READ_10;
	      goto retry;
	    }
	  else
	    {
	      cdio_info ("READ_10 failed; switching to ioctl(CDROMREADMODE2) mode...");
	      p_env->access_mode = _AM_IOCTL;
	      goto retry;
	    }
	  return 1;
	}
      break;
    }

  memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
#else
  return cdio_generic_read_form1_sector(p_user_data, p_data, lsn);
#endif
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_read_mode1_sectors_linux (void *p_user_data, void *p_data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_linux (p_env,
					    ((char *)p_data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_mode2_sector_linux (void *p_user_data, void *p_data, lsn_t lsn, 
			  bool b_form2)
{
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  struct cdrom_msf *msf = (struct cdrom_msf *) &buf;
  msf_t _msf;

  _img_private_t *p_env = p_user_data;

  cdio_lba_to_msf (cdio_lsn_to_lba(lsn), &_msf);
  msf->cdmsf_min0 = from_bcd8(_msf.m);
  msf->cdmsf_sec0 = from_bcd8(_msf.s);
  msf->cdmsf_frame0 = from_bcd8(_msf.f);

 retry:
  switch (p_env->access_mode)
    {
    case _AM_NONE:
      cdio_warn ("no way to read mode2");
      return 1;
      break;
      
    case _AM_IOCTL:
      if (ioctl (p_env->gen.fd, CDROMREADMODE2, &buf) == -1)
	{
	  perror ("ioctl()");
	  return 1;
	  /* exit (EXIT_FAILURE); */
	}
      break;
      
    case _AM_READ_CD:
    case _AM_READ_10:
      if (_read_mode2_sectors (p_env, buf, lsn, 1, 
			       (p_env->access_mode == _AM_READ_10)))
	{
	  perror ("ioctl()");
	  if (p_env->access_mode == _AM_READ_CD)
	    {
	      cdio_info ("READ_CD failed; switching to READ_10 mode...");
	      p_env->access_mode = _AM_READ_10;
	      goto retry;
	    }
	  else
	    {
	      cdio_info ("READ_10 failed; switching to ioctl(CDROMREADMODE2) mode...");
	      p_env->access_mode = _AM_IOCTL;
	      goto retry;
	    }
	  return 1;
	}
      break;
    }

  if (b_form2)
    memcpy (p_data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)p_data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
  
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_read_mode2_sectors_linux (void *p_user_data, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  unsigned int i_blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  /* For each frame, pick out the data part we need */
  for (i = 0; i < nblocks; i++) {
    int retval;
    if ( (retval = _read_mode2_sector_linux (p_env, 
					    ((char *)data) + (i_blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_stat_size_linux (void *p_user_data)
{
  _img_private_t *p_env = p_user_data;

  struct cdrom_tocentry tocent;
  uint32_t size;

  tocent.cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  tocent.cdte_format = CDROM_LBA;
  if (ioctl (p_env->gen.fd, CDROMREADTOCENTRY, &tocent) == -1)
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
_set_arg_linux (void *p_user_data, const char key[], const char value[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source"))
    {
      if (!value)
	return -2;

      free (p_env->gen.source_name);
      
      p_env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      return str_to_access_mode_linux(value);
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
_cdio_read_toc (_img_private_t *p_env) 
{
  int i;

  /* read TOC header */
  if ( ioctl(p_env->gen.fd, CDROMREADTOCHDR, &p_env->tochdr) == -1 ) {
    cdio_warn("%s: %s\n", 
            "error in ioctl CDROMREADTOCHDR", strerror(errno));
    return false;
  }

  /* read individual tracks */
  for (i= FIRST_TRACK_NUM; i<=TOTAL_TRACKS; i++) {
    p_env->tocent[i-FIRST_TRACK_NUM].cdte_track = i;
    p_env->tocent[i-FIRST_TRACK_NUM].cdte_format = CDROM_MSF;
    if ( ioctl(p_env->gen.fd, CDROMREADTOCENTRY, 
	       &p_env->tocent[i-FIRST_TRACK_NUM]) == -1 ) {
      cdio_warn("%s %d: %s\n",
              "error in ioctl CDROMREADTOCENTRY for track", 
              i, strerror(errno));
      return false;
    }
    /****
    struct cdrom_msf0 *msf= &env->tocent[i-1].cdte_addr.msf;
    
    fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	     i, msf->minute, msf->second, msf->frame);
    ****/

  }

  /* read the lead-out track */
  p_env->tocent[TOTAL_TRACKS].cdte_track = CDIO_CDROM_LEADOUT_TRACK;
  p_env->tocent[TOTAL_TRACKS].cdte_format = CDROM_MSF;

  if (ioctl(p_env->gen.fd, CDROMREADTOCENTRY, 
	    &p_env->tocent[TOTAL_TRACKS]) == -1 ) {
    cdio_warn("%s: %s\n", 
	     "error in ioctl CDROMREADTOCENTRY for lead-out",
            strerror(errno));
    return false;
  }

  /*
  struct cdrom_msf0 *msf= &env->tocent[TOTAL_TRACKS].cdte_addr.msf;

  fprintf (stdout, "--- track# %d (msf %2.2x:%2.2x:%2.2x)\n",
	   i, msf->minute, msf->second, msf->frame);
  */

  p_env->gen.toc_init = true;
  return true;
}

static void
set_cdtext_field_linux(void *user_data, track_t i_track, 
		       track_t i_first_track,
		       cdtext_field_t e_field, const char *psz_value)
{
  char **pp_field;
  _img_private_t *env = user_data;
  
  if( i_track == 0 )
    pp_field = &(env->cdtext.field[e_field]);
  
  else
    pp_field = &(env->cdtext_track[i_track-i_first_track].field[e_field]);

  *pp_field = strdup(psz_value);
}

/*
  Read cdtext information for a CdIo object .
  
  return true on success, false on error or CD-TEXT information does
  not exist.
*/
static bool
_init_cdtext_linux (_img_private_t *p_env)
{

  scsi_mmc_cdb_t cdb;
  unsigned char wdata[2000]= {0, };  /* Data read from device starts here */
  int status;

  memset (&cdb, 0, sizeof (scsi_mmc_cdb_t));

  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC); 

  /* Format */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_CDTEXT;

  status = scsi_mmc_run_cmd_linux (p_env, DEFAULT_TIMEOUT,
				   scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				   SCSI_MMC_DATA_READ, sizeof(wdata), &wdata);

  if (status < 0) {
    cdio_info ("CD-TEXT reading failed\n");  
    p_env->b_cdtext_error = true;
    return false;
  } else {
    return cdtext_data_init(p_env, FIRST_TRACK_NUM, wdata, 
			    set_cdtext_field_linux);
  }
}

/*! 
  Get cdtext information for a CdIo object .
  
  @param obj the CD object that may contain CD-TEXT information.
  @return the CD-TEXT object or NULL if obj is NULL
  or CD-TEXT information does not exist.
*/
static const cdtext_t *
_get_cdtext_linux (void *p_user_data, track_t i_track)
{
  _img_private_t *p_env = p_user_data;

  if ( NULL == p_env ||
       (0 != i_track 
       && i_track >= TOTAL_TRACKS+FIRST_TRACK_NUM ) )
    return NULL;

  p_env->b_cdtext_init = _init_cdtext_linux(p_env);
  if (!p_env->b_cdtext_init || p_env->b_cdtext_error) return NULL;

  if (0 == i_track) 
    return &(p_env->cdtext);
  else 
    return &(p_env->cdtext_track[i_track-FIRST_TRACK_NUM]);

}

/*
 * Eject using SCSI MMC commands. Return 1 if successful, 0 otherwise.
 */
static int 
_eject_media_mmc(_img_private_t *p_env)
{
  int i_status;
  scsi_mmc_cdb_t cdb;
  uint8_t buf[1];
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL);

  i_status = scsi_mmc_run_cmd_linux (p_env, DEFAULT_TIMEOUT,
				 scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 1;
  i_status = scsi_mmc_run_cmd_linux (p_env, DEFAULT_TIMEOUT,
				 scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 2; /* eject */

  i_status = scsi_mmc_run_cmd_linux (p_env, DEFAULT_TIMEOUT,
				 scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  /* force kernel to reread partition table when new disc inserted */
  i_status = ioctl(p_env->gen.fd, BLKRRPART);
  return (i_status);
  
}

/*!
  Eject media in CD drive. 
  Return 0 if success and 1 for failure, and 2 if no routine.
 */
static int 
_eject_media_linux (void *p_user_data) {

  _img_private_t *p_env = p_user_data;
  int ret=2;
  int status;
  int fd;

  if ((fd = open (p_env->gen.source_name, O_RDONLY|O_NONBLOCK)) > -1) {
    if((status = ioctl(fd, CDROM_DRIVE_STATUS, CDSL_CURRENT)) > 0) {
      switch(status) {
      case CDS_TRAY_OPEN:
	if((ret = ioctl(fd, CDROMCLOSETRAY)) != 0) {
	  cdio_warn ("ioctl CDROMCLOSETRAY failed: %s\n", strerror(errno));  
	  ret = 1;
	}
	break;
      case CDS_DISC_OK:
	if((ret = ioctl(fd, CDROMEJECT)) != 0) {
	  int eject_error = errno;
	  /* Try ejecting the MMC way... */
	  ret = _eject_media_mmc(p_env);
	  if (0 != ret) {
	    cdio_warn("ioctl CDROMEJECT failed: %s\n", strerror(eject_error));
	    ret = 1;
	  }
	}
	break;
      default:
	cdio_warn ("Unknown CD-ROM (%d)\n", status);
	ret = 1;
      }
    } else {
      cdio_warn ("CDROM_DRIVE_STATUS failed: %s\n", strerror(errno));
      ret=1;
    }
    close(fd);
  } else
    ret = 2;
  close(p_env->gen.fd);
  p_env->gen.fd = -1;
  return ret;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_linux (void *env, const char key[])
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
_get_first_track_num_linux(void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  
  if (!p_env->gen.toc_init) _cdio_read_toc (p_env) ;

  return FIRST_TRACK_NUM;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
_get_mcn_linux (const void *env) {

  struct cdrom_mcn mcn;
  const _img_private_t *_obj = env;
  memset(&mcn, 0, sizeof(mcn));
  if (ioctl(_obj->gen.fd, CDROM_GET_MCN, &mcn) != 0)
    return NULL;
  return strdup(mcn.medium_catalog_number);
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static void
_get_drive_cap_linux (const void *env,
                    cdio_drive_read_cap_t  *p_read_cap,
                    cdio_drive_write_cap_t *p_write_cap,
                    cdio_drive_misc_cap_t  *p_misc_cap)
{
  const _img_private_t *_obj = env;
  int32_t i_drivetype;

  i_drivetype = ioctl (_obj->gen.fd, CDROM_GET_CAPABILITY, CDSL_CURRENT);

  if (i_drivetype < 0) {
    *p_read_cap  = CDIO_DRIVE_CAP_ERROR;
    *p_write_cap = CDIO_DRIVE_CAP_ERROR;
    *p_misc_cap  = CDIO_DRIVE_CAP_ERROR;
    return;
  }
  
  *p_read_cap  = 0;
  *p_write_cap = 0;
  *p_misc_cap  = 0;

  /* Reader */
  if (i_drivetype & CDC_PLAY_AUDIO) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_AUDIO;
  if (i_drivetype & CDC_CD_R) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_R;
  if (i_drivetype & CDC_CD_RW) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_RW;
  if (i_drivetype & CDC_DVD) 
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_DVD_ROM;

  /* Writer */
  if (i_drivetype & CDC_CD_RW) 
    *p_read_cap  |= CDIO_DRIVE_CAP_WRITE_CD_RW;
  if (i_drivetype & CDC_DVD_R) 
    *p_read_cap  |= CDIO_DRIVE_CAP_WRITE_DVD_R;
  if (i_drivetype & CDC_DVD_RAM) 
    *p_read_cap  |= CDIO_DRIVE_CAP_WRITE_DVD_RAM;

  /* Misc */
  if (i_drivetype & CDC_CLOSE_TRAY) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_CLOSE_TRAY;
  if (i_drivetype & CDC_OPEN_TRAY) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_EJECT;
  if (i_drivetype & CDC_LOCK) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_LOCK;
  if (i_drivetype & CDC_SELECT_SPEED) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_SELECT_SPEED;
  if (i_drivetype & CDC_SELECT_DISC) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_SELECT_DISC;
  if (i_drivetype & CDC_MULTI_SESSION) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_MULTI_SESSION;
  if (i_drivetype & CDC_MEDIA_CHANGED) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED;
  if (i_drivetype & CDC_RESET) 
    *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_RESET;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_get_num_tracks_linux(void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  
  if (!p_env->gen.toc_init) _cdio_read_toc (p_env) ;

  return TOTAL_TRACKS;
}

/*!  
  Get format of track. 
*/
static track_format_t
_get_track_format_linux(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if (i_track > (TOTAL_TRACKS+FIRST_TRACK_NUM) || i_track < FIRST_TRACK_NUM)
    return TRACK_FORMAT_ERROR;

  i_track -= FIRST_TRACK_NUM;

  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
   */
  if (p_env->tocent[i_track].cdte_ctrl & CDIO_CDROM_DATA_TRACK) {
    if (p_env->tocent[i_track].cdte_format == CDIO_CDROM_CDI_TRACK)
      return TRACK_FORMAT_CDI;
    else if (p_env->tocent[i_track].cdte_format == CDIO_CDROM_XA_TRACK)
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
_get_track_green_linux(void *p_user_data, track_t i_track) 
{
  _img_private_t *p_env = p_user_data;
  
  if (!p_env->gen.toc_init) _cdio_read_toc (p_env) ;

  if (i_track >= (TOTAL_TRACKS+FIRST_TRACK_NUM) || i_track < FIRST_TRACK_NUM)
    return false;

  i_track -= FIRST_TRACK_NUM;

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cd-info for a while.
   */
  return ((p_env->tocent[i_track].cdte_ctrl & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers usually start at something 
  greater than 0, usually 1.

  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
_get_track_msf_linux(void *p_user_data, track_t i_track, msf_t *msf)
{
  _img_private_t *p_env = p_user_data;

  if (NULL == msf) return false;

  if (!p_env->gen.toc_init) _cdio_read_toc (p_env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) 
    i_track = TOTAL_TRACKS + FIRST_TRACK_NUM;

  if (i_track > (TOTAL_TRACKS+FIRST_TRACK_NUM) || i_track < FIRST_TRACK_NUM) {
    return false;
  } else {
    struct cdrom_msf0  *msf0= 
      &p_env->tocent[i_track-FIRST_TRACK_NUM].cdte_addr.msf;
    msf->m = to_bcd8(msf0->minute);
    msf->s = to_bcd8(msf0->second);
    msf->f = to_bcd8(msf0->frame);
    return true;
  }
}

/* checklist: /dev/cdrom, /dev/dvd /dev/hd?, /dev/scd? /dev/sr? */
static char checklist1[][40] = {
  {"cdrom"}, {"dvd"}, {""}
};
static char checklist2[][40] = {
  {"?a hd?"}, {"?0 scd?"}, {"?0 sr?"}, {""}
};

#endif /* HAVE_LINUX_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_linux (void)
{
#ifndef HAVE_LINUX_CDROM
  return NULL;
#else
  unsigned int i;
  char drive[40];
  char *ret_drive;
  bool exists;
  char **drives = NULL;
  unsigned int num_drives=0;
  
  /* Scan the system for CD-ROM drives.
  */
  for ( i=0; strlen(checklist1[i]) > 0; ++i ) {
    sprintf(drive, "/dev/%s", checklist1[i]);
    if ( (exists=cdio_is_cdrom(drive, NULL)) > 0 ) {
      cdio_add_device_list(&drives, drive, &num_drives);
    }
  }

  /* Now check the currently mounted CD drives */
  if (NULL != (ret_drive = cdio_check_mounts("/etc/mtab"))) {
    cdio_add_device_list(&drives, ret_drive, &num_drives);
    free(ret_drive);
  }
  
  /* Finally check possible mountable drives in /etc/fstab */
  if (NULL != (ret_drive = cdio_check_mounts("/etc/fstab"))) {
    cdio_add_device_list(&drives, ret_drive, &num_drives);
    free(ret_drive);
  }

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for ( i=0; strlen(checklist2[i]) > 0; ++i ) {
    unsigned int j;
    char *insert;
    exists = true;
    for ( j=checklist2[i][1]; exists; ++j ) {
      sprintf(drive, "/dev/%s", &checklist2[i][3]);
      insert = strchr(drive, '?');
      if ( insert != NULL ) {
	*insert = j;
      }
      if ( (exists=cdio_is_cdrom(drive, NULL)) > 0 ) {
	cdio_add_device_list(&drives, drive, &num_drives);
      }
    }
  }
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /*HAVE_LINUX_CDROM*/
}

/*!
  Return a string containing the default CD device.
 */
char *
cdio_get_default_device_linux(void)
{
#ifndef HAVE_LINUX_CDROM
  return NULL;
  
#else
  unsigned int i;
  char drive[40];
  bool exists;
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
    exists = true;
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
cdio_open_linux (const char *psz_source_name)
{
  return cdio_open_am_linux(psz_source_name, NULL);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_linux (const char *psz_orig_source, const char *access_mode)
{

#ifdef HAVE_LINUX_CDROM
  CdIo *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs _funcs = {
    .eject_media        = _eject_media_linux,
    .free               = cdio_generic_free,
    .get_arg            = _get_arg_linux,
    .get_cdtext         = _get_cdtext_linux,
    .get_devices        = cdio_get_devices_linux,
    .get_default_device = cdio_get_default_device_linux,
    .get_discmode       = _get_discmode_linux,
    .get_drive_cap      = _get_drive_cap_linux,
    .get_first_track_num= _get_first_track_num_linux,
    .get_mcn            = _get_mcn_linux,
    .get_num_tracks     = _get_num_tracks_linux,
    .get_track_format   = _get_track_format_linux,
    .get_track_green    = _get_track_green_linux,
    .get_track_lba      = NULL, /* This could be implemented if need be. */
    .get_track_msf      = _get_track_msf_linux,
    .lseek              = cdio_generic_lseek,
    .read               = cdio_generic_read,
    .read_audio_sectors = _read_audio_sectors_linux,
    .read_mode1_sector  = _read_mode1_sector_linux,
    .read_mode1_sectors = _read_mode1_sectors_linux,
    .read_mode2_sector  = _read_mode2_sector_linux,
    .read_mode2_sectors = _read_mode2_sectors_linux,
    .set_arg            = _set_arg_linux,
    .stat_size          = _stat_size_linux
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));

  _data->access_mode    = str_to_access_mode_linux(access_mode);
  _data->gen.init       = false;
  _data->gen.fd         = -1;
  _data->b_cdtext_init  = false;
  _data->b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_linux();
    if (NULL == psz_source) return NULL;
    _set_arg_linux(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_linux(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
      cdio_info ("source %s is a not a device", psz_orig_source);
      return NULL;
    }
  }

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


