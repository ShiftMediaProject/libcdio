/*
  $Id: common_interface.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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

#ifndef _CDDA_COMMON_INTERFACE_H_
#define _CDDA_COMMON_INTERFACE_H_

#include "low_interface.h"

/* Test for presence of a cdrom by pinging with the 'CDROMVOLREAD' ioctl() */
extern int ioctl_ping_cdrom(int fd);

extern char *atapi_drive_info(int fd);
extern int data_bigendianp(cdrom_drive_t *d);
extern int FixupTOC(cdrom_drive_t *d,int tracks);

#endif /*_CDDA_COMMON_INTERFACE_H_*/
