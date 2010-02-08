/* 
   "Higher-level" Multimedia Command (MMC) commands which build on
   the "lower-level" commands.
   
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

/**
   Eject using MMC commands. If CD-ROM is "locked" we'll unlock it.
   Command is not "immediate" -- we'll wait for the command to complete.
   For a more general (and lower-level) routine, @see mmc_start_stop_media.

   @param p_cdio the CD object to be acted upon.
   @return DRIVER_OP_SUCCESS (0) if we got the status.
   return codes are the same as driver_return_code_t
*/
driver_return_code_t
mmc_eject_media( const CdIo_t *p_cdio )
{
  int i_status = 0;
  i_status = mmc_prevent_allow_medium_removal(p_cdio, false, false, 0);
  if (0 != i_status) return i_status;

  return mmc_start_stop_unit(p_cdio, true, false, 0, 0);
  
}

/**
   Run a SCSI-MMC MMC MODE SENSE command (6- or 10-byte version) 
   and put the results in p_buf 
   @param p_cdio the CD object to be acted upon.
   @param p_buf pointer to location to store mode sense information
   @param i_size number of bytes allocated to p_buf
   @param page which "page" of the mode sense command we are interested in
   @return DRIVER_OP_SUCCESS if we ran the command ok.
*/
driver_return_code_t
mmc_mode_sense(CdIo_t *p_cdio, /*out*/ void *p_buf, unsigned int i_size, 
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
  Set the drive speed in CD-ROM speed units.
  
  @param p_cdio	   CD structure set by cdio_open().
  @param i_drive_speed   speed in CD-ROM speed units. Note this
                         not Kbytes/sec as would be used in the MMC spec or
	                 in mmc_set_speed(). To convert CD-ROM speed units 
		         to Kbs, multiply the number by 176 (for raw data)
		         and by 150 (for filesystem data). On many CD-ROM 
		         drives, specifying a value too large will result 
		         in using the fastest speed.

  @return the drive speed if greater than 0. -1 if we had an error. is -2
  returned if this is not implemented for the current driver.

   @see cdio_set_speed and mmc_set_speed
*/
driver_return_code_t 
mmc_set_drive_speed( const CdIo_t *p_cdio, int i_drive_speed )
{
  return mmc_set_speed(p_cdio, i_drive_speed * 176);
}
