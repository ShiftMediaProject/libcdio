/*  Common SCSI Multimedia Command (MMC) routines.

    $Id: scsi_mmc.c,v 1.9 2004/07/26 03:39:55 rocky Exp $

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/scsi_mmc.h>
#include "cdio_private.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*!
  On input a MODE_SENSE command was issued and we have the results
  in p. We interpret this and return a bit mask set according to the 
  capabilities.
 */
void
scsi_mmc_get_drive_cap(const uint8_t *p,
		       /*out*/ cdio_drive_read_cap_t  *p_read_cap,
		       /*out*/ cdio_drive_write_cap_t *p_write_cap,
		       /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  /* Reader */
  if (p[2] & 0x01) *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_R;
  if (p[2] & 0x02) *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_RW;
  if (p[2] & 0x08) *p_read_cap  |= CDIO_DRIVE_CAP_READ_DVD_ROM;
  if (p[4] & 0x01) *p_read_cap  |= CDIO_DRIVE_CAP_READ_AUDIO;
  if (p[5] & 0x01) *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_DA;
  if (p[5] & 0x10) *p_read_cap  |= CDIO_DRIVE_CAP_READ_C2_ERRS;
  
  /* Writer */
  if (p[3] & 0x01) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_R;
  if (p[3] & 0x02) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_RW;
  if (p[3] & 0x10) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_R;
  if (p[3] & 0x20) *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_RAM;
  if (p[4] & 0x80) *p_misc_cap  |= CDIO_DRIVE_CAP_WRITE_BURN_PROOF;

  /* Misc */
  if (p[4] & 0x40) *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_MULTI_SESSION;
  if (p[6] & 0x01) *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_LOCK;
  if (p[6] & 0x08) *p_misc_cap  |= CDIO_DRIVE_CAP_MISC_EJECT;
  if (p[6] >> 5 != 0) 
    *p_misc_cap |= CDIO_DRIVE_CAP_MISC_CLOSE_TRAY;
}

/*!  
  Return the number of length in bytes of the Command Descriptor
  buffer (CDB) for a given SCSI MMC command. The length will be 
  either 6, 10, or 12. 
*/
uint8_t
scsi_mmc_get_cmd_len(uint8_t scsi_cmd) 
{
  static const uint8_t scsi_cdblen[8] = {6, 10, 10, 12, 12, 12, 10, 10};
  return scsi_cdblen[((scsi_cmd >> 5) & 7)];
}

/*!
  Run a SCSI MMC command. 
 
  cdio	        CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  buf	        Buffer for data, both sending and receiving
  len	        Size of buffer
  e_direction	direction the transfer is to go
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return 0 if command completed successfully and 1 if not.
 */
int
scsi_mmc_run_cmd( const CdIo *cdio, int i_timeout, 
		  const scsi_mmc_cdb_t *p_cdb,
		  scsi_mmc_direction_t e_direction, unsigned int i_buf, 
		  /*in/out*/ void *p_buf )
{
  if (cdio && cdio->op.run_scsi_mmc_cmd) {
    return cdio->op.run_scsi_mmc_cmd(cdio, i_timeout,
				     scsi_mmc_get_cmd_len(p_cdb->field[0]), 
				     p_cdb, e_direction, i_buf, p_buf);
  } else 
    return 1;
}

#define DEFAULT_TIMEOUT_MS 6000
  
/*!
 * Eject using SCSI MMC commands. Return 0 if successful.
 */
int 
scsi_mmc_eject_media( const CdIo *cdio )
{
  int i_status;
  scsi_mmc_cdb_t cdb = {{0, }};
  uint8_t buf[1];
  scsi_mmc_run_cmd_fn_t run_scsi_mmc_cmd;

  if ( ! cdio || ! cdio->op.run_scsi_mmc_cmd )
    return -2;

  run_scsi_mmc_cmd = cdio->op.run_scsi_mmc_cmd;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_ALLOW_MEDIUM_REMOVAL);

  i_status = run_scsi_mmc_cmd (cdio->env, DEFAULT_TIMEOUT_MS,
			       scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
			       SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 1;
  i_status = run_scsi_mmc_cmd (cdio->env, DEFAULT_TIMEOUT_MS,
			       scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
			       SCSI_MMC_DATA_WRITE, 0, &buf);
  if (0 != i_status)
    return i_status;
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_START_STOP);
  cdb.field[4] = 2; /* eject */

  return run_scsi_mmc_cmd (cdio->env, DEFAULT_TIMEOUT_MS,
			   scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
			   SCSI_MMC_DATA_WRITE, 0, &buf);
  
}

/*! Packet driver to read mode2 sectors. 
   Can read only up to 25 blocks.
*/
int
scsi_mmc_read_sectors ( const CdIo *cdio, void *p_buf, lba_t lba, 
			int sector_type, unsigned int nblocks )
{
  scsi_mmc_cdb_t cdb = {{0, }};

  scsi_mmc_run_cmd_fn_t run_scsi_mmc_cmd;

  if ( ! cdio || ! cdio->op.run_scsi_mmc_cmd )
    return -2;

  run_scsi_mmc_cmd = cdio->op.run_scsi_mmc_cmd;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_CD);
  CDIO_MMC_SET_READ_TYPE  (cdb.field, sector_type);
  CDIO_MMC_SET_READ_LBA   (cdb.field, lba);
  CDIO_MMC_SET_READ_LENGTH(cdb.field, nblocks);
  CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(cdb.field, 
					   CDIO_MMC_MCSB_ALL_HEADERS);

  return run_scsi_mmc_cmd (cdio->env, DEFAULT_TIMEOUT_MS,
			   scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
			   SCSI_MMC_DATA_READ, 
			   CDIO_CD_FRAMESIZE_RAW * nblocks,
			   p_buf);
}

int
set_bsize_mmc ( const void *p_env, 
		const scsi_mmc_run_cmd_fn_t *run_scsi_mmc_cmd, 
		unsigned int bsize)
{
  scsi_mmc_cdb_t cdb = {{0, }};

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

  if ( ! p_env || ! run_scsi_mmc_cmd )
    return -2;

  memset (&mh, 0, sizeof (mh));
  mh.block_desc_length = 0x08;
  mh.block_length_hi   = (bsize >> 16) & 0xff;
  mh.block_length_med  = (bsize >>  8) & 0xff;
  mh.block_length_lo   = (bsize >>  0) & 0xff;

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SELECT_6);

  cdb.field[1] = 1 << 4;
  cdb.field[4] = 12;
  
  return (*run_scsi_mmc_cmd) (p_env, DEFAULT_TIMEOUT_MS,
			      scsi_mmc_get_cmd_len(cdb.field[0]), &cdb, 
			      SCSI_MMC_DATA_WRITE, sizeof(mh), &mh);
}

int 
scsi_mmc_set_bsize ( const CdIo *cdio, unsigned int bsize)
{
  if ( ! cdio )  return -2;
  return set_bsize_mmc (cdio->env, (&cdio->op.run_scsi_mmc_cmd), bsize);
}
