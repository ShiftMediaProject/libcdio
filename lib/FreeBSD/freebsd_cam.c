/*
    $Id: freebsd_cam.c,v 1.19 2004/07/19 01:13:32 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>

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
   control of the CD drive via SCSI emulation.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: freebsd_cam.c,v 1.19 2004/07/19 01:13:32 rocky Exp $";

#ifdef HAVE_FREEBSD_CDROM

#include "freebsd.h"
#include <cdio/scsi_mmc.h>

static const u_char scsi_cdblen[8] = {6, 10, 10, 12, 12, 12, 10, 10};

static int
_scsi_cmd (_img_private_t * env)
{
  int retval;
  env->ccb.csio.cdb_len = scsi_cdblen[(env->ccb.csio.cdb_io.cdb_bytes[0] >> 5) & 7];
  if ((retval = cam_send_ccb(env->cam, &env->ccb)) < 0)
    {
      cdio_warn ("transport failed: ", retval);
      return -1;
    }
  if ((env->ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
    {
      return 0;
    }
  errno = EIO;
  retval = ERRCODE(((unsigned char *)&env->ccb.csio.sense_data));
  if (retval == 0)
    retval = -1;
  else
    CREAM_ON_ERRNO(((unsigned char *)&env->ccb.csio.sense_data));
  cdio_warn ("transport failed: ", retval);
  return retval;
}

bool
init_freebsd_cam (_img_private_t *env)
{
  char pass[100];
  
  env->cam=NULL;
  memset (&env->ccb, 0, sizeof(env->ccb));
  env->ccb.ccb_h.func_code = XPT_GDEVLIST;

  if (-1 == env->gen.fd) 
    env->gen.fd = open (env->device, O_RDONLY, 0);

  if (env->gen.fd < 0)
    {
      cdio_warn ("open (%s): %s", env->device, strerror (errno));
      return false;
    }

  if (ioctl (env->gen.fd, CAMGETPASSTHRU, &env->ccb) < 0)
    {
      cdio_warn ("open: %s", strerror (errno));
      return false;
    }
  sprintf (pass,"/dev/%.15s%u",
	   env->ccb.cgdl.periph_name,
	   env->ccb.cgdl.unit_number);
  env->cam = cam_open_pass (pass,O_RDWR,NULL);
  env->gen.init   = true;
  env->b_cam_init = true;
  return true;
}

void
free_freebsd_cam (void *user_data)
{
  _img_private_t *env = user_data;

  if (NULL == env) return;

  if (env->gen.fd > 0)
    close (env->gen.fd);
  env->gen.fd = -1;

  if(env->cam)
    cam_close_device(env->cam);

  free (env);
}

/**** 
      The below are really rough guesses. They are here in the hope
      they can be used as a starting point for someone who knows what
      they are doing.
*******/
#if 0
/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
get_drive_mcn_freebsd_cam (img_private_t *env)
{
  char buf[192] = { 0, };
  int rc;
  
  memset(&env->ccb, 0, sizeof(env->ccb));

  env->ccb.ccb_h.path_id    = env->cam->path_id;
  env->ccb.ccb_h.target_id  = env->cam->target_id;
  env->ccb.ccb_h.target_lun = env->cam->target_lun;
  cam_fill_csio (&(env->ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(env->ccb.csio.sense_data), 0, 30*1000);

  /* Initialize my_scsi_cdb as a Mode Select(6) */
  CDIO_MMC_SET_COMMAND(env->ccb.csio.cdb_io.cdb_bytes, 
		       CDIO_MMC_GPCMD_READ_SUBCHANNEL);
  env->ccb.csio.cdb_io.cdb_bytes[1] = 0x0;  
  env->ccb.csio.cdb_io.cdb_bytes[2] = 0x40;
  env->ccb.csio.cdb_io.cdb_bytes[3] = CDIO_SUBCHANNEL_MEDIA_CATALOG;
  env->ccb.csio.cdb_io.cdb_bytes[4] = 0;    /* Not used */
  env->ccb.csio.cdb_io.cdb_bytes[5] = 0;    /* Not used */
  env->ccb.csio.cdb_io.cdb_bytes[6] = 0;    /* Not used */
  env->ccb.csio.cdb_io.cdb_bytes[7] = 0;    /* Not used */
  env->ccb.csio.cdb_io.cdb_bytes[8] = 28; 
  env->ccb.csio.cdb_io.cdb_bytes[9] = 0;    /* Not used */
  
  /* suc.suc_timeout = 500; */
  env->ccb.csio.ccb_h.flags = CAM_DIR_OUT;
  env->ccb.csio.data_ptr  = buf;
  env->ccb.csio.dxfer_len = sizeof(buf);

  rc =  _scsi_cmd (env);

  if(rc == 0) {
    return strdup(&buf[9]);
  }
  return NULL;
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static cdio_drive_cap_t
get_drive_cap_freebsd_cam (img_private_t *env,
			   cdio_drive_read_cap_t  *p_read_cap,
			   cdio_drive_write_cap_t *p_write_cap,
			   cdio_drive_misc_cap_t  *p_misc_cap)
{
  int32_t i_drivetype = 0;
  uint8_t buf[192] = { 0, };
  int rc;
  
  memset(&env->ccb, 0, sizeof(env->ccb));

  env->ccb.ccb_h.path_id    = env->cam->path_id;
  env->ccb.ccb_h.target_id  = env->cam->target_id;
  env->ccb.ccb_h.target_lun = env->cam->target_lun;
  cam_fill_csio (&(env->ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(env->ccb.csio.sense_data), 0, 30*1000);

  /* Initialize my_scsi_cdb as a Mode Select(6) */
  CDIO_MMC_SET_COMMAND(env->ccb.csio.cdb_io.cdb_bytes, 
		       CDIO_MMC_GPCMD_MODE_SENSE6);
  env->ccb.csio.cdb_io.cdb_bytes[1] = 0x0;  
                                      /* use ALL_PAGES?*/
  env->ccb.csio.cdb_io.cdb_bytes[2] = CDIO_MMC_CAPABILITIES_PAGE; 
  env->ccb.csio.cdb_io.cdb_bytes[3] = 0;    /* Not used */
  env->ccb.csio.cdb_io.cdb_bytes[4] = 128; 
  env->ccb.csio.cdb_io.cdb_bytes[5] = 0;    /* Not used */
  
  /* suc.suc_timeout = 500; */
  env->ccb.csio.ccb_h.flags = CAM_DIR_OUT;
  env->ccb.csio.data_ptr  = buf;
  env->ccb.csio.dxfer_len = sizeof(buf);

  rc =  _scsi_cmd (env);

  if(rc == 0) {
    unsigned int n=buf[3]+4;
    cdio_get_drive_cap_mmc(&(buf[n], p_read_cap, p_write_cap, p_misc_cap));
  } else {
    *p_read_cap  = CDIO_DRIVE_CAP_ERROR;
    *p_write_cap = CDIO_DRIVE_CAP_ERROR;
    *p_misc_cap  = CDIO_DRIVE_CAP_ERROR;
  }
}
#endif

static int
_set_bsize (_img_private_t *env, unsigned int bsize)
{
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

  memset(&env->ccb, 0, sizeof(env->ccb));
  memset (&mh, 0, sizeof (mh));

  env->ccb.ccb_h.path_id    = env->cam->path_id;
  env->ccb.ccb_h.target_id  = env->cam->target_id;
  env->ccb.ccb_h.target_lun = env->cam->target_lun;
  cam_fill_csio (&(env->ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(env->ccb.csio.sense_data), 0, 30*1000);
  env->ccb.csio.cdb_len = 4+1;
  
  CDIO_MMC_SET_COMMAND(env->ccb.csio.cdb_io.cdb_bytes, 
		       CDIO_MMC_GPCMD_MODE_SELECT_6);
  env->ccb.csio.cdb_io.cdb_bytes[1] = 1 << 4;
  env->ccb.csio.cdb_io.cdb_bytes[4] = 12;
  
  env->ccb.csio.data_ptr = (u_char *)&mh;
  env->ccb.csio.dxfer_len = sizeof (mh);;
  
  /* suc.suc_timeout = 500; */
  env->ccb.csio.ccb_h.flags = CAM_DIR_OUT;
  
  mh.block_desc_length = 0x08;
  mh.block_length_hi   = (bsize >> 16) & 0xff;
  mh.block_length_med  = (bsize >> 8) & 0xff;
  mh.block_length_lo   = (bsize >> 0) & 0xff;
  
  return _scsi_cmd (env);
}

int
read_mode2_sector_freebsd_cam (_img_private_t *env, void *data, lsn_t lsn, 
			       bool b_form2)
{
  if ( b_form2 )
    return read_mode2_sectors_freebsd_cam(env, data, lsn, 1);
  else {
    /* Need to pick out the data portion from a mode2 form2 frame */
    char buf[M2RAW_SECTOR_SIZE] = { 0, };
    int retval = read_mode2_sectors_freebsd_cam(env, buf, lsn, 1);
    if ( retval ) return retval;
    memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    return 0;
  }
}

int
read_mode2_sectors_freebsd_cam (_img_private_t *env, void *buf, lsn_t lsn, 
				unsigned int nblocks)
{
  int retval = 0;
  bool b_read_10 = false;

  memset(&env->ccb,0,sizeof(env->ccb));

  env->ccb.ccb_h.path_id    = env->cam->path_id;
  env->ccb.ccb_h.target_id  = env->cam->target_id;
  env->ccb.ccb_h.target_lun = env->cam->target_lun;
  cam_fill_csio (&(env->ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(env->ccb.csio.sense_data), 0, 30*1000);
  env->ccb.csio.cdb_len = (b_read_10 ? 8 : 9) + 1;
  
  CDIO_MMC_SET_COMMAND(env->ccb.csio.cdb_io.cdb_bytes, b_read_10
		       ? CDIO_MMC_GPCMD_READ_10 : CDIO_MMC_GPCMD_READ_CD);

  CDIO_MMC_SET_READ_LBA(env->ccb.csio.cdb_io.cdb_bytes, lsn);
  CDIO_MMC_SET_READ_LENGTH(env->ccb.csio.cdb_io.cdb_bytes, nblocks);
  
  if (!b_read_10) {
    env->ccb.csio.cdb_io.cdb_bytes[1] = 0; /* sector size mode2 */
    env->ccb.csio.cdb_io.cdb_bytes[9] = 0x58; /* 2336 mode2 mixed form */
  }

  env->ccb.csio.dxfer_len = M2RAW_SECTOR_SIZE * nblocks;
  env->ccb.csio.data_ptr  = buf;
  
  /* suc.suc_timeout = 500; */
  env->ccb.csio.ccb_h.flags = CAM_DIR_IN;
  
  if (b_read_10)
    {
      if ((retval = _set_bsize (env, M2RAW_SECTOR_SIZE)))
	goto out;
      
      if ((retval = _scsi_cmd(env)))
	{
	  _set_bsize (env, CDIO_CD_FRAMESIZE);
	  goto out;
	}
      retval = _set_bsize (env, CDIO_CD_FRAMESIZE);
    }
  else
    retval = _scsi_cmd(env);
  
 out:
  return retval;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
uint32_t
stat_size_freebsd_cam (_img_private_t *env)
{
  uint8_t buf[12] = { 0, };

  uint32_t retval;

  memset(&env->ccb,0,sizeof(env->ccb));
  env->ccb.ccb_h.path_id    = env->cam->path_id;
  env->ccb.ccb_h.target_id  = env->cam->target_id;
  env->ccb.ccb_h.target_lun = env->cam->target_lun;
  cam_fill_csio (&(env->ccb.csio), 1, NULL, CAM_DEV_QFRZDIS, 
		 MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(env->ccb.csio.sense_data), 0, 30*1000);
  env->ccb.csio.cdb_len = 8+1;
  
  /* Operation code */
  CDIO_MMC_SET_COMMAND(env->ccb.csio.cdb_io.cdb_bytes, 
		       CDIO_MMC_GPCMD_READ_TOC);

  env->ccb.csio.cdb_io.cdb_bytes[1] = 0; /* lba; msf: 0x2 */

  /* Format */
  env->ccb.csio.cdb_io.cdb_bytes[2] = CDIO_MMC_READTOC_FMT_TOC;

  CDIO_MMC_SET_START_TRACK(env->ccb.csio.cdb_io.cdb_bytes, 
			   CDIO_CDROM_LEADOUT_TRACK);

  env->ccb.csio.cdb_io.cdb_bytes[8] = 12; /* ? */
  
  env->ccb.csio.data_ptr = buf;
  env->ccb.csio.dxfer_len = sizeof (buf);

  /*suc.suc_timeout = 500; */
  env->ccb.csio.ccb_h.flags = CAM_DIR_IN;

  if (_scsi_cmd(env))
    return 0;

  {
    int i;

    retval = 0;
    for (i = 8; i < 12; i++)
      {
        retval <<= 8;
        retval += buf[i];
      }
  }

  return retval;
}

int
eject_media_freebsd_cam (_img_private_t *env) 
{
  
  if (env->gen.fd == -1)
    return 2;
  
  memset(&env->ccb,0,sizeof(env->ccb));
  env->ccb.ccb_h.path_id    = env->cam->path_id;
  env->ccb.ccb_h.target_id  = env->cam->target_id;
  env->ccb.ccb_h.target_lun = env->cam->target_lun;
  cam_fill_csio (&(env->ccb.csio), 1, NULL, CAM_DEV_QFRZDIS, 
		 MSG_SIMPLE_Q_TAG, NULL, 0, sizeof(env->ccb.csio.sense_data), 
		 0, 30*1000);
  env->ccb.csio.cdb_len = 5+1;
  
  CDIO_MMC_SET_COMMAND(env->ccb.csio.cdb_io.cdb_bytes, 
		       CDIO_MMC_GPCMD_START_STOP);
  env->ccb.csio.cdb_io.cdb_bytes[1] = 0x1;	/* immediate */
  env->ccb.csio.cdb_io.cdb_bytes[4] = 0x2;	/* eject */
  
  env->ccb.csio.data_ptr = 0;
  env->ccb.csio.dxfer_len = 0;
  
  /* suc.suc_timeout = 500; */
  env->ccb.csio.ccb_h.flags = CAM_DIR_IN;
  return(_scsi_cmd(env));
}

#endif /* HAVE_FREEBSD_CDROM */
