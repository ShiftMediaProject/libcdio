/*
    $Id: scsi_mmc.h,v 1.11 2004/07/21 11:07:27 rocky Exp $

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

/*!
   \file scsi_mmc.h 
   \brief Common definitions for SCSI MMC (Multimedia commands).
*/

#ifndef __SCSI_MMC_H__
#define __SCSI_MMC_H__

/*! The generic packet command opcodes for CD/DVD Logical Units. */

#define CDIO_MMC_GPCMD_MODE_SENSE 	     0x1a
#define CDIO_MMC_GPCMD_START_STOP            0x1b
#define CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL  0x1e
#define CDIO_MMC_GPCMD_READ_10	             0x28

/*!
  	Group 2 Commands
  */
#define CDIO_MMC_GPCMD_READ_SUBCHANNEL	     0x42
#define CDIO_MMC_GPCMD_READ_TOC              0x43
#define CDIO_MMC_GPCMD_READ_HEADER           0x44
#define CDIO_MMC_GPCMD_PLAYAUDIO10           0x45
#define CDIO_MMC_GPCMD_PLAYAUDIO_MSF         0x47
#define CDIO_MMC_GPCMD_PLAYAUDIO_TI          0x48
#define CDIO_MMC_GPCMD_PLAYTRACK_REL10       0x49
#define CDIO_MMC_GPCMD_PAUSE_RESUME          0x4b

#define CDIO_MMC_GPCMD_READ_DISC_INFO	     0x51
#define CDIO_MMC_GPCMD_MODE_SELECT	     0x55
#define CDIO_MMC_GPCMD_MODE_SELECT_6	     0x15
#define CDIO_MMC_GPCMD_MODE_SENSE_10	     0x5a

/*!
 	Group 5 Commands
 */
#define CDIO_MMC_GPCMD_PLAYAUDIO_12	     0xa5
#define CDIO_MMC_GPCMD_READ_12	             0xa8
#define CDIO_MMC_GPCMD_PLAYTRACK_REL12       0xa9
#define CDIO_MMC_GPCMD_READ_CD	             0xbe
#define CDIO_MMC_GPCMD_READ_MSF	             0xb9

/*!
 	Group 6 Commands
 */

#define	CDIO_MMC_GPCMD_CD_PLAYBACK_STATUS    0xc4 /**< SONY unique command */
#define	CDIO_MMC_GPCMD_PLAYBACK_CONTROL      0xc9 /**< SONY unique command */
#define	CDIO_MMC_GPCMD_READ_CDDA	     0xd8 /**< Vendor unique command */
#define	CDIO_MMC_GPCMD_READ_CDXA	     0xdb /**< Vendor unique command */
#define	CDIO_MMC_GPCMD_READ_ALL_SUBCODES     0xdf /**< Vendor unique command */



/*! Level values that can go into READ_CD */
#define CDIO_MMC_READ_TYPE_ANY   0  /**< All types */
#define CDIO_MMC_READ_TYPE_CDDA  1  /**< Only CD-DA sectors */
#define CDIO_MMC_READ_TYPE_MODE1 2  /**< mode1 sectors (user data = 2048) */
#define CDIO_MMC_READ_TYPE_MODE2 3  /**< mode2 sectors form1 or form2 */
#define CDIO_MMC_READ_TYPE_M2F1  4  /**< mode2 sectors form1 */
#define CDIO_MMC_READ_TYPE_M2F2  5  /**< mode2 sectors form2 */

/*! Format values for READ_TOC */
#define CDIO_MMC_READTOC_FMT_TOC      0
#define CDIO_MMC_READTOC_FMT_SESSION  1  
#define CDIO_MMC_READTOC_FMT_FULTOC   2  
#define CDIO_MMC_READTOC_FMT_PMA      3  /**< Q subcode data */
#define CDIO_MMC_READTOC_FMT_ATIP     4  /**< includes media type */
#define CDIO_MMC_READTOC_FMT_CDTEXT   5  /**< CD-TEXT info  */

/*! Page codes for MODE SENSE and MODE SET. */
#define CDIO_MMC_R_W_ERROR_PAGE		0x01
#define CDIO_MMC_WRITE_PARMS_PAGE	0x05
#define CDIO_MMC_AUDIO_CTL_PAGE		0x0e
#define CDIO_MMC_CDR_PARMS_PAGE		0x0d
#define CDIO_MMC_POWER_PAGE		0x1a
#define CDIO_MMC_FAULT_FAIL_PAGE	0x1c
#define CDIO_MMC_TO_PROTECT_PAGE	0x1d
#define CDIO_MMC_CAPABILITIES_PAGE	0x2a
#define CDIO_MMC_ALL_PAGES		0x3f


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

/*!
  On input a MODE_SENSE command was issued and we have the results
  in p. We interpret this and return a bit mask set according to the 
  capabilities.
 */
void cdio_get_drive_cap_mmc(const uint8_t *p,
			    /*out*/ cdio_drive_read_cap_t  *p_read_cap,
			    /*out*/ cdio_drive_write_cap_t *p_write_cap,
			    /*out*/ cdio_drive_misc_cap_t  *p_misc_cap);

#endif /* __SCSI_MMC_H__ */
