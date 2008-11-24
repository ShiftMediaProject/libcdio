/* -*- c -*-
  $Id: compat.swg,v 1.3 2008/05/01 16:55:05 karl Exp $

  Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
%inline %{
/* When libcdio version > 0.76 comes out this won't be needed. */
#include <cdio/version.h>
#if LIBCDIO_VERSION_NUM <= 76

/**< Masks derived from above... */
#undef CDIO_DRIVE_CAP_WRITE_DVD
#define CDIO_DRIVE_CAP_WRITE_DVD (               \
      CDIO_DRIVE_CAP_WRITE_DVD_R                 \
    | CDIO_DRIVE_CAP_WRITE_DVD_PR                \
    | CDIO_DRIVE_CAP_WRITE_DVD_RAM               \
    | CDIO_DRIVE_CAP_WRITE_DVD_RW                \
    | CDIO_DRIVE_CAP_WRITE_DVD_RPW               \
    ) 

/** All the different ways a block/sector can be read. */
typedef enum {
  CDIO_READ_MODE_AUDIO,  /**< CD-DA, audio, Red Book */
  CDIO_READ_MODE_M1F1,   /**< Mode 1 Form 1 */
  CDIO_READ_MODE_M1F2,   /**< Mode 1 Form 2 */
  CDIO_READ_MODE_M2F1,   /**< Mode 2 Form 1 */
  CDIO_READ_MODE_M2F2,   /**< Mode 2 Form 2 */
} cdio_read_mode_t;

/*!
  Reads a number of sectors (AKA blocks).
  
  @param p_buf place to read data into. The caller should make sure
  this location is large enough. See below for size information.
  @param read_mode the kind of "mode" to use in reading.
  @param i_lsn sector to read
  @param i_blocks number of sectors to read
  @return DRIVER_OP_SUCCESS (0) if no error, other (negative) enumerations
  are returned on error.
  
  If read_mode is CDIO_MODE_AUDIO,
    *p_buf should hold at least CDIO_FRAMESIZE_RAW * i_blocks bytes.

  If read_mode is CDIO_MODE_DATA,
    *p_buf should hold at least i_blocks times either ISO_BLOCKSIZE, 
    M1RAW_SECTOR_SIZE or M2F2_SECTOR_SIZE depending on the kind of 
    sector getting read. If you don't know whether you have a Mode 1/2, 
    Form 1/ Form 2/Formless sector best to reserve space for the maximum
    which is M2RAW_SECTOR_SIZE.

  If read_mode is CDIO_MODE_M2F1,
    *p_buf should hold at least M2RAW_SECTOR_SIZE * i_blocks bytes.

  If read_mode is CDIO_MODE_M2F2,
    *p_buf should hold at least CDIO_CD_FRAMESIZE * i_blocks bytes.


*/
driver_return_code_t 
cdio_read_sectors(const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                  cdio_read_mode_t read_mode, uint32_t i_blocks)
{
  switch(read_mode) {
  case CDIO_READ_MODE_AUDIO:
    return cdio_read_audio_sectors (p_cdio, p_buf, i_lsn, i_blocks);
  case CDIO_READ_MODE_M1F1:
    return cdio_read_mode1_sectors (p_cdio, p_buf, i_lsn, false, i_blocks);
  case CDIO_READ_MODE_M1F2:
    return cdio_read_mode1_sectors (p_cdio, p_buf, i_lsn, true,  i_blocks);
  case CDIO_READ_MODE_M2F1:
    return cdio_read_mode2_sectors (p_cdio, p_buf, i_lsn, false, i_blocks);
  case CDIO_READ_MODE_M2F2:
    return cdio_read_mode2_sectors (p_cdio, p_buf, i_lsn, true,  i_blocks);
  }
  /* Can't happen. Just to shut up gcc. */
  return DRIVER_OP_ERROR; 
}

driver_return_code_t
cdio_eject_media_drive (const char *psz_drive)
{
  CdIo_t *p_cdio = cdio_open (psz_drive, DRIVER_DEVICE);
  if (p_cdio) {
    return cdio_eject_media(&p_cdio);
  } else {
    return DRIVER_OP_UNINIT;
  }
}
#endif /* LIBCDIO_VERSION_NUM <= 76 */
%}
