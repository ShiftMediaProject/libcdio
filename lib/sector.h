/*
    $Id: sector.h,v 1.2 2003/03/29 17:32:00 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
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
   Things related to CDROM layout. Sector sizes, MSFs, LBAs, 
*/

#ifndef _CDIO_SECTOR_H_
#define _CDIO_SECTOR_H_

#include "cdio_types.h"

#define CDIO_PREGAP_SECTORS 150
#define CDIO_POSTGAP_SECTORS 150

/*! 
  Constant for ending or "leadout" track.
*/
#define CDIO_LEADOUT_TRACK  0xaa

/*
 * A CD-ROM physical sector size is 2048, 2052, 2056, 2324, 2332, 2336, 
 * 2340, or 2352 bytes long.  

*         Sector types of the standard CD-ROM data formats:
 *
 * format   sector type               user data size (bytes)
 * -----------------------------------------------------------------------------
 *   1     (Red Book)    CD-DA          2352    (CDDA_SECTOR_SIZE)
 *   2     (Yellow Book) Mode1 Form1    2048    (M1F1_SECTOR_SIZE)
 *   3     (Yellow Book) Mode1 Form2    2336    (M2RAW_SECTOR_SIZE)
 *   4     (Green Book)  Mode2 Form1    2048    (M2F1_SECTOR_SIZE)
 *   5     (Green Book)  Mode2 Form2    2328    (2324+4 spare bytes)
 *
 *
 *       The layout of the standard CD-ROM data formats:
 * -----------------------------------------------------------------------------
 * - audio (red):                  | audio_sample_bytes |
 *                                 |        2352        |
 *
 * - data (yellow, mode1):         | sync - head - data - EDC - zero - ECC |
 *                                 |  12  -   4  - 2048 -  4  -   8  - 276 |
 *
 * - data (yellow, mode2):         | sync - head - data |
 *                                 |  12  -   4  - 2336 |
 *
 * - XA data (green, mode2 form1): | sync - head - sub - data - EDC - ECC |
 *                                 |  12  -   4  -  8  - 2048 -  4  - 276 |
 *
 * - XA data (green, mode2 form2): | sync - head - sub - data - Spare |
 *                                 |  12  -   4  -  8  - 2324 -  4    |
 *
 */

#define CD_RAW_SECTOR_SIZE  2352
#define CDDA_SECTOR_SIZE    CD_RAW_SECTOR_SIZE
#define M1F1_SECTOR_SIZE    2048
#define M2F1_SECTOR_SIZE    2048
#define FORM1_DATA_SIZE     2048  /* Mode 1 or Mode 2 user data */
#define M2F2_SECTOR_SIZE    2324
#define M2SUB_SECTOR_SIZE   2332
#define M2RAW_SECTOR_SIZE   2336

#define CD_MAX_TRACKS 99 

#define CD_FRAMES_PER_SECOND 75
#define CD_FRAMES_PER_MINUTE (60*(CD_FRAMES_PER_SECOND))
#define CD_74MIN_SECTORS (UINT32_C(74)*CD_FRAMES_PER_MINUTE)
#define CD_80MIN_SECTORS (UINT32_C(80)*CD_FRAMES_PER_MINUTE)
#define CD_90MIN_SECTORS (UINT32_C(90)*CD_FRAMES_PER_MINUTE)
#define CD_MAX_SECTORS   (UINT32_C(100)*CD_FRAMES_PER_MINUTE-CDIO_PREGAP_SECTORS)

#define msf_t_SIZEOF 3

/* warning, returns new allocated string */
char *
cdio_lba_to_msf_str (lba_t lba);

static inline lba_t
cdio_lba_to_lsn (lba_t lba)
{
  return lba - CDIO_PREGAP_SECTORS; 
}

void
cdio_lba_to_msf(lba_t lba, msf_t *msf);

static inline lba_t
cdio_lsn_to_lba (lsn_t lsn)
{
  return lsn + CDIO_PREGAP_SECTORS; 
}

void
cdio_lsn_to_msf (lsn_t lsn, msf_t *msf);

uint32_t
cdio_msf_to_lba (const msf_t *msf);

uint32_t
cdio_msf_to_lsn (const msf_t *msf);

#endif /* _CDIO_SECTOR_H_ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
