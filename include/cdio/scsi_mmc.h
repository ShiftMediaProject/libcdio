/*
    $Id: scsi_mmc.h,v 1.3 2004/05/26 06:29:15 rocky Exp $

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

/* 
   This file contains common definitions/routines for SCSI MMC
   (Multimedia commands).
*/

#ifndef __SCSI_MMC_H__
#define __SCSI_MMC_H__

/* Leval values that can go into READ_CD */
#define CDIO_MMC_READ_TYPE_ANY   0  /* All types */
#define CDIO_MMC_READ_TYPE_CDDA  1  /* Only CD-DA sectors */
#define CDIO_MMC_READ_TYPE_MODE1 2  /* Only mode1 sectors (user data = 2048) */
#define CDIO_MMC_READ_TYPE_MODE2 3  /* mode2 sectors form1 or form2 */
#define CDIO_MMC_READ_TYPE_M2F1  4  /* mode2 sectors form1 */
#define CDIO_MMC_READ_TYPE_M2F2  5  /* mode2 sectors form2 */

/*! The generic packet command opcodes for CD/DVD Logical Units. */

#define CDIO_MMC_MODE_SENSE 	        0x1a
#define CDIO_MMC_START_STOP             0x1b
#define CDIO_MMC_GPCMD_READ_10	        0x28
#define CDIO_MMC_GPCMD_READ_SUBCHANNEL	0x42
#define CDIO_MMC_READ_TOC               0x43
#define CDIO_MMC_MODE_SELECT	        0x55
#define CDIO_MMC_MODE_SELECT_6	        0x15
#define CDIO_MMC_MODE_SENSE_10	        0x5a
#define CDIO_MMC_GPCMD_READ_12	        0xa8
#define CDIO_MMC_GPCMD_READ_CD	        0xbe
#define CDIO_MMC_GPCMD_READ_MSF	        0xb9

/*! This is listed as optional in ATAPI 2.6, but is (curiously) 
  missing from Mt. Fuji, Table 57.  It _is_ mentioned in Mt. Fuji
  Table 377 as an MMC command for SCSi devices though...  Most ATAPI
  drives support it. */
#define CDIO_MMC_GPCMD_SET_SPEED	0xbb


#define CDIO_MMC_SET_COMMAND(rec, command) \
  rec[0] = command

#define CDIO_MMC_SET_READ_TYPE(rec, sector_type) \
  rec[1] = (sector_type << 2)
  

#define CDIO_MMC_SET_READ_LBA(rec, lba) \
  rec[2] = (lba >> 24) & 0xff; \
  rec[3] = (lba >> 16) & 0xff; \
  rec[4] = (lba >>  8) & 0xff; \
  rec[5] = (lba      ) & 0xff

#define CDIO_MMC_SET_START_TRACK(rec, command) \
  rec[6] = command

#define CDIO_MMC_SET_READ_LENGTH(rec, len) \
  rec[6] = (len >> 16) & 0xff; \
  rec[7] = (len >>  8) & 0xff; \
  rec[8] = (len      ) & 0xff

#define CDIO_MMC_MCSB_ALL_HEADERS 0x78

#define CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(rec, val) \
  rec[9] = val;

#endif /* __SCSI_MMC_H__ */
