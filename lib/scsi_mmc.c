/*  Common MMC routines.

    $Id: scsi_mmc.c,v 1.1 2004/06/27 21:58:12 rocky Exp $

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

/*!
  On input A MODE_SENSE command was issued and we have the results
  in p. We interpret this and return a bit mask set according to the 
  capabilities.
 */
cdio_drive_cap_t
cdio_get_drive_cap_mmc(const uint8_t *p) 
{
  cdio_drive_cap_t i_drivetype=0;
  /* Reader */
  if (p[2] & 0x02) i_drivetype |= CDIO_DRIVE_CAP_CD_RW;
  if (p[2] & 0x08) i_drivetype |= CDIO_DRIVE_CAP_DVD;
  if (p[5] & 0x01) i_drivetype |= CDIO_DRIVE_CAP_CD_AUDIO;
  
  /* Writer */
  if (p[3] & 0x01) i_drivetype |= CDIO_DRIVE_CAP_CD_R;
  if (p[3] & 0x10) i_drivetype |= CDIO_DRIVE_CAP_DVD_R;
  if (p[3] & 0x20) i_drivetype |= CDIO_DRIVE_CAP_DVD_RAM;
  
  if (p[6] & 0x01) i_drivetype |= CDIO_DRIVE_CAP_LOCK;
  if (p[6] & 0x08) i_drivetype |= CDIO_DRIVE_CAP_OPEN_TRAY;
  if (p[6] >> 5 != 0) 
    i_drivetype |= CDIO_DRIVE_CAP_CLOSE_TRAY;
  return i_drivetype;
}
