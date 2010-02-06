/*
    Copyright (C) 2010 Rocky Bernstein <rocky@gnu.org>

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

/**
   \file mmc_cmds.h 
   
   \brief Wrappers for specific Multimedia Command (MMC) commands e.g., READ
   DISC, START/STOP UNIT.

   The documents we make use of are described in several
   specifications made by the SCSI committee T10
   http://www.t10.org. In particular, SCSI Primary Commands (SPC),
   SCSI Block Commands (SBC), and Multi-Media Commands (MMC). These
   documents generally have a numeric level number appended. For
   example SPC-3 refers to ``SCSI Primary Commands - 3'.

   In year 2010 the current versions were SPC-3, SBC-2, MMC-5.

*/

#ifndef __CDIO_MMC_CMDS_H__
#define __CDIO_MMC_CMDS_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /**
    Return results of media status
    @param p_cdio the CD object to be acted upon.
    @param out_buf media status code from operation
    @return DRIVER_OP_SUCCESS (0) if we got the status.
    return codes are the same as driver_return_code_t
   */
  driver_return_code_t mmc_get_event_status(const CdIo_t *p_cdio, 
                                            uint8_t out_buf[2]);


  /**
     Run a MODE_SENSE command (6- or 10-byte version) 
     and put the results in p_buf 
     @param p_cdio the CD object to be acted upon.
     @param p_buf pointer to location to store mode sense information
     @param i_size number of bytes allocated to p_buf
     @param page which "page" of the mode sense command we are interested in
     @return DRIVER_OP_SUCCESS if we ran the command ok.
  */
  int mmc_mode_sense( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                      int page);
  
  /**
     Run a MODE_SENSE command (10-byte version) 
     and put the results in p_buf 
     @param p_cdio the CD object to be acted upon.
     @param p_buf pointer to location to store mode sense information
     @param i_size number of bytes allocated to p_buf
     @param page which "page" of the mode sense command we are interested in
     @return DRIVER_OP_SUCCESS if we ran the command ok.
  */
  int  mmc_mode_sense_10( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                          int page);
  
  /**
      Run a MODE_SENSE command (6-byte version) 
      and put the results in p_buf 
      @param p_cdio the CD object to be acted upon.
      @param p_buf pointer to location to store mode sense information
      @param i_size number of bytes allocated to p_buf
      @param page which "page" of the mode sense command we are interested in
      @return DRIVER_OP_SUCCESS if we ran the command ok.
  */
  int  mmc_mode_sense_6( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                         int page);
  
  /**
      Issue a MMC READ_CD command.
    
      @param p_cdio  object to read from 
  
      @param p_buf   Place to store data. The caller should ensure that 
      p_buf can hold at least i_blocksize * i_blocks  bytes.
  
      @param i_lsn   sector to read 
  
      @param expected_sector_type restricts reading to a specific CD
      sector type.  Only 3 bits with values 1-5 are used:
      0 all sector types
      1 CD-DA sectors only 
      2 Mode 1 sectors only
      3 Mode 2 formless sectors only. Note in contrast to all other
        values an MMC CD-ROM is not required to support this mode.
      4 Mode 2 Form 1 sectors only
    5 Mode 2 Form 2 sectors only

    @param b_digital_audio_play Control error concealment when the
    data being read is CD-DA.  If the data being read is not CD-DA,
    this parameter is ignored.  If the data being read is CD-DA and
    DAP is false zero, then the user data returned should not be
    modified by flaw obscuring mechanisms such as audio data mute and
    interpolate.  If the data being read is CD-DA and DAP is true,
    then the user data returned should be modified by flaw obscuring
    mechanisms such as audio data mute and interpolate.  
    
    b_sync_header return the sync header (which will probably have
    the same value as CDIO_SECTOR_SYNC_HEADER of size
    CDIO_CD_SYNC_SIZE).
    
    @param header_codes Header Codes refer to the sector header and
    the sub-header that is present in mode 2 formed sectors: 
    
    0 No header information is returned.  
    1 The 4-byte sector header of data sectors is be returned, 
    2 The 8-byte sector sub-header of mode 2 formed sectors is
    returned.  
    3 Both sector header and sub-header (12 bytes) is returned.  
    The Header preceeds the rest of the bytes (e.g. user-data bytes) 
    that might get returned.
    
    @param b_user_data  Return user data if true. 
    
    For CD-DA, the User Data is CDIO_CD_FRAMESIZE_RAW bytes.
    
    For Mode 1, The User Data is ISO_BLOCKSIZE bytes beginning at
    offset CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE.
    
    For Mode 2 formless, The User Data is M2RAW_SECTOR_SIZE bytes
    beginning at offset CDIO_CD_HEADER_SIZE+CDIO_CD_SUBHEADER_SIZE.
    
    For data Mode 2, form 1, User Data is ISO_BLOCKSIZE bytes beginning at
    offset CDIO_CD_XA_SYNC_HEADER.
    
    For data Mode 2, form 2, User Data is 2 324 bytes beginning at
    offset CDIO_CD_XA_SYNC_HEADER.
    
    @param b_sync 
    
    @param b_edc_ecc true if we return EDC/ECC error detection/correction bits.
    
    The presence and size of EDC redundancy or ECC parity is defined
    according to sector type: 
    
    CD-DA sectors have neither EDC redundancy nor ECC parity.  
    
    Data Mode 1 sectors have 288 bytes of EDC redundancy, Pad, and
    ECC parity beginning at offset 2064.
    
    Data Mode 2 formless sectors have neither EDC redundancy nor ECC
    parity
    
    Data Mode 2 form 1 sectors have 280 bytes of EDC redundancy and
    ECC parity beginning at offset 2072
    
    Data Mode 2 form 2 sectors optionally have 4 bytes of EDC
    redundancy beginning at offset 2348.
    
    
    @param c2_error_information If true associate a bit with each
    sector for C2 error The resulting bit field is ordered exactly as
    the main channel bytes.  Each 8-bit boundary defines a byte of
    flag bits.
    
    @param subchannel_selection subchannel-selection bits
    
    0  No Sub-channel data shall be returned.  (0 bytes)
    1  RAW P-W Sub-channel data shall be returned.  (96 byte)
    2  Formatted Q sub-channel data shall be transferred (16 bytes)
    3  Reserved     
    4  Corrected and de-interleaved R-W sub-channel (96 bytes)
    5-7  Reserved
    
    @param i_blocksize size of the a block expected to be returned
     
    @param i_blocks number of blocks expected to be returned.
  */
  driver_return_code_t
  mmc_read_cd ( const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                int expected_sector_type, bool b_digital_audio_play,
                bool b_sync, uint8_t header_codes, bool b_user_data, 
                bool b_edc_ecc, uint8_t c2_error_information, 
                uint8_t subchannel_selection, uint16_t i_blocksize, 
                uint32_t i_blocks );
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __MMC_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
