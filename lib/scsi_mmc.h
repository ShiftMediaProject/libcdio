/*
    $Id: scsi_mmc.h,v 1.1 2003/09/14 09:36:32 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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

/* 
   This file contains common definitions/routines for SCSI MMC
   (Multimedia commands).
*/

#ifndef __SCSI_MMC_H__
#define __SCSI_MMC_H__

#define SCSI_MMC_SET_READ_LBA(rec, lba) \
  rec[2] = (lba >> 24) & 0xff; \
  rec[3] = (lba >> 16) & 0xff; \
  rec[4] = (lba >>  8) & 0xff; \
  rec[5] = (lba      ) & 0xff

#define SCSI_MMC_SET_READ_LENGTH(rec, len) \
  rec[6] = (len >> 16) & 0xff; \
  rec[7] = (len >>  8) & 0xff; \
  rec[8] = (len      ) & 0xff

#endif /* __SCSI_MMC_H__ */
