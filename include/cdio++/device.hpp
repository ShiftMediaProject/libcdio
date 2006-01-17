/* -*- C++ -*-
    $Id: device.hpp,v 1.3 2006/01/17 02:09:32 rocky Exp $

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

/** \file device.hpp
 *
 *  \brief C++ header for driver- or device-related libcdio calls.  
 *         ("device" includes CD-image reading devices.)
 */

/*! 
  Free resources associated with CD-ROM Device/Image. After this we 
  must do another open before any more reading.
*/
bool 
close()
{
  cdio_destroy(p_cdio);
  p_cdio = (CdIo_t *) NULL;
  return true;
}

/*!
  Close media tray in CD drive if there is a routine to do so. 
  
  @param psz_drive the name of CD-ROM to be closed.
  @param driver_id is the driver to be used or that got used if
  it was DRIVER_UNKNOWN or DRIVER_DEVICE; If this is NULL, we won't
  report back the driver used.
*/
void closeTray (const char *psz_drive, /*in/out*/ driver_id_t &driver_id) 
{
  driver_return_code_t drc = cdio_close_tray (psz_drive, &driver_id);
  possible_throw_device_exception(drc);
}

/*!
  Close media tray in CD drive if there is a routine to do so. 
  
  @param psz_drive the name of CD-ROM to be closed.
*/
void closeTray (const char *psz_drive) 
{
  driver_id_t driver_id = DRIVER_UNKNOWN;
  closeTray(psz_drive, driver_id);
}


/*!
  Eject media in CD drive if there is a routine to do so. 
  
  If the CD is ejected, object is destroyed.
*/
void
ejectMedia () 
{
  driver_return_code_t drc = cdio_eject_media(&p_cdio);
  possible_throw_device_exception(drc);
}

/*!
  Eject media in CD drive if there is a routine to do so. 
  
  If the CD is ejected, object is destroyed.
*/
void
ejectMedia (const char *psz_drive) 
{
  driver_return_code_t drc = cdio_eject_media_drive(psz_drive);
  possible_throw_device_exception(drc);
}

/*! 
  Get a string decribing driver_id. 
  
  @param driver_id the driver you want the description for
  @return a sring of driver description
*/
const char *
driverDescribe (driver_id_t driver_id) 
{
  return cdio_driver_describe(driver_id);
}

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

/*!
    Get the value associatied with key. 
    
    @param key the key to retrieve
    @return the value associatd with "key" or NULL if p_cdio is NULL
    or "key" does not exist.
  */
const char * 
getArg (const char key[]) 
{
  return cdio_get_arg (p_cdio, key);
}

/*!  
  Return an opaque CdIo_t pointer for the given track object.
*/
CdIo_t *getCdIo()
{
  return p_cdio;
}

/*!  
  Return an opaque CdIo_t pointer for the given track object.
*/
cdtext_t *getCdtext(track_t i_track)
{
  return cdio_get_cdtext (p_cdio, i_track);
}

/*!
  Get the default CD device.
  
  @return a string containing the default CD device or NULL is
  if we couldn't get a default device.
  
  In some situations of drivers or OS's we can't find a CD device if
  there is no media in it and it is possible for this routine to return
  NULL even though there may be a hardware CD-ROM.
*/
char *
getDefaultDevice () 
{
  return cdio_get_default_device(p_cdio);
}

/*!
  Return a string containing the default CD device if none is specified.
  if p_driver_id is DRIVER_UNKNOWN or DRIVER_DEVICE
  then find a suitable one set the default device for that.
  
  NULL is returned if we couldn't get a default device.
*/
char *
getDefaultDevice(/*in/out*/ driver_id_t &driver_id) 
{
  return cdio_get_default_device_driver(&driver_id);
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
getDevices(driver_id_t driver_id) 
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
	   cdio_fs_anal_t capabilities, bool b_any) 
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
	   cdio_fs_anal_t capabilities, bool b_any,
	   /*out*/ driver_id_t &driver_id) 
{
  return cdio_get_devices_with_cap_ret(ppsz_search_devices, capabilities,
				       b_any, &driver_id);
}


/*!
  Get the what kind of device we've got.
  
  @param p_read_cap pointer to return read capabilities
  @param p_write_cap pointer to return write capabilities
  @param p_misc_cap pointer to return miscellaneous other capabilities
  
  In some situations of drivers or OS's we can't find a CD device if
  there is no media in it and it is possible for this routine to return
  NULL even though there may be a hardware CD-ROM.
*/
void 
getDriveCap (cdio_drive_read_cap_t  &read_cap,
	     cdio_drive_write_cap_t &write_cap,
	     cdio_drive_misc_cap_t  &misc_cap) 
{
  cdio_get_drive_cap(p_cdio, &read_cap, &write_cap, &misc_cap);
}

/*!
  Get the drive capabilities for a specified device.
  
  @return a list of device capabilities.
  
  In some situations of drivers or OS's we can't find a CD device if
  there is no media in it and it is possible for this routine to return
  NULL even though there may be a hardware CD-ROM.
*/
void 
getDriveCap (const char *device,
		   cdio_drive_read_cap_t  &read_cap,
		   cdio_drive_write_cap_t &write_cap,
		   cdio_drive_misc_cap_t  &misc_cap) 
{
  cdio_get_drive_cap(p_cdio, &read_cap, &write_cap, &misc_cap);
}

/*!
  Get a string containing the name of the driver in use.
  
  @return a string with driver name or NULL if CdIo_t is NULL (we
  haven't initialized a specific device.
*/
const char *
getDriverName () 
{
  return cdio_get_driver_name(p_cdio);
}

/*!
  Get the driver id. 
  if CdIo_t is NULL (we haven't initialized a specific device driver), 
  then return DRIVER_UNKNOWN.
  
  @return the driver id..
*/
driver_id_t 
getDriverId () 
{
  return cdio_get_driver_id(p_cdio);
}

/*! 
  Get the CD-ROM hardware info via a SCSI MMC INQUIRY command.
  False is returned if we had an error getting the information.
*/
bool 
getHWinfo ( /*out*/ cdio_hwinfo_t &hw_info ) 
{
  return cdio_get_hwinfo(p_cdio, &hw_info);
}

/*! Get the LSN of the first track of the last session of
  on the CD.
  
  @param i_last_session pointer to the session number to be returned.
*/
void
getLastSession (/*out*/ lsn_t &i_last_session) 
{
  driver_return_code_t drc = cdio_get_last_session(p_cdio, &i_last_session);
  possible_throw_device_exception(drc);
}

/*! 
  Find out if media has changed since the last call.
  @return 1 if media has changed since last call, 0 if not. Error
  return codes are the same as driver_return_code_t
*/
int 
getMediaChanged() 
{
  return cdio_get_media_changed(p_cdio);
}

/*! True if CD-ROM understand ATAPI commands. */
bool_3way_t 
haveATAPI ()
{
  return cdio_have_atapi(p_cdio);
}

/*! Like cdio_have_xxx but uses an enumeration instead. */
bool 
haveDriver (driver_id_t driver_id) 
{
  return cdio_have_driver(driver_id);
}

/*! 

  Sets up to read from the device specified by psz_source.  An open
  routine should be called before using any read routine. If device
  object was previously opened it is "closed".
  
  @return true if open succeeded or false if error.

*/
bool 
open(const char *psz_source)
{
  if (p_cdio) cdio_destroy(p_cdio);
  p_cdio = cdio_open_cd(psz_source);
  return NULL != p_cdio ;
}

/*! 
  Sets up to read from the device specified by psz_source and
  driver_id. An open routine should be called before using any read
  routine. If device object was previously opened it is "closed".
  
  @return true if open succeeded or false if error.
*/
bool 
open (const char *psz_source, driver_id_t driver_id) 
{
  if (p_cdio) cdio_destroy(p_cdio);
  p_cdio = cdio_open(psz_source, driver_id);
  return NULL != p_cdio ;
}

/*! 

  Sets up to read from the device specified by psz_source and access
  mode.  An open routine should be called before using any read
  routine. If device object was previously opened it is "closed".
  
  @return true if open succeeded or false if error.
*/
bool 
open (const char *psz_source, driver_id_t driver_id, 
	    const char *psz_access_mode) 
{
  if (p_cdio) cdio_destroy(p_cdio);
  p_cdio = cdio_open_am(psz_source, driver_id, psz_access_mode);
  return NULL != p_cdio ;
}

/*! 

Determine if bin_name is the bin file part of  a CDRWIN CD disk image.

@param bin_name location of presumed CDRWIN bin image file.
    @return the corresponding CUE file if bin_name is a BIN file or
    NULL if not a BIN file.
  */
char *
isBinFile(const char *bin_name)
{
  return cdio_is_binfile(bin_name);
}
  
/*! 
    Determine if cue_name is the cue sheet for a CDRWIN CD disk image.
    
    @return corresponding BIN file if cue_name is a CDRWIN cue file or
    NULL if not a CUE file.
  */
char *
isCueFile(const char *cue_name)
{
  return cdio_is_cuefile(cue_name);
}
  
/*! 
    Determine if psg_nrg is a Nero CD disk image.
    
    @param psz_nrg location of presumed NRG image file.
    @return true if psz_nrg is a Nero NRG image or false
    if not a NRG image.
*/
bool 
isNero(const char *psz_nrg)
{
  return cdio_is_nrg(psz_nrg);
}

/*! 
  Determine if psg_toc is a TOC file for a cdrdao CD disk image.
  
  @param psz_toc location of presumed TOC image file.
  @return true if toc_name is a cdrdao TOC file or false
  if not a TOC file.
*/
bool 
isTocFile(const char *psz_toc)
{
  return cdio_is_tocfile(psz_toc);
}
  
/*! 
    Determine if psz_source refers to a real hardware CD-ROM.
    
    @param psz_source location name of object
    @param driver_id   driver for reading object. Use DRIVER_UNKNOWN if you
    don't know what driver to use.
    @return true if psz_source is a device; If false is returned we
    could have a CD disk image. 
*/
bool 
isDevice(const char *psz_source, driver_id_t driver_id)
{
  return cdio_is_device(psz_source, driver_id);
}

/*!
  Set the blocksize for subsequent reads. 
*/
void
setBlocksize ( int i_blocksize )
{
  driver_return_code_t drc = cdio_set_blocksize ( p_cdio, i_blocksize );
  possible_throw_device_exception(drc);
}

/*!
    Set the drive speed. 
*/
void
setSpeed ( int i_speed ) 
{
  driver_return_code_t drc = cdio_set_speed ( p_cdio, i_speed );
  possible_throw_device_exception(drc);
}

/*!
    Set the arg "key" with "value" in "p_cdio".
    
    @param key the key to set
    @param value the value to assocaiate with key
*/
void
setArg (const char key[], const char value[]) 
{
  driver_return_code_t drc = cdio_set_arg (p_cdio, key, value);
  possible_throw_device_exception(drc);
}
  
