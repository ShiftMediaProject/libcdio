/*
    $Id: freebsd_cam.c,v 1.29 2004/08/01 11:36:35 rocky Exp $

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

static const char _rcsid[] = "$Id: freebsd_cam.c,v 1.29 2004/08/01 11:36:35 rocky Exp $";

#ifdef HAVE_FREEBSD_CDROM

#include "freebsd.h"
#include <cdio/scsi_mmc.h>

/* Default value in seconds we will wait for a command to 
   complete. */
#define DEFAULT_TIMEOUT_MSECS 10000

/*!
  Run a SCSI MMC command. 
 
  p_user_data   internal CD structure.
  i_timeout_ms  time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  i_cdb	        Size of p_cdb
  p_cdb	        CDB bytes. 
  e_direction	direction the transfer is to go.
  i_buf	        Size of buffer
  p_buf	        Buffer for data, both sending and receiving

  Return 0 if no error.
 */
int
run_scsi_cmd_freebsd_cam( const void *p_user_data, unsigned int i_timeout_ms,
			  unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
			  scsi_mmc_direction_t e_direction, 
			  unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  int   i_status;
  union ccb ccb;

  if (!p_env || !p_env->cam) return -2;
    
  memset(ccb, 0, sizeof(ccb));

  ccb.ccb_h.path_id    = p_env->cam->path_id;
  ccb.ccb_h.target_id  = p_env->cam->target_id;
  ccb.ccb_h.target_lun = p_env->cam->target_lun;
  ccb.ccb_h.timeout    = i_timeout_ms;

  cam_fill_csio (&(ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(ccb.csio.sense_data), 0, 30*1000);

  memcpy(ccb.csio.cdb_io.cdb_bytes, p_cdb, i_cdb);
  
  ccb.csio.ccb_h.flags = (SCSI_MMC_DATA_READ == e_direction)
    ? CAM_DIR_OUT : CAM_DIR_IN;

  ccb.csio.data_ptr  = p_buf;
  ccb.csio.dxfer_len = i_buf;

  ccb.csio.cdb_len = 
    scsi_mmc_get_cmd_len(ccb.csio.cdb_io.cdb_bytes[0]);
  
  if ((i_status = cam_send_ccb(p_env->cam, &ccb)) < 0)
    {
      cdio_warn ("transport failed: %d", i_status);
      return -1;
    }
  if ((ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
    {
      return 0;
    }
  errno = EIO;
  i_status = ERRCODE(((unsigned char *)&ccb.csio.sense_data));
  if (i_status == 0)
    i_status = -1;
  else
    CREAM_ON_ERRNO(((unsigned char *)&ccb.csio.sense_data));
  cdio_warn ("transport failed: %d", i_status);
  return i_status;
}

bool
init_freebsd_cam (_img_private_t *p_env)
{
  char pass[100];
  
  p_env->cam=NULL;
  memset (&p_env->ccb, 0, sizeof(p_env->ccb));
  p_env->ccb.ccb_h.func_code = XPT_GDEVLIST;

  if (-1 == p_env->gen.fd) 
    p_env->gen.fd = open (p_env->device, O_RDONLY, 0);

  if (p_env->gen.fd < 0)
    {
      cdio_warn ("open (%s): %s", p_env->device, strerror (errno));
      return false;
    }

  if (ioctl (p_env->gen.fd, CAMGETPASSTHRU, &p_env->ccb) < 0)
    {
      cdio_warn ("open: %s", strerror (errno));
      return false;
    }
  sprintf (pass,"/dev/%.15s%u",
	   p_env->ccb.cgdl.periph_name,
	   p_env->ccb.cgdl.unit_number);
  p_env->cam = cam_open_pass (pass,O_RDWR,NULL);
  if (!p_env->cam) return false;
  
  p_env->gen.init   = true;
  p_env->b_cam_init = true;
  return true;
}

void
free_freebsd_cam (void *user_data)
{
  _img_private_t *p_env = user_data;

  if (NULL == p_env) return;

  if (p_env->gen.fd > 0)
    close (p_env->gen.fd);
  p_env->gen.fd = -1;

  if(p_env->cam)
    cam_close_device(p_env->cam);

  free (p_env);
}

/**** 
      The below are really rough guesses. They are here in the hope
      they can be used as a starting point for someone who knows what
      they are doing.
*******/
#if 1
/*!
  Return the the kind of drive capabilities of device.

 */
void
get_drive_cap_freebsd_cam (const _img_private_t *p_env,
			   cdio_drive_read_cap_t  *p_read_cap,
			   cdio_drive_write_cap_t *p_write_cap,
			   cdio_drive_misc_cap_t  *p_misc_cap)
{
  scsi_mmc_cdb_t cdb = {{0, }};
  uint8_t  buf[256] = { 0, };
  int      i_status;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SENSE_10);
  cdb.field[1]  = 0x0;
  cdb.field[2]  = CDIO_MMC_ALL_PAGES;
  cdb.field[7]  = 0x01;
  cdb.field[8]  = 0x00; 

  i_status = run_scsi_cmd_freebsd_cam(p_env, DEFAULT_TIMEOUT_MSECS, 
				      scsi_mmc_get_cmd_len(cdb.field[0]),
				      &cdb, SCSI_MMC_DATA_READ, 
				      sizeof(buf), buf);
  if (0 == i_status) {
    uint8_t *p;
    int lenData  = ((unsigned int)buf[0] << 8) + buf[1];
    uint8_t *pMax = buf + 256;

    *p_read_cap  = 0;
    *p_write_cap = 0;
    *p_misc_cap  = 0;

    /* set to first sense mask, and then walk through the masks */
    p = buf + 8;
    while( (p < &(buf[2+lenData])) && (p < pMax) )       {
      uint8_t which;
      
      which = p[0] & 0x3F;
      switch( which )
	{
	case CDIO_MMC_AUDIO_CTL_PAGE:
	case CDIO_MMC_R_W_ERROR_PAGE:
	case CDIO_MMC_CDR_PARMS_PAGE:
	  /* Don't handle these yet. */
	  break;
	case CDIO_MMC_CAPABILITIES_PAGE:
	  scsi_mmc_get_drive_cap(p, p_read_cap, p_write_cap, p_misc_cap);
	  break;
	default: ;
	}
      p += (p[1] + 2);
    }
  } else {
    cdio_info("error in aspi USCSICMD MODE_SELECT");
    *p_read_cap  = CDIO_DRIVE_CAP_UNKNOWN;
    *p_write_cap = CDIO_DRIVE_CAP_UNKNOWN;
    *p_misc_cap  = CDIO_DRIVE_CAP_UNKNOWN;
  }
}

#endif

static int 
_set_bsize (_img_private_t *p_env, unsigned int bsize)
{
  scsi_mmc_cdb_t cdb = {{0, }};

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
  
  return run_scsi_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
				   scsi_mmc_get_cmd_len(cdb.field[0]), 
				   &cdb, SCSI_MMC_DATA_WRITE, 
				   sizeof(mh), &mh);
}

int
read_mode2_sector_freebsd_cam (_img_private_t *p_env, void *data, lsn_t lsn, 
			       bool b_form2)
{
  if ( b_form2 )
    return read_mode2_sectors_freebsd_cam(p_env, data, lsn, 1);
  else {
    /* Need to pick out the data portion from a mode2 form2 frame */
    char buf[M2RAW_SECTOR_SIZE] = { 0, };
    int retval = read_mode2_sectors_freebsd_cam(p_env, buf, lsn, 1);
    if ( retval ) return retval;
    memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    return 0;
  }
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
int
read_mode2_sectors_freebsd_cam (_img_private_t *p_env, void *p_buf, 
				lsn_t lsn, unsigned int nblocks)
{
  scsi_mmc_cdb_t cdb = {{0, }};

  bool b_read_10 = false;

  CDIO_MMC_SET_READ_LBA(cdb.field, lsn);
  
  if (b_read_10) {
    int retval;
    
    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_10);
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, nblocks);
    if ((retval = _set_bsize (p_env, M2RAW_SECTOR_SIZE)))
      return retval;
    
    if ((retval = run_scsi_cmd_freebsd_cam (p_env, 0, 
						scsi_mmc_get_cmd_len(cdb.field[0]), 
						&cdb, 
						SCSI_MMC_DATA_READ,
						M2RAW_SECTOR_SIZE * nblocks, 
						p_buf)))
      {
	_set_bsize (p_env, CDIO_CD_FRAMESIZE);
	return retval;
      }
    
    if ((retval = _set_bsize (p_env, CDIO_CD_FRAMESIZE)))
      return retval;
  } else
    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);
    CDIO_MMC_SET_READ_LENGTH24(cdb.field, nblocks);
    cdb.field[1] = 0; /* sector size mode2 */
    cdb.field[9] = 0x58; /* 2336 mode2 */
    return run_scsi_cmd_freebsd_cam (p_env, 0, 
					 scsi_mmc_get_cmd_len(cdb.field[0]), 
					 &cdb, 
					 SCSI_MMC_DATA_READ,
					 M2RAW_SECTOR_SIZE * nblocks, p_buf);

  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
uint32_t
stat_size_freebsd_cam (_img_private_t *p_env)
{
  scsi_mmc_cdb_t cdb = {{0, }};
  uint8_t buf[12] = { 0, };

  uint32_t retval;
  int i_status;

  /* Operation code */
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_TOC);

  cdb.field[1] = 0; /* lba; msf: 0x2 */

  /* Format */
  cdb.field[2] = CDIO_MMC_READTOC_FMT_TOC;

  CDIO_MMC_SET_START_TRACK(cdb.field, CDIO_CDROM_LEADOUT_TRACK);

  CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(buf));
  
  p_env->ccb.csio.data_ptr = buf;
  p_env->ccb.csio.dxfer_len = sizeof (buf);

  i_status = run_scsi_cmd_freebsd_cam(p_env, DEFAULT_TIMEOUT_MSECS,
					  scsi_mmc_get_cmd_len(cdb.field[0]), 
					  &cdb, SCSI_MMC_DATA_READ, 
					  sizeof(buf), buf);
  if (0 != i_status)
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

/*!
 * Eject using SCSI MMC commands. Return 0 if successful.
 */
int
eject_media_freebsd_cam (_img_private_t *p_env) 
{
  int i_status;
  scsi_mmc_cdb_t cdb = {{0, }};
  uint8_t buf[1];
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL);

  i_status = run_scsi_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
					   scsi_mmc_get_cmd_len(cdb.field[0]), 
					   &cdb, SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  cdb.field[4] = 1;
  i_status = run_scsi_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
				 scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
				 SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 2; /* eject */

  return run_scsi_cmd_freebsd_cam (p_env, DEFAULT_TIMEOUT_MSECS,
				       scsi_mmc_get_cmd_len(cdb.field[0]), 
				       &cdb, 
				       SCSI_MMC_DATA_WRITE, 0, &buf);
}

#endif /* HAVE_FREEBSD_CDROM */
