#!/usr/bin/python
#  $Id: cdio.py,v 1.6 2008/05/01 16:55:03 karl Exp $
#
#  Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""The CD Input and Control library (pycdio) encapsulates CD-ROM 
reading and control. Applications wishing to be oblivious of the OS-
and device-dependent properties of a CD-ROM can use this library."""

import pycdio
import types

class DeviceException(Exception):
    """General device or driver exceptions"""
        
class DriverError(DeviceException): pass
class DriverUnsupportedError(DeviceException): pass
class DriverUninitError(DeviceException): pass
class DriverNotPermittedError(DeviceException): pass
class DriverBadParameterError(DeviceException): pass
class DriverBadPointerError(DeviceException): pass
class NoDriverError(DeviceException): pass

class TrackError(DeviceException): pass

        
# Note: the keys below match those the names returned by
# cdio_get_driver_name()

drivers = {
    'Unknown'  : pycdio.DRIVER_UNKNOWN,
    'AIX'      : pycdio.DRIVER_AIX,
    'aix'      : pycdio.DRIVER_AIX,
    'BSDI'     : pycdio.DRIVER_BSDI,
    'bsdi'     : pycdio.DRIVER_BSDI,
    'FreeBSD'  : pycdio.DRIVER_FREEBSD,
    'freebsd'  : pycdio.DRIVER_FREEBSD,
    'GNU/Linux': pycdio.DRIVER_LINUX,
    'Solaris'  : pycdio.DRIVER_SOLARIS,
    'solaris'  : pycdio.DRIVER_SOLARIS,
    'OS X'     : pycdio.DRIVER_OSX,
    'WIN32'    : pycdio.DRIVER_WIN32,
    'CDRDAO'   : pycdio.DRIVER_CDRDAO,
    'cdrdao'   : pycdio.DRIVER_CDRDAO,
    'BIN/CUE'  : pycdio.DRIVER_BINCUE,
    'NRG'      : pycdio.DRIVER_NRG,
    'Nero'     : pycdio.DRIVER_NRG,
    'device'   : pycdio.DRIVER_DEVICE
    }

read_mode2blocksize = {
    pycdio.READ_MODE_AUDIO: pycdio.CD_FRAMESIZE_RAW,
    pycdio.READ_MODE_M1F1:  pycdio.M2RAW_SECTOR_SIZE,
    pycdio.READ_MODE_M1F2:  pycdio.CD_FRAMESIZE,
    pycdio.READ_MODE_M2F1:  pycdio.M2RAW_SECTOR_SIZE,
    pycdio.READ_MODE_M2F2:  pycdio.CD_FRAMESIZE
    }

def __possibly_raise_exception__(drc, msg=None):
    """Raise a Driver Error exception on error as determined by drc"""
    if drc==pycdio.DRIVER_OP_SUCCESS:
        return
    if drc==pycdio.DRIVER_OP_ERROR:
        raise DriverError
    if drc==pycdio.DRIVER_OP_UNINIT:
        raise DriverUninitError
    if drc==pycdio.DRIVER_OP_UNSUPPORTED:
        raise DriverUnsupportedError
    if drc==pycdio.DRIVER_OP_NOT_PERMITTED:
        raise DriverUnsupportedError
    if drc==pycdio.DRIVER_OP_BAD_PARAMETER:
        raise DriverBadParameterError
    if drc==pycdio.DRIVER_OP_BAD_POINTER:
        raise DriverBadPointerError
    if drc==pycdio.DRIVER_OP_NO_DRIVER:
        raise NoDriverError
    raise DeviceException('unknown exception %d' % drc)

def close_tray(drive=None, driver_id=pycdio.DRIVER_UNKNOWN):
    """close_tray(drive=None, driver_id=DRIVER_UNKNOWN) -> driver_id
    
    close media tray in CD drive if there is a routine to do so. 
    The driver id is returned. A DeviceException is thrown on error."""
    drc, found_driver_id = pycdio.close_tray(drive, driver_id)
    __possibly_raise_exception__(drc)
    return found_driver_id

def get_default_device_driver(driver_id=pycdio.DRIVER_DEVICE):
    """get_default_device_driver(self, driver_id=pycdio.DRIVER_DEVICE)
	->[device, driver]

    Return a string containing the default CD device if none is
    specified.  if driver_id is DRIVER_UNKNOWN or DRIVER_DEVICE
    then one set the default device for that.
    
    None is returned as the device if we couldn't get a default
    device."""
    result = pycdio.get_default_device_driver(driver_id)
    if type(result) == type([1,2]):
	return result
    return None

def get_devices(driver_id=pycdio.DRIVER_UNKNOWN):
    """
    get_devices(driver_id)->[device1, device2, ...]

    Get an list of device names.
    """
    result = pycdio.get_devices(driver_id)
    if type(result) == types.StringType:
        return [result]
    else:
        return result

def get_devices_ret(driver_id=pycdio.DRIVER_UNKNOWN):
    """
    get_devices_ret(driver_id)->[device1, device2, ... driver_id]

    Like get_devices, but return the p_driver_id which may be different
    from the passed-in driver_id if it was pycdio.DRIVER_DEVICE or
    pycdio.DRIVER_UNKNOWN. The return driver_id may be useful because
    often one wants to get a drive name and then *open* it
    afterwards. Giving the driver back facilitates this, and speeds things
    up for libcdio as well.
    """
    # FIXME: SWIG code is not removing a parameter properly, hence the [1:]
    # at the end
    return pycdio.get_devices_ret(driver_id)[1:]

def get_devices_with_cap(capabilities, any=False):
    """
    get_devices_with_cap(capabilities, any=False)->[device1, device2...]
    Get an array of device names in search_devices that have at least
    the capabilities listed by the capabities parameter.  
        
    If any is False then every capability listed in the
    extended portion of capabilities (i.e. not the basic filesystem)
    must be satisified. If any is True, then if any of the
    capabilities matches, we call that a success.

    To find a CD-drive of any type, use the mask pycdio.CDIO_FS_MATCH_ALL.

    The array of device names is returned or NULL if we couldn't get a
    default device.  It is also possible to return a non NULL but after
    dereferencing the the value is NULL. This also means nothing was
    found.
    """
    # FIXME: SWIG code is not removing a parameter properly, hence the [1:]
    # at the end
    return pycdio.get_devices_with_cap(capabilities, any)[1:]

def get_devices_with_cap_ret(capabilities, any=False):
    """
    get_devices_with_cap(capabilities, any=False)
      [device1, device2..., driver_id]

    Like cdio_get_devices_with_cap but we return the driver we found
    as well. This is because often one wants to search for kind of drive
    and then *open* it afterwards. Giving the driver back facilitates this,
      and speeds things up for libcdio as well.
    """
    # FIXME: SWIG code is not removing a parameter properly, hence the [1:]
    # at the end
    return pycdio.get_devices_with_cap_ret(capabilities, any)[1:]

def have_driver(driver_id):
    """
    have_driver(driver_id) -> bool
    
    Return True if we have driver driver_id.
    """
    if type(driver_id)==types.IntType:
        return pycdio.have_driver(driver_id)
    elif type(driver_id)==types.StringType and driver_id in drivers:
        ret = pycdio.have_driver(drivers[driver_id])
        if ret == 0: return False
        if ret == 1: return True
        raise ValueError('internal error: driver id came back %d' % ret)
    else:
        raise ValueError('need either a number or string driver id')
    
def is_binfile(binfile_name):
    """
    is_binfile(binfile_name)->cue_name
    
    Determine if binfile_name is the BIN file part of a CDRWIN CD
    disk image.
    
    Return the corresponding CUE file if bin_name is a BIN file or
    None if not a BIN file.
    """
    return pycdio.is_binfile(binfile_name)

def is_cuefile(cuefile_name):
    """
    is_cuefile(cuefile_name)->bin_name
    
    Determine if cuefile_name is the CUE file part of a CDRWIN CD
    disk image.
    
    Return the corresponding BIN file if bin_name is a CUE file or
    None if not a CUE file.
    """
    return pycdio.is_cuefile(cuefile_name)

def is_device(source, driver_id=pycdio.DRIVER_UNKNOWN):
    """
    is_device(source, driver_id=pycdio.DRIVER_UNKNOWN)->bool
    Return True if source refers to a real hardware CD-ROM.
    """
    if driver_id is None: driver_id=pycdio.DRIVER_UNKNOWN
    return pycdio.is_device(source, driver_id)

def is_nrg(nrgfile_name):
    """
    is_nrg(nrgfile_name)->bool
    
    Determine if nrgfile_name is a Nero CD disc image
    """
    return pycdio.is_nrg(nrgfile_name)

def is_tocfile(tocfile_name):
    """
    is_tocfile(tocfile_name)->bool
    
    Determine if tocfile_name is a cdrdao CD disc image
    """
    return pycdio.is_tocfile(tocfile_name)

def convert_drive_cap_misc(bitmask):
    """Convert bit mask for miscellaneous drive properties
    into a dictionary of drive capabilities"""
    result={}
    if bitmask & pycdio.DRIVE_CAP_ERROR:
        result['DRIVE_CAP_ERROR'] = True
    if bitmask & pycdio.DRIVE_CAP_UNKNOWN:
        result['DRIVE_CAP_UNKNOWN'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_CLOSE_TRAY:
        result['DRIVE_CAP_MISC_CLOSE_TRAY'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_EJECT:
        result['DRIVE_CAP_MISC_EJECT'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_LOCK:
        result['DRIVE_CAP_MISC_LOCK'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_SELECT_SPEED:
        result['DRIVE_CAP_MISC_SELECT_SPEED'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_SELECT_DISC:
        result['DRIVE_CAP_MISC_SELECT_DISC'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_MULTI_SESSION:
        result['DRIVE_CAP_MISC_MULTI_SESSION'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_MEDIA_CHANGED:
        result['DRIVE_CAP_MISC_MEDIA_CHANGED'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_RESET:
        result['DRIVE_CAP_MISC_RESET'] = True
    if bitmask & pycdio.DRIVE_CAP_MISC_FILE:
        result['DRIVE_CAP_MISC_FILE'] = True
    return result

def convert_drive_cap_read(bitmask):
    """Convert bit mask for drive read properties
    into a dictionary of drive capabilities"""
    result={}
    if bitmask & pycdio.DRIVE_CAP_READ_AUDIO:
        result['DRIVE_CAP_READ_AUDIO'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_CD_DA:
        result['DRIVE_CAP_READ_CD_DA'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_CD_G:
        result['DRIVE_CAP_READ_CD_G'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_CD_R:
        result['DRIVE_CAP_READ_CD_R'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_CD_RW:
        result['DRIVE_CAP_READ_CD_RW'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_DVD_R:
        result['DRIVE_CAP_READ_DVD_R'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_DVD_PR:
        result['DRIVE_CAP_READ_DVD_PR'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_DVD_RAM:
        result['DRIVE_CAP_READ_DVD_RAM'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_DVD_ROM:
        result['DRIVE_CAP_READ_DVD_ROM'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_DVD_RW:
        result['DRIVE_CAP_READ_DVD_RW'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_DVD_RPW:
        result['DRIVE_CAP_READ_DVD_RPW'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_C2_ERRS:
        result['DRIVE_CAP_READ_C2_ERRS'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_MODE2_FORM1:
        result['DRIVE_CAP_READ_MODE2_FORM1'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_MODE2_FORM2:
        result['DRIVE_CAP_READ_MODE2_FORM2'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_MCN:
        result['DRIVE_CAP_READ_MCN'] = True
    if bitmask & pycdio.DRIVE_CAP_READ_ISRC:
        result['DRIVE_CAP_READ_ISRC'] = True
    return result

def convert_drive_cap_write(bitmask):
    """Convert bit mask for drive write properties
    into a dictionary of drive capabilities"""
    result={}
    if bitmask & pycdio.DRIVE_CAP_WRITE_CD_R:
        result['DRIVE_CAP_WRITE_CD_R'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_CD_RW:
        result['DRIVE_CAP_WRITE_CD_RW'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_DVD_R:
        result['DRIVE_CAP_WRITE_DVD_R'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_DVD_PR:
        result['DRIVE_CAP_WRITE_DVD_PR'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_DVD_RAM:
        result['DRIVE_CAP_WRITE_DVD_RAM'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_DVD_RW:
        result['DRIVE_CAP_WRITE_DVD_RW'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_DVD_RPW:
        result['DRIVE_CAP_WRITE_DVD_RPW'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_MT_RAINIER:
        result['DRIVE_CAP_WRITE_MT_RAINIER'] = True
    if bitmask & pycdio.DRIVE_CAP_WRITE_BURN_PROOF:
        result['DRIVE_CAP_WRITE_BURN_PROOF'] = True
    return result

class Device:
    """CD Input and control class for discs/devices"""

    def __init__(self, source=None, driver_id=None,
                 access_mode=None):
        self.cd = None
        if source is not None or driver_id is not None:
            self.open(source, driver_id, access_mode)

    def audio_pause(self):
        """
        audio_pause(cdio)->status
        Pause playing CD through analog output.
        A DeviceError exception may be raised.
        """
        drc=pycdio.audio_pause(self.cd)
        __possibly_raise_exception__(drc)

    def audio_play_lsn(self, start_lsn, end_lsn):
        """
        auto_play_lsn(cdio, start_lsn, end_lsn)->status
        
        Playing CD through analog output at the given lsn to the ending lsn
        A DeviceError exception may be raised.
        """
        drc=pycdio.audio_play_lsn(self.cd, start_lsn, end_lsn)
        __possibly_raise_exception__(drc)

    def audio_resume(self):
        """
        audio_resume(cdio)->status
        Resume playing an audio CD through the analog interface.
        A DeviceError exception may be raised.
        """
        drc=pycdio.audio_resume(self.cd)
        __possibly_raise_exception__(drc)

    def audio_stop(self):
        """
        audio_stop(cdio)->status
        Stop playing an audio CD through the analog interface.
        A DeviceError exception may be raised.
        """
        drc=pycdio.audio_stop(self.cd)
        __possibly_raise_exception__(drc)

    def close(self):
        """close(self)
        Free resources associated with p_cdio.  Call this when done using
        using CD reading/control operations for the current device.
        """
        if self.cd is not None:
            pycdio.close(self.cd)
        else:
            print "***No object to close"
        self.cd=None

    def eject_media(self):
        """eject_media(self)
        Eject media in CD drive if there is a routine to do so.
        A DeviceError exception may be raised.
        """
        drc=pycdio.eject_media(self.cd)
        self.cd = None
        __possibly_raise_exception__(drc)

    ### FIXME: combine into above by testing if drive is the string
    ### None versus drive = pycdio.DRIVER_UNKNOWN
    def eject_media_drive(self, drive=None):
        """eject_media_drive(self, drive=None)
        Eject media in CD drive if there is a routine to do so.
        An exception is thrown on error."""
        pycdio.eject_media_drive(drive)

    def get_arg(self, key):
        """get_arg(self, key)->string
        Get the value associatied with key."""
        return pycdio.get_arg(self.cd, key)

    def get_device(self):
        """get_device(self)->str
        Get the default CD device.
        If we haven't initialized a specific device driver), 
        then find a suitable one and return the default device for that.
        In some situations of drivers or OS's we can't find a CD device if
        there is no media in it and it is possible for this routine to return
        None even though there may be a hardware CD-ROM."""
        if self.cd is not None:
            return pycdio.get_arg(self.cd, "source")
        return pycdio.get_device(self.cd)

    def get_disc_last_lsn(self):
        """
        get_disc_last_lsn(self)->int
        Get the LSN of the end of the CD
        
        DriverError and IOError may raised on error.
        """
        lsn = pycdio.get_disc_last_lsn(self.cd)
        if lsn == pycdio.INVALID_LSN:
            raise DriverError('Invalid LSN returned')
        return lsn

    def get_disc_mode(self):
        """
        get_disc_mode(p_cdio) -> str
        
        Get disc mode - the kind of CD (CD-DA, CD-ROM mode 1, CD-MIXED, etc.
        that we've got. The notion of 'CD' is extended a little to include
        DVD's.
        """
        return pycdio.get_disc_mode(self.cd)

    def get_drive_cap(self):
        """
        get_drive_cap(self)->(read_cap, write_cap, misc_cap)
        
        Get drive capabilities of device.
        
        In some situations of drivers or OS's we can't find a CD
        device if there is no media in it. In this situation
        capabilities will show up as empty even though there is a
        hardware CD-ROM.  get_drive_cap_dev()->(read_cap, write_cap,
        misc_cap)
        
        Get drive capabilities of device.
        
        In some situations of drivers or OS's we can't find a CD
        device if there is no media in it. In this situation
        capabilities will show up as empty even though there is a
        hardware CD-ROM."""

        b_read_cap, b_write_cap, b_misc_cap = pycdio.get_drive_cap(self.cd)
        return (convert_drive_cap_read(b_read_cap), \
                convert_drive_cap_write(b_write_cap), \
                convert_drive_cap_misc(b_misc_cap))

    ### FIXME: combine into above by testing on the type of device.
    def get_drive_cap_dev(self, device=None):
        b_read_cap, b_write_cap, b_misc_cap = \
                    pycdio.get_drive_cap_dev(device)
        return (convert_drive_cap_read(b_read_cap), \
                convert_drive_cap_write(b_write_cap), \
                convert_drive_cap_misc(b_misc_cap))

    def get_driver_name(self):
        """
        get_driver_name(self)-> string
        
        return a string containing the name of the driver in use.
        
        An IOError exception is raised on error.
        """
        return pycdio.get_driver_name(self.cd)

    def get_driver_id(self):
        """
        get_driver_id(self)-> int
        
        Return the driver id of the driver in use.
        if object has not been initialized or is None,
        return pycdio.DRIVER_UNKNOWN.
        """
        return pycdio.get_driver_id(self.cd)

    def get_first_track(self):
        """
        get_first_track(self)->Track

        return a Track object of the first track. None is returned 
        if there was a problem.
        """
        track = pycdio.get_first_track_num(self.cd)
        if track == pycdio.INVALID_TRACK:
            return None
        return Track(self.cd, track)

    def get_hwinfo(self):
        """
        get_hwinfo(self)->[vendor, model, release]
        Get the CD-ROM hardware info via a SCSI MMC INQUIRY command.
        """
        return pycdio.get_hwinfo(self.cd)

    def get_joliet_level(self):
        """
        get_joliet_level(self)->int
        
        Return the Joliet level recognized for cdio.
        This only makes sense for something that has an ISO-9660
        filesystem.
        """
        return pycdio.get_joliet_level(self.cd)

    def get_last_session(self):
        """get_last_session(self) -> int
        Get the LSN of the first track of the last session of on the CD.
        An exception is thrown on error."""
        drc, session = pycdio.get_last_session(self.cd)
        __possibly_raise_exception__(drc)
        return session

    def get_last_track(self):
        """
        get_last_track(self)->Track

        return a Track object of the first track. None is returned 
        if there was a problem.
        """
        track = pycdio.get_last_track_num(self.cd)
        if track == pycdio.INVALID_TRACK:
            return None
        return Track(self.cd, track)

    def get_mcn(self):
        """
        get_mcn(self) -> str
        
        Get the media catalog number (MCN) from the CD.
        """
        return pycdio.get_mcn(self.cd)

    def get_media_changed(self):
        """
        get_media_changed(self) -> bool
        
        Find out if media has changed since the last call.
        Return True if media has changed since last call. An exception
        Error is given on error.
        """
        drc = pycdio.get_media_changed(self.cd)
        if drc == 0: return False
        if drc == 1: return True
        __possibly_raise_exception__(drc)
        raise DeviceException('Unknown return value %d' % drc)

    def get_num_tracks(self):
        """
        get_num_tracks(self)->int
        
        Return the number of tracks on the CD. 
        A TrackError or IOError exception may be raised on error.
        """
        track = pycdio.get_num_tracks(self.cd)
        if track == pycdio.INVALID_TRACK:
            raise TrackError('Invalid track returned')
        return track

    def get_track(self, track_num):
        """
        get_track(self, track_num)->track
        
        Return a track object for the given track number.
        """
        return Track(self.cd, track_num)

    def get_track_for_lsn(self, lsn):
        """
        get_track_for_lsn(self, lsn)->Track
        
        Find the track which contains lsn.
        None is returned if the lsn outside of the CD or
        if there was some error. 
        
        If the lsn is before the pregap of the first track,
        A track object with a 0 track is returned.
        Otherwise we return the track that spans the lsn.
        """
        track = pycdio.get_last_track_num(self.cd)
        if track == pycdio.INVALID_TRACK:
            return None
        return Track(self.cd, track)

    def have_ATAPI(self):
        """have_ATAPI(self)->bool
        return True if CD-ROM understand ATAPI commands."""
        return pycdio.have_ATAPI(self.cd)

    def lseek(self, offset, whence):
        """
        lseek(self, offset, whence)->int
        Reposition read offset
        Similar to (if not the same as) libc's fseek()
        
        cdio is object to get adjested, offset is amount to seek and 
        whence is like corresponding parameter in libc's lseek, e.g. 
        it should be SEEK_SET or SEEK_END.
        
        the offset is returned or -1 on error.
        """
        return pycdio.lseek(self.cd, offset, whence)

    def open(self, source=None, driver_id=pycdio.DRIVER_UNKNOWN,
             access_mode=None):
        """
        open(self, source=None, driver_id=pycdio.DRIVER_UNKNOWN,
             access_mode=None)
        
        Sets up to read from place specified by source, driver_id and
        access mode. This should be called before using any other routine
        except those that act on a CD-ROM drive by name. 
        
        If None is given as the source, we'll use the default driver device.
        If None is given as the driver_id, we'll find a suitable device driver.

        If device object was, previously opened it is closed first.
        
        Device is opened so that subsequent operations can be performed.

        """
        if driver_id is None: driver_id=pycdio.DRIVER_UNKNOWN
        if self.cd is not None:
            self.close()
        self.cd = pycdio.open_cd(source, driver_id, access_mode)

    def read(self, size):
        """
        read(self, size)->[size, data]
        
        Reads the next size bytes.
        Similar to (if not the same as) libc's read()
        
        The number of bytes read and the data is returned. 
        A DeviceError exception may be raised.
        """
        size, data = pycdio.read_cd(self.cd, size)
        __possibly_raise_exception__(size)
        return [size, data]
    
    def read_data_blocks(self, lsn, blocks=1):
        """
        read_data_blocks(blocks, lsn, blocks=1)->[size, data]
        
        Reads a number of data sectors (AKA blocks).
        
        lsn is sector to read, bytes is the number of bytes.
        A DeviceError exception may be raised.
        """
        size = pycdio.ISO_BLOCKSIZE*blocks
        size, data = pycdio.read_data_bytes(self.cd, size, lsn,
                                            pycdio.ISO_BLOCKSIZE)
        if size < 0:
             __possibly_raise_exception__(size)
        return [size, data]
    
    def read_sectors(self, lsn, read_mode, blocks=1):
        """
        read_sectors(self, lsn, read_mode, blocks=1)->[blocks, data]
        Reads a number of sectors (AKA blocks).
        
        lsn is sector to read, bytes is the number of bytes.
        
        If read_mode is pycdio.MODE_AUDIO, the return buffer size will be
        truncated to multiple of pycdio.CDIO_FRAMESIZE_RAW i_blocks bytes.
        
        If read_mode is pycdio.MODE_DATA, buffer will be truncated to a
        multiple of pycdio.ISO_BLOCKSIZE, pycdio.M1RAW_SECTOR_SIZE or
        pycdio.M2F2_SECTOR_SIZE bytes depending on what mode the data is in.

        If read_mode is pycdio.MODE_M2F1, buffer will be truncated to a 
        multiple of pycdio.M2RAW_SECTOR_SIZE bytes.
        
        If read_mode is pycdio.MODE_M2F2, the return buffer size will be
        truncated to a multiple of pycdio.CD_FRAMESIZE bytes.
        
        The number of bytes read and the data is returned. 
        A DeviceError exception may be raised.
        """
        try:
            blocksize = read_mode2blocksize[read_mode]
            size = blocks * blocksize
        except KeyError:
            raise DriverBadParameterError ('Bad read mode %d' % read_mode)
        size, data = pycdio.read_sectors(self.cd, size, lsn, read_mode)
        if size < 0:
            __possibly_raise_exception__(size)
        blocks = size / blocksize
        return [blocks, data]

    def set_blocksize(self, blocksize):
        """set_blocksize(self, blocksize)
        Set the blocksize for subsequent reads.
        An exception is thrown on error.
        """
        drc = pycdio.set_blocksize(self.cd, blocksize)
        __possibly_raise_exception__(drc)

    def set_speed(self, speed):
        """set_speed(self, speed)
        Set the drive speed. An exception is thrown on error."""
        drc = pycdio.set_speed(self.cd, speed)
        __possibly_raise_exception__(drc)

class Track:
    """CD Input and control track class"""

    def __init__(self, device, track_num):

        if type(track_num) != types.IntType:
            raise TrackError('track number parameter is not an integer')
        self.track = track_num

        # See if the device parameter is a string or
        # a device object.
        if type(device) == types.StringType:
            self.device = Device(device)
        else:
            test_device=Device()
            ## FIXME: would like a way to test if device
            ## is a PySwigObject 
            self.device = device

    def get_audio_channels(self):
        """
        get_audio_channels(self, track)->int
        
        Return number of channels in track: 2 or 4
        Not meaningful if track is not an audio track.
        An exception can be raised on error.
        """
        channels = pycdio.get_track_channels(self.device, self.track)
        if -2 == channels:
            raise DriverUnsupportedError
        elif -1 == channels:
            raise TrackError
        else:
            return channels
    
    def get_copy_permit(self):
        """
        get_copy_permit(self, track)->int
        
        Return copy protection status on a track. Is this meaningful 
        not an audio track?
        
        """
        if pycdio.get_track_copy_permit(self.device, self.track):
            return "OK"
        else:
            return "no"

    def get_format(self):
        """
        get_format(self)->format
        
        Get the format (e.g. 'audio', 'mode2', 'mode1') of track. 
        """
        return pycdio.get_track_format(self.device, self.track)

    def get_last_lsn(self):
        """
        get_last_lsn(self)->lsn
        
        Return the ending LSN for a track 
        A TrackError or IOError exception may be raised on error.
        """
        lsn = pycdio.get_track_last_lsn(self.device, self.track)
        if lsn == pycdio.INVALID_LSN:
            raise TrackError('Invalid LSN returned')
        return lsn

    def get_lba(self):
        """
        get_lsn(self)->lba
        
        Return the starting LBA for a track
        A TrackError exception is raised on error.
        """
        lba = pycdio.get_track_lba(self.device, self.track)
        if lba == pycdio.INVALID_LBA:
            raise TrackError('Invalid LBA returned')
        return lba

    def get_lsn(self):
        """
        get_lsn(self)->lsn
        
        Return the starting LSN for a track
        A TrackError exception is raised on error.
        """
        lsn = pycdio.get_track_lsn(self.device, self.track)
        if lsn == pycdio.INVALID_LSN:
            raise TrackError('Invalid LSN returned')
        return lsn

    def get_msf(self):
        """
        get_msf(self)->str

        Return the starting MSF (minutes/secs/frames) for track number track.
        Track numbers usually start at something greater than 0, usually 1.
        
        Returns string of the form mm:ss:ff if all good, or string None on
        error.
        """
        return pycdio.get_track_msf(self.device, self.track)

    def get_preemphasis(self):
        """
        get_preemphaisis(self)->result
        
        Get linear preemphasis status on an audio track.
        This is not meaningful if not an audio track?
        A TrackError exception is raised on error.
        """
        rc = pycdio.get_track_preemphasis(self.device, self.track)
        if rc == pycdio.TRACK_FLAG_FALSE:
            return 'none'
        elif rc == pycdio.TRACK_FLAG_TRUE:
            return 'preemphasis'
        elif rc == pycdio.TRACK_FLAG_UNKNOWN:
            return 'unknown'
        else:
            raise TrackError('Invalid return value %d' % d)

    def get_track_sec_count(self):
        """
        get_track_sec_count(self)->int
        
        Get the number of sectors between this track an the next.  This
        includes any pregap sectors before the start of the next track.
        Track numbers usually start at something 
        greater than 0, usually 1.
        
        A TrackError exception is raised on error.
        """
        sec_count = pycdio.get_track_sec_count(self.device, self.track)
        if sec_count == 0:
            raise TrackError
        return sec_count

    def is_green(self):
        """
        is_track_green(self, track) -> bool
        
        Return True if we have XA data (green, mode2 form1) or
        XA data (green, mode2 form2). That is track begins:
        sync - header - subheader
        12     4      -  8
        """
        return pycdio.is_track_green(self.device, self.track)

    def set_track(self, track_num):
        """
        set_track(self, track_num)
        
        Set a new track number.
        """
        self.track = track_num


#
# Local variables:
#  mode: Python
# End:
