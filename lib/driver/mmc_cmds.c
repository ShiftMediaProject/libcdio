/* 
   Wrappers for specific Multimedia Command (MMC) commands e.g., READ
   DISC, START/STOP UNIT.
   
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/mmc_cmds.h>
#include "cdio_private.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/* Boilerplate initialization code to setup running MMC command.  We
   assume variables 'p_cdio', 'p_buf', and 'i_size' are previously
   defined.  It does the following:

   1. Defines a cdb variable, 
   2  Checks to see if we have a cdio object and can run an MMC command
   3. zeros the buffer (p_buf) using i_size.
   4. Sets up the command field of cdb to passed in value mmc_cmd.
*/
#define MMC_CMD_SETUP(mmc_cmd)						\
    mmc_cdb_t cdb = {{0, }};						\
									\
    if ( ! p_cdio ) return DRIVER_OP_UNINIT;				\
    if ( ! p_cdio->op.run_mmc_cmd ) return DRIVER_OP_UNSUPPORTED;	\
									\
    memset (p_buf, 0, i_size);						\
    CDIO_MMC_SET_COMMAND(cdb.field, mmc_cmd)				

/* Boilerplate initialization code to setup running MMC read command
   needs to set the cdb 16-bit length field. See above
   comment for MMC_CMD_SETUP.
*/
#define MMC_CMD_SETUP_READ16(mmc_cmd)					\
    MMC_CMD_SETUP(mmc_cmd);						\
									\
    /* Setup to read header, to get length of data */			\
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, i_size)

/* Boilerplate code to run a MMC command. 

   We assume variables 'p_cdio', 'mmc_timeout_ms', 'cdb', 'i_size' and
   'p_buf' are defined previously.

   'direction' is the SCSI direction (read, write, none) of the
   command.  
*/
#define MMC_RUN_CMD(direction) \
    p_cdio->op.run_mmc_cmd(p_cdio->env,					\
	mmc_timeout_ms,							\
	mmc_get_cmd_len(cdb.field[0]),                                  \
        &cdb,								\
	direction, i_size, p_buf)


/**
  Return results of media status
  @param p_cdio the CD object to be acted upon.
  @return DRIVER_OP_SUCCESS (0) if we got the status.
  return codes are the same as driver_return_code_t
*/
driver_return_code_t 
mmc_get_event_status(const CdIo_t *p_cdio, uint8_t out_buf[2])
{
    uint8_t buf[8] = { 0, };
    void   *p_buf  = &buf; 
    const unsigned int i_size = sizeof(buf);
    driver_return_code_t i_status;
    
    MMC_CMD_SETUP_READ16(CDIO_MMC_GPCMD_GET_EVENT_STATUS);
    
    cdb.field[1] = 1;      /* We poll for info */
    cdb.field[4] = 1 << 4; /* We want Media events */
    
    i_status = MMC_RUN_CMD(SCSI_MMC_DATA_READ);
    if (i_status == DRIVER_OP_SUCCESS) {
	out_buf[0] = buf[4];
	out_buf[1] = buf[5];
    }
    return i_status;
}

/**
   Run a MODE_SENSE command (6- or 10-byte version) 
   and put the results in p_buf 
   @param p_cdio the CD object to be acted upon.
   @param p_buf pointer to location to store mode sense information
   @param i_size number of bytes allocated to p_buf
   @param page which "page" of the mode sense command we are interested in
   @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
driver_return_code_t
mmc_mode_sense( CdIo_t *p_cdio, /*out*/ void *p_buf, int i_size, 
                int page)
{
  /* We used to make a choice as to which routine we'd use based
     cdio_have_atapi(). But since that calls this in its determination,
     we had an infinite recursion. So we can't use cdio_have_atapi()
     (until we put in better capability checks.)
   */
    if ( DRIVER_OP_SUCCESS == mmc_mode_sense_6(p_cdio, p_buf, i_size, page) )
	return DRIVER_OP_SUCCESS;
    return mmc_mode_sense_10(p_cdio, p_buf, i_size, page);
}

/**
   Run a MODE_SENSE command (10-byte version) 
   and put the results in p_buf 
   @param p_cdio the CD object to be acted upon.
   @param p_buf pointer to location to store mode sense information
   @param i_size number of bytes allocated to p_buf
   @param page which "page" of the mode sense command we are interested in
   @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
driver_return_code_t
mmc_mode_sense_10( CdIo_t *p_cdio, void *p_buf, int i_size, int page)
{
    MMC_CMD_SETUP_READ16(CDIO_MMC_GPCMD_MODE_SENSE_10);
    cdb.field[2] = CDIO_MMC_ALL_PAGES & page;
    return MMC_RUN_CMD(SCSI_MMC_DATA_READ);
}

/**
   Run a MODE_SENSE command (6-byte version) 
   and put the results in p_buf 
   @param p_cdio the CD object to be acted upon.
   @param p_buf pointer to location to store mode sense information
   @param i_size number of bytes allocated to p_buf
   @param page which "page" of the mode sense command we are interested in
   @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
driver_return_code_t
mmc_mode_sense_6( CdIo_t *p_cdio, void *p_buf, int i_size, int page)
{
    MMC_CMD_SETUP(CDIO_MMC_GPCMD_MODE_SENSE_6);
    cdb.field[4] = i_size;
    
    cdb.field[2] = CDIO_MMC_ALL_PAGES & page;

    return MMC_RUN_CMD(SCSI_MMC_DATA_READ);
}

/* Maximum blocks to retrieve. Would be nice to customize this based on
   drive capabilities.
*/
#define MAX_CD_READ_BLOCKS 16
#define CD_READ_TIMEOUT_MS mmc_timeout_ms * (MAX_CD_READ_BLOCKS/2)

/**
   Issue a MMC READ_CD command.
   
   @param p_cdio  object to read from 
   
   @param p_buf1   Place to store data. The caller should ensure that 
   p_buf1 can hold at least i_blocksize * i_blocks  bytes.
   
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
mmc_read_cd(const CdIo_t *p_cdio, void *p_buf1, lsn_t i_lsn, 
	    int read_sector_type, bool b_digital_audio_play,
	    bool b_sync, uint8_t header_codes, bool b_user_data, 
	    bool b_edc_ecc, uint8_t c2_error_information, 
	    uint8_t subchannel_selection, uint16_t i_blocksize, 
	    uint32_t i_blocks)
{
    void *p_buf = p_buf1;
    uint8_t cdb9 = 0;
    const unsigned int i_size = i_blocksize * i_blocks;
    
    MMC_CMD_SETUP(CDIO_MMC_GPCMD_READ_CD);
    
    CDIO_MMC_SET_READ_TYPE(cdb.field, read_sector_type);
    if (b_digital_audio_play) cdb.field[1] |= 0x2;
    
    if (b_sync)      cdb9 |= 128;
    if (b_user_data) cdb9 |=  16;
    if (b_edc_ecc)   cdb9 |=   8;
    cdb9 |= (header_codes & 3)         << 5;
    cdb9 |= (c2_error_information & 3) << 1;
    cdb.field[9]  = cdb9;
    cdb.field[10] = (subchannel_selection & 7);
    
  {
      unsigned int j = 0;
      int i_status = DRIVER_OP_SUCCESS;
      
      while (i_blocks > 0) {
	  const unsigned i_blocks2 = (i_blocks > MAX_CD_READ_BLOCKS) 
	      ? MAX_CD_READ_BLOCKS : i_blocks;
	  
	  const unsigned int i_size = i_blocksize * i_blocks2;
	  
	  p_buf = ((char *)p_buf1 ) + (j * i_blocksize);
	  CDIO_MMC_SET_READ_LBA     (cdb.field, (i_lsn+j));
	  CDIO_MMC_SET_READ_LENGTH24(cdb.field, i_blocks2);
	  
	  i_status = MMC_RUN_CMD(SCSI_MMC_DATA_READ);
	  
	  if (i_status) return i_status;
	  
	  i_blocks -= i_blocks2;
	  j += i_blocks2;
      }
      return i_status;
  }
}

/**
   Set the drive speed in K bytes per second using SCSI-MMC SET SPEED.
   .
   
   @param p_cdio        CD structure set by cdio_open().
   @param i_Kbs_speed   speed in K bytes per second. Note this is 
                        not in standard CD-ROM speed units, e.g.
                        1x, 4x, 16x as it is in cdio_set_speed.
                        To convert CD-ROM speed units to Kbs,
                        multiply the number by 176 (for raw data)
                        and by 150 (for filesystem data). 
                        Also note that ATAPI specs say that a value
                        less than 176 will result in an error.
                        On many CD-ROM drives,
                        specifying a value too large will result in using
                        the fastest speed.

    @return the drive speed if greater than 0. -1 if we had an
    error. is -2 returned if this is not implemented for the current
    driver.

    @see cdio_set_speed and mmc_set_drive_speed
  */
int
mmc_set_speed(const CdIo_t *p_cdio, int i_Kbs_speed)

{
    uint8_t buf[14] = { 0, };
    void * p_buf = &buf;
    const unsigned int i_size = sizeof(buf);
    
    MMC_CMD_SETUP(CDIO_MMC_GPCMD_SET_SPEED);
    
    /* If the requested speed is less than 1x 176 kb/s this command
       will return an error - it's part of the ATAPI specs. Therefore, 
       test and stop early. */
    
    if ( i_Kbs_speed < 176 ) return -1;
    
    CDIO_MMC_SET_LEN16(cdb.field, 2, i_Kbs_speed);
    /* Some drives like the Creative 24x CDRW require one to set a
       nonzero write speed or else one gets an error back.  Some
       specifications have setting the value 0xfffff indicate setting to
       the maximum allowable speed.
    */
    CDIO_MMC_SET_LEN16(cdb.field, 4, 0xffff);
    return MMC_RUN_CMD(SCSI_MMC_DATA_WRITE);
}

/**
   Load or Unload media using a MMC START/STOP UNIT command. 
   
   @param p_cdio  the CD object to be acted upon.
   @param b_eject eject if true and close tray if false
   @param b_immediate wait or don't wait for operation to complete
   @param power_condition Set CD-ROM to idle/standby/sleep. If nonzero,
          eject/load is ignored, so set to 0 if you want to eject or load.
    
    @see mmc_eject_media or mmc_close_tray
*/
driver_return_code_t
mmc_start_stop_unit(const CdIo_t *p_cdio, bool b_eject, bool b_immediate,
		    uint8_t power_condition)
{
  uint8_t buf[1];
  void * p_buf = &buf;
  const unsigned int i_size = 0;

  MMC_CMD_SETUP_READ16(CDIO_MMC_GPCMD_START_STOP_UNIT);

  if (b_immediate) cdb.field[1] |= 1;

  if (power_condition) 
    cdb.field[4] = power_condition << 4;
  else {
    if (b_eject) 
      cdb.field[4] = 2; /* eject */
    else 
      cdb.field[4] = 3; /* close tray for tray-type */
  }

  return MMC_RUN_CMD(SCSI_MMC_DATA_WRITE);
}
