/*
    $Id: freebsd_cam.c,v 1.2 2004/04/30 21:36:54 rocky Exp $

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

static const char _rcsid[] = "$Id: freebsd_cam.c,v 1.2 2004/04/30 21:36:54 rocky Exp $";

#ifdef HAVE_FREEBSD_CDROM

#include "freebsd.h"
#include "scsi_mmc.h"

static const u_char scsi_cdblen[8] = {6, 10, 10, 12, 12, 12, 10, 10};

static int
_scsi_cmd (_img_private_t * _obj)
{
  int retval;
  _obj->ccb.csio.cdb_len = scsi_cdblen[(_obj->ccb.csio.cdb_io.cdb_bytes[0] >> 5) & 7];
  if ((retval = cam_send_ccb(_obj->cam, &_obj->ccb)) < 0)
    {
      cdio_error ("transport failed: ", retval);
      return -1;
    }
  if ((_obj->ccb.ccb_h.status & CAM_STATUS_MASK) == CAM_REQ_CMP)
    {
      return 0;
    }
  errno = EIO;
  retval = ERRCODE(((unsigned char *)&_obj->ccb.csio.sense_data));
  if (retval == 0)
    retval = -1;
  else
    CREAM_ON_ERRNO(((unsigned char *)&_obj->ccb.csio.sense_data));
  cdio_error ("transport failed: ", retval);
  return retval;
}

bool
init_freebsd_cam (_img_private_t *_obj)
{
  char pass[100];
  
  _obj->cam=NULL;
  memset (&_obj->ccb, 0, sizeof(_obj->ccb));
  _obj->ccb.ccb_h.func_code = XPT_GDEVLIST;
  _obj->gen.fd = open (_obj->device, O_RDONLY, 0);

  if (_obj->gen.fd < 0)
    {
      cdio_error ("open (%s): %s", _obj->device, strerror (errno));
      return false;
    }

  if (ioctl (_obj->gen.fd, CAMGETPASSTHRU, &_obj->ccb) < 0)
    {
      cdio_error ("open: %s", strerror (errno));
      return false;
    }
  sprintf (pass,"/dev/%.15s%u",
	   _obj->ccb.cgdl.periph_name,
	   _obj->ccb.cgdl.unit_number);
  _obj->cam = cam_open_pass (pass,O_RDWR,NULL);
  _obj->gen.init   = true;
  _obj->b_cam_init = true;
  return true;
}

void
free_freebsd_cam (void *obj)
{
  _img_private_t *env = obj;

  if (NULL == env) return;

  free (env->device);

  if (env->gen.fd > 0)
    close (env->gen.fd);
  env->gen.fd = -1;

  if(env->cam)
    cam_close_device(env->cam);

  free (env);
}

static int
_set_bsize (_img_private_t *_obj, unsigned int bsize)
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

  memset(&_obj->ccb, 0, sizeof(_obj->ccb));
  memset (&mh, 0, sizeof (mh));

  _obj->ccb.ccb_h.path_id    = _obj->cam->path_id;
  _obj->ccb.ccb_h.target_id  = _obj->cam->target_id;
  _obj->ccb.ccb_h.target_lun = _obj->cam->target_lun;
  cam_fill_csio (&(_obj->ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(_obj->ccb.csio.sense_data), 0, 30*1000);
  _obj->ccb.csio.cdb_len = 4+1;
  
  
  _obj->ccb.csio.cdb_io.cdb_bytes[0] = 0x15;
  _obj->ccb.csio.cdb_io.cdb_bytes[1] = 1 << 4;
  _obj->ccb.csio.cdb_io.cdb_bytes[4] = 12;
  
  _obj->ccb.csio.data_ptr = (u_char *)&mh;
  _obj->ccb.csio.dxfer_len = sizeof (mh);;
  
  /* suc.suc_timeout = 500; */
  _obj->ccb.csio.ccb_h.flags = CAM_DIR_OUT;
  
  mh.block_desc_length = 0x08;
  mh.block_length_hi   = (bsize >> 16) & 0xff;
  mh.block_length_med  = (bsize >> 8) & 0xff;
  mh.block_length_lo   = (bsize >> 0) & 0xff;
  
  return _scsi_cmd (_obj);
}

int
read_mode2_sectors_freebsd_cam (_img_private_t *_obj, void *buf, uint32_t lba, 
				unsigned int nblocks, bool b_form2)
{
  int retval = 0;
  memset(&_obj->ccb,0,sizeof(_obj->ccb));
  _obj->ccb.ccb_h.path_id    = _obj->cam->path_id;
  _obj->ccb.ccb_h.target_id  = _obj->cam->target_id;
  _obj->ccb.ccb_h.target_lun = _obj->cam->target_lun;
  cam_fill_csio (&(_obj->ccb.csio), 1, NULL, 
		 CAM_DEV_QFRZDIS, MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(_obj->ccb.csio.sense_data), 0, 30*1000);
  _obj->ccb.csio.cdb_len = (b_form2?8:9)+1;
  
  _obj->ccb.csio.cdb_io.cdb_bytes[0] = (b_form2
					? CDIO_MMC_GPCMD_READ_10
					: CDIO_MMC_GPCMD_READ_CD );
  
  if (!b_form2)
    _obj->ccb.csio.cdb_io.cdb_bytes[1] = 0; /* sector size mode2 */
  
  CDIO_MMC_SET_READ_LBA(_obj->ccb.csio.cdb_io.cdb_bytes, lba);
  CDIO_MMC_SET_READ_LENGTH(_obj->ccb.csio.cdb_io.cdb_bytes, nblocks);
  
  if (!b_form2)
    _obj->ccb.csio.cdb_io.cdb_bytes[9] = 0x58; /* 2336 mode2 mixed form */
  
  _obj->ccb.csio.data_ptr = buf;
  _obj->ccb.csio.dxfer_len = M2RAW_SECTOR_SIZE * nblocks;
  
  /* suc.suc_timeout = 500; */
  _obj->ccb.csio.ccb_h.flags = CAM_DIR_IN;
  
  if (b_form2)
    {
      if ((retval = _set_bsize (_obj, M2RAW_SECTOR_SIZE)))
	goto out;
      
      if ((retval = _scsi_cmd(_obj)))
	{
	  _set_bsize (_obj, CDIO_CD_FRAMESIZE);
	  goto out;
	}
      retval = _set_bsize (_obj, CDIO_CD_FRAMESIZE);
    }
  else
    retval = _scsi_cmd(_obj);
  
 out:
  return retval;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
uint32_t
stat_size_freebsd_cam (_img_private_t *_obj)
{
  uint8_t buf[12] = { 0, };

  uint32_t retval;

  memset(&_obj->ccb,0,sizeof(_obj->ccb));
  _obj->ccb.ccb_h.path_id    = _obj->cam->path_id;
  _obj->ccb.ccb_h.target_id  = _obj->cam->target_id;
  _obj->ccb.ccb_h.target_lun = _obj->cam->target_lun;
  cam_fill_csio (&(_obj->ccb.csio), 1, NULL, CAM_DEV_QFRZDIS, 
		 MSG_SIMPLE_Q_TAG, NULL, 0, 
		 sizeof(_obj->ccb.csio.sense_data), 0, 30*1000);
  _obj->ccb.csio.cdb_len = 8+1;
  
  CDIO_MMC_SET_COMMAND(_obj->ccb.csio.cdb_io.cdb_bytes, CDIO_MMC_READ_TOC);
  _obj->ccb.csio.cdb_io.cdb_bytes[1] = 0; /* lba; msf: 0x2 */
  CDIO_MMC_SET_START_TRACK(_obj->ccb.csio.cdb_io.cdb_bytes, 
			   CDIO_CDROM_LEADOUT_TRACK);
  _obj->ccb.csio.cdb_io.cdb_bytes[8] = 12; /* ? */
  
  _obj->ccb.csio.data_ptr = buf;
  _obj->ccb.csio.dxfer_len = sizeof (buf);

  /*suc.suc_timeout = 500; */
  _obj->ccb.csio.ccb_h.flags = CAM_DIR_IN;

  if (_scsi_cmd(_obj))
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
eject_media_freebsd_cam (_img_private_t *_obj) 
{
  
  if (_obj->gen.fd == -1)
    return 2;
  
  memset(&_obj->ccb,0,sizeof(_obj->ccb));
  _obj->ccb.ccb_h.path_id    = _obj->cam->path_id;
  _obj->ccb.ccb_h.target_id  = _obj->cam->target_id;
  _obj->ccb.ccb_h.target_lun = _obj->cam->target_lun;
  cam_fill_csio (&(_obj->ccb.csio), 1, NULL, CAM_DEV_QFRZDIS, 
		 MSG_SIMPLE_Q_TAG, NULL, 0, sizeof(_obj->ccb.csio.sense_data), 
		 0, 30*1000);
  _obj->ccb.csio.cdb_len = 5+1;
  
  _obj->ccb.csio.cdb_io.cdb_bytes[0] = CDIO_MMC_START_STOP;
  _obj->ccb.csio.cdb_io.cdb_bytes[1] = 0x1;	/* immediate */
  _obj->ccb.csio.cdb_io.cdb_bytes[4] = 0x2;	/* eject */
  
  _obj->ccb.csio.data_ptr = 0;
  _obj->ccb.csio.dxfer_len = 0;
  
  /* suc.suc_timeout = 500; */
  _obj->ccb.csio.ccb_h.flags = CAM_DIR_IN;
  return(_scsi_cmd(_obj));
}

#endif /* HAVE_FREEBSD_CDROM */
