/*
    $Id: freebsd.h,v 1.6 2004/05/19 03:00:12 rocky Exp $

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

#include <cdio/sector.h>
#include "cdio_assert.h"
#include "cdio_private.h"

/*!
  For ioctl access /dev/acd0c is preferred over /dev/cd0c.
  For cam access /dev/cd0c is preferred. DEFAULT_CDIO_DEVICE and
  DEFAULT_FREEBSD_AM should be consistent. 
 */

#ifndef DEFUALT_CDIO_DEVICE
#define DEFAULT_CDIO_DEVICE "/dev/acd0c"
#endif

#ifndef DEFUALT_FREEBSD_AM
#define DEFAULT_FREEBSD_AM _AM_IOCTL
#endif

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
#include <sys/param.h>

#define HAVE_FREEBSD_CAM
#ifdef HAVE_FREEBSD_CAM
#include <camlib.h>

#include <cam/scsi/scsi_message.h>
#include <cam/scsi/scsi_pass.h>
#include <errno.h>
#define ERRCODE(s)	((((s)[2]&0x0F)<<16)|((s)[12]<<8)|((s)[13]))
#define EMEDIUMTYPE	EINVAL
#define	ENOMEDIUM	ENODEV
#define CREAM_ON_ERRNO(s)	do {			\
    switch ((s)[12])					\
    {	case 0x04:	errno=EAGAIN;	break;		\
	case 0x20:	errno=ENODEV;	break;		\
	case 0x21:	if ((s)[13]==0)	errno=ENOSPC;	\
			else		errno=EINVAL;	\
			break;				\
	case 0x30:	errno=EMEDIUMTYPE;  break;	\
	case 0x3A:	errno=ENOMEDIUM;    break;	\
    }							\
} while(0)
#endif /*HAVE_FREEBSD_CAM*/

#include <cdio/util.h>

#define TOTAL_TRACKS    ( _obj->tochdr.ending_track \
			- _obj->tochdr.starting_track + 1)
#define FIRST_TRACK_NUM (_obj->tochdr.starting_track)

typedef  enum {
  _AM_NONE,
  _AM_IOCTL,
  _AM_CAM
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  char *source_name;

#ifdef HAVE_FREEBSD_CAM
  char *device;
  struct cam_device  *cam;
  union ccb	      ccb;
#endif

  access_mode_t access_mode;

  bool b_ioctl_init;
  bool b_cam_init;
  
  /* Track information */
  bool toc_init;                         /* if true, info below is valid. */
  struct ioc_toc_header  tochdr;
  struct ioc_read_toc_single_entry tocent[100]; /* entry info for each track */

} _img_private_t;

bool cdio_is_cdrom_freebsd_ioctl(char *drive, char *mnttype);

char *get_mcn_freebsd_ioctl (const void *env);

track_format_t get_track_format_freebsd_ioctl(void *env, track_t track_num);
bool get_track_green_freebsd_ioctl(void *env, track_t track_num);

int  eject_media_freebsd_ioctl (_img_private_t *env);
int  eject_media_freebsd_cam (_img_private_t *env);

void free_freebsd_cam (void *obj);

int  read_audio_sectors_freebsd_ioctl (_img_private_t *env, void *data, 
				       lsn_t lsn, unsigned int nblocks);
int  read_mode2_sector_freebsd_ioctl (_img_private_t *env, void *data, 
				      lsn_t lsn, bool b_form2);

int  read_mode2_sectors_freebsd_cam (_img_private_t *env, void *buf, 
				     uint32_t lba, unsigned int nblocks, 
				     bool b_form2);

bool read_toc_freebsd_ioctl (_img_private_t *_obj);

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
uint32_t stat_size_freebsd_cam (_img_private_t *env);
uint32_t stat_size_freebsd_ioctl (_img_private_t *env);

bool init_freebsd_cam (_img_private_t *_obj);
void free_freebsd_cam (void *obj);

#endif /*HAVE_FREEBSD_CDROM*/
