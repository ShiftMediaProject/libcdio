/* -*- C++ -*-
    $Id: devices.hpp,v 1.2 2006/01/25 06:36:07 rocky Exp $

    Copyright (C) 2005, 2006 Rocky Bernstein <rocky@panix.com>

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

/** \file devices.hpp
 *
 *  \brief methods relating to getting lists of devices. This file
 *  should not be #included directly.
 */
/*!
  Free device list returned by GetDevices
  
  @param device_list list returned by GetDevices
  
  @see GetDevices
  
*/
void 
freeDeviceList (char * device_list[]) 
{
  cdio_free_device_list(device_list);
}

/*! Return an array of device names. If you want a specific
  devices for a driver, give that device. If you want hardware
  devices, give DRIVER_DEVICE and if you want all possible devices,
  image drivers and hardware drivers give DRIVER_UNKNOWN.
  
  NULL is returned if we couldn't return a list of devices.
  
  In some situations of drivers or OS's we can't find a CD device if
  there is no media in it and it is possible for this routine to return
  NULL even though there may be a hardware CD-ROM.
*/
char ** 
getDevices(driver_id_t driver_id=DRIVER_DEVICE) 
{
  return cdio_get_devices(driver_id);
}

/*! Like GetDevices above, but we may change the p_driver_id if we
  were given DRIVER_DEVICE or DRIVER_UNKNOWN. This is because
  often one wants to get a drive name and then *open* it
  afterwards. Giving the driver back facilitates this, and speeds
  things up for libcdio as well.
*/

char **
getDevices (driver_id_t &driver_id) 
{
  return cdio_get_devices_ret(&driver_id);
}

/*!
  Get an array of device names in search_devices that have at least
  the capabilities listed by the capabities parameter.  If
  search_devices is NULL, then we'll search all possible CD drives.
  
  If "b_any" is set false then every capability listed in the
  extended portion of capabilities (i.e. not the basic filesystem)
  must be satisified. If "any" is set true, then if any of the
  capabilities matches, we call that a success.
  
  To find a CD-drive of any type, use the mask CDIO_FS_MATCH_ALL.
  
  @return the array of device names or NULL if we couldn't get a
  default device.  It is also possible to return a non NULL but
  after dereferencing the the value is NULL. This also means nothing
  was found.
*/
char ** 
getDevices(/*in*/ char *ppsz_search_devices[],
	   cdio_fs_anal_t capabilities, bool b_any=false) 
{
  return cdio_get_devices_with_cap(ppsz_search_devices, capabilities, b_any);
}

/*!
  Like GetDevices above but we return the driver we found
  as well. This is because often one wants to search for kind of drive
  and then *open* it afterwards. Giving the driver back facilitates this,
  and speeds things up for libcdio as well.
*/
char ** 
getDevices(/*in*/ char* ppsz_search_devices[],
	   cdio_fs_anal_t capabilities, /*out*/ driver_id_t &driver_id,
	   bool b_any=false) 
{
  return cdio_get_devices_with_cap_ret(ppsz_search_devices, capabilities,
				       b_any, &driver_id);
}
