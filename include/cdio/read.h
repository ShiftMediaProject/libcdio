/* -*- c -*-
    $Id: read.h,v 1.4 2005/02/17 04:57:21 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file read.h 
 *
 *  \brief The top-level header for sector (block, frame)-related
 *  libcdio calls. 
 */

#ifndef __CDIO_READ_H__
#define __CDIO_READ_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /*!
    Reposition read offset
    Similar to (if not the same as) libc's lseek()

    @param p_cdio object to get information from
    @param offset amount to seek
    @param whence  like corresponding parameter in libc's lseek, e.g. 
                   SEEK_SET or SEEK_END.
    @return (off_t) -1 on error. 
  */

  off_t cdio_lseek(const CdIo_t *p_cdio, off_t offset, int whence);
    
  /*!
    Reads into buf the next size bytes.
    Similar to (if not the same as) libc's read()

    @param p_cdio object to read from
    @param p_buf place to read data into. The caller should make sure
                 this location can store at least i_size bytes.
    @param i_size number of bytes to read

    @return (ssize_t) -1 on error. 
  */
  ssize_t cdio_read(const CdIo_t *p_cdio, void *p_buf, size_t i_size);
    
  /*!
    Read an audio sector

    @param p_cdio object to read from
    @param p_buf place to read data into. The caller should make sure
                 this location can store at least CDIO_FRAMESIZE_RAW
		 bytes.
    @param i_lsn sector to read
  */
  driver_return_code_t cdio_read_audio_sector (const CdIo_t *p_cdio, 
					       void *p_buf, lsn_t i_lsn);

  /*!
    Reads audio sectors

    @param p_cdio object to read from
    @param p_buf place to read data into. The caller should make sure
                 this location can store at least CDIO_FRAMESIZE_RAW
		 * i_blocks bytes.
    @param i_lsn sector to read
    @param i_blocks number of sectors to read
  */
  driver_return_code_t cdio_read_audio_sectors (const CdIo_t *p_cdio, 
						void *p_buf, lsn_t i_lsn,
						uint32_t i_blocks);

  /*!
    Read a data sector

    @param p_cdio object to read from
    @param p_buf place to read data into.  The caller should make sure
                 this location can store at least ISO_BLOCKSIZE, 
		 M2RAW_SECTOR_SIZE, or M2F2_SECTOR_SIZE depending
		 on the kind of sector getting read. If you don't 
		 know whether you have a Mode 1/2, Form 1/ Form 2/Formless
		 sector best to reserve space for the maximum, 
		 M2RAW_SECTOR_SIZE.
    @param i_lsn sector to read
    @param i_blocksize size of block. Should be either CDIO_CD_FRAMESIZE, 
    M2RAW_SECTOR_SIZE, or M2F2_SECTOR_SIZE. See comment above under p_buf.
  */
  driver_return_code_t cdio_read_data_sector (const CdIo_t *p_cdio, 
					      void *p_buf, lsn_t i_lsn,
					      uint16_t i_blocksize
					      );

  /*!
    Reads a mode 1 sector

    @param p_cdio object to read from
    @param p_buf place to read data into.
    @param i_lsn sector to read
    @param b_form2 true for reading mode 1 form 2 sectors or false for 
    mode 1 form 1 sectors.
  */
  driver_return_code_t cdio_read_mode1_sector (const CdIo_t *p_cdio, 
					       void *p_buf, lsn_t i_lsn, 
					       bool b_form2);
  
  /*!
    Reads mode 1 sectors

    @param p_cdio object to read from
    @param p_buf place to read data into
    @param i_lsn sector to read
    @param b_form2 true for reading mode 1 form 2 sectors or false for 
    mode 1 form 1 sectors.
    @param i_blocks number of sectors to read
  */
  driver_return_code_t cdio_read_mode1_sectors (const CdIo_t *p_cdio, 
						void *p_buf, lsn_t i_lsn, 
						bool b_form2, 
						uint32_t i_blocks);
  
  /*!
    Reads a mode 2 sector

    @param p_cdio object to read from
    @param p_buf place to read data into. The caller should make sure
                 this location can store at least 
		 M2RAW_SECTOR_SIZE (for form 1) or CDIO_CD_FRAMESIZE (for 
		 form 2) bytes.
    @param i_lsn sector to read
    @param b_form2 true for reading mode 2 form 2 sectors or false for 
    mode 2 form 1 sectors.

    @return 0 if no error, nonzero otherwise.
  */
  driver_return_code_t cdio_read_mode2_sector (const CdIo_t *p_cdio, 
					       void *p_buf, lsn_t i_lsn, 
					       bool b_form2);
  
  /*!
    Reads mode 2 sectors

    @param p_cdio object to read from
    @param p_buf place to read data into. The caller should make sure
                 this location can store at least 
		 M2RAW_SECTOR_SIZE (for form 1) or CDIO_CD_FRAMESIZE (for 
		 form 2) * i_blocks bytes.
    @param i_lsn sector to read
    @param b_form2 true for reading mode2 form 2 sectors or false for 
           mode 2  form 1 sectors.
    @param i_blocks number of sectors to read

    @return 0 if no error, nonzero otherwise.
  */
  driver_return_code_t cdio_read_mode2_sectors (const CdIo_t *p_cdio, 
						void *p_buf, lsn_t i_lsn, 
						bool b_form2, 
						uint32_t i_blocks);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_TRACK_H__ */
