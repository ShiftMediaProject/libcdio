#!/usr/bin/env ruby
# $Id: cdio.rb,v 1.1 2006/11/13 05:12:42 rocky Exp $
#
#    Copyright (C) 2006 Rocky Bernstein <rocky@gnu.org>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#    02110-1301 USA.
#

# The CD Input and Control library (pycdio) encapsulates CD-ROM 
# reading and control. Applications wishing to be oblivious of the OS-
# and device-dependent properties of a CD-ROM can use this library.

require "rubycdio"

# General device or driver exceptions
class DeviceException < Exception
end

class DriverError < DeviceException; end
class DriverUnsupportedError < DeviceException; end
class DriverUninitError < DeviceException; end
class DriverNotPermittedError < DeviceException; end
class DriverBadParameterError < DeviceException; end
class DriverBadPointerError < DeviceException; end
class NoDriverError < DeviceException; end

class TrackError < DeviceException; end


# Note: the keys below match those the names returned by
# cdio_get_driver_name()
def drivers()
  return {
    "Unknown"   => Rubycdio::DRIVER_UNKNOWN,
    "AIX"       => Rubycdio::DRIVER_AIX,
    "BSDI"      => Rubycdio::DRIVER_BSDI,
    "FreeBSD"   => Rubycdio::DRIVER_FREEBSD,
    "GNU/Linux" => Rubycdio::DRIVER_LINUX,
    "Solaris"   => Rubycdio::DRIVER_SOLARIS,
    "OS X"      => Rubycdio::DRIVER_OSX,
    "WIN32"     => Rubycdio::DRIVER_WIN32,
    "CDRDAO"    => Rubycdio::DRIVER_CDRDAO,
    "BINCUE"    => Rubycdio::DRIVER_BINCUE,
    "NRG"       => Rubycdio::DRIVER_NRG,
    "device" => Rubycdio::DRIVER_DEVICE
  }
end


read_mode2blocksize = {
  :READ_MODE_AUDIO => Rubycdio::CD_FRAMESIZE_RAW,
  :READ_MODE_M1F1  =>  Rubycdio::M2RAW_SECTOR_SIZE,
  :READ_MODE_M1F2  =>  Rubycdio::CD_FRAMESIZE,
  :READ_MODE_M2F1  =>  Rubycdio::M2RAW_SECTOR_SIZE,
  :READ_MODE_M2F2  =>  Rubycdio::CD_FRAMESIZE
}


# Raise a Driver Error exception on error as determined by drc
def __possibly_raise_exception__(drc, msg=nil)
  if drc==Rubycdio::DRIVER_OP_SUCCESS
    return
  end
  if drc==Rubycdio::DRIVER_OP_ERROR
    raise DriverError
  end
  if drc==Rubycdio::DRIVER_OP_UNINIT
    raise DriverUninitError
  end
  if drc==Rubycdio::DRIVER_OP_UNSUPPORTED
      raise DriverUnsupportedError
  end
  if drc==Rubycdio::DRIVER_OP_NOT_PERMITTED
      raise DriverUnsupportedError
  end
  if drc==Rubycdio::DRIVER_OP_BAD_PARAMETER
      raise DriverBadParameterError
  end
  if drc==Rubycdio::DRIVER_OP_BAD_POINTER
      raise DriverBadPointerError
  end
  if drc==Rubycdio::DRIVER_OP_NO_DRIVER
      raise NoDriverError
  end
  raise DeviceException('unknown exception %d' % drc)
end

# close_tray(drive=nil, driver_id=DRIVER_UNKNOWN) -> driver_id
#    
#    close media tray in CD drive if there is a routine to do so. 
#    The driver id is returned. A DeviceException is thrown on error.
def close_tray(drive=nil, driver_id=Rubycdio::DRIVER_UNKNOWN)
    drc, found_driver_id = Rubycdio::close_tray(drive, driver_id)
  __possibly_raise_exception__(drc)
  return found_driver_id
end

#    get_default_device_driver(driver_id=Rubycdio::DRIVER_DEVICE)
#	->[device, driver]
#
#    Return a string containing the default CD device if none is
#    specified.  if driver_id is DRIVER_UNKNOWN or DRIVER_DEVICE
#    then one set the default device for that.
#    
#    nil is returned as the device if we couldn't get a default
#    device.
def get_default_device_driver(driver_id=Rubycdio::DRIVER_DEVICE)
  return Rubycdio::get_default_device_driver(driver_id)
end

#    get_devices(driver_id)->[device1, device2, ...]
#
#    Get an list of device names.
def get_devices(driver_id=nil)
    return Rubycdio::get_devices(driver_id)
end

#    get_devices_ret(driver_id)->[device1, device2, ... driver_id]
#
#    Like get_devices, but return the p_driver_id which may be different
#    from the passed-in driver_id if it was Rubycdio::DRIVER_DEVICE or
#    Rubycdio::DRIVER_UNKNOWN. The return driver_id may be useful because
#    often one wants to get a drive name and then *open* it
#    afterwards. Giving the driver back facilitates this, and speeds things
#    up for libcdio as well.
def get_devices_ret(driver_id=nil)
  devices = Rubycdio::get_devices_ret(driver_id)
end

#    get_devices_with_cap(capabilities, any=false)->[device1, device2...]
#    Get an array of device names in search_devices that have at least
#    the capabilities listed by the capabities parameter.  
#        
#    If any is false then every capability listed in the
#    extended portion of capabilities (i.e. not the basic filesystem)
#    must be satisified. If any is true, then if any of the
#    capabilities matches, we call that a success.
#
#    To find a CD-drive of any type, use the mask Rubycdio::CDIO_FS_MATCH_ALL.
#
#    The array of device names is returned or NULL if we couldn't get a
#    default device.  It is also possible to return a non NULL but after
#    dereferencing the the value is NULL. This also means nothing was
#    found.
def get_devices_with_cap(capabilities, any=false)
  # FIXME: SWIG code is not removing a parameter properly, hence the [1:]
  # at the end
  return Rubycdio::get_devices_with_cap(capabilities, any)
end

#    get_devices_with_cap(capabilities, any=false)
#      [device1, device2..., driver_id]
#
#    Like cdio_get_devices_with_cap but we return the driver we found
#    as well. This is because often one wants to search for kind of drive
#    and then *open* it afterwards. Giving the driver back facilitates this,
#      and speeds things up for libcdio as well.
def get_devices_with_cap_ret(capabilities, any=false)
  # FIXME: SWIG code is not removing a parameter properly, hence the [1:]
  # at the end
  return Rubycdio::get_devices_with_cap_ret(capabilities, any)
end

#    have_driver(driver_id) -> bool
#    
#    Return true if we have driver driver_id.
def have_driver(driver_id)
  if driver_id.class == Fixnum
    return Rubycdio::have_driver(driver_id)
  elsif driver_id.class == String and drivers.member?(driver_id)
    ret = Rubycdio::have_driver(drivers[driver_id])
    if ret == 0 then return false end
    if ret == 1 then return true end
    raise ValueError('internal error: driver id came back %d' % ret)
  else
    raise ValueError('need either a number or string driver id')
  end
end

#    binfile?(binfile_name)->cue_name
#    
#    Determine if binfile_name is the BIN file part of a CDRWIN CD
#    disk image.
#    
#    Return the corresponding CUE file if bin_name is a BIN file or
#    nil if not a BIN file.
def binfile?(binfile_name)
  return Rubycdio::is_binfile(binfile_name)
end

#    cuefile?(cuefile_name)->bin_name
#    
#    Determine if cuefile_name is the CUE file part of a CDRWIN CD
#    disk image.
#    
#    Return the corresponding BIN file if bin_name is a CUE file or
#    nil if not a CUE file.
def cuefile?(cuefile_name)
  return Rubycdio::is_cuefile(cuefile_name)
end

#    is_device(source, driver_id=Rubycdio::DRIVER_UNKNOWN)->bool
#    Return true if source refers to a real hardware CD-ROM.
def device?(source, driver_id=Rubycdio::DRIVER_UNKNOWN)
  if not driver_id then driver_id=Rubycdio::DRIVER_UNKNOWN end
  return Rubycdio::is_device(source, driver_id)
end

#    nrg?(nrgfile_name)->bool
#    
#    Determine if nrgfile_name is a Nero CD disc image
def nrg?(nrgfile_name)
  return Rubycdio::is_nrg(nrgfile_name)
end

#    tocfile?(tocfile_name)->bool
#    
#    Determine if tocfile_name is a cdrdao CD disc image
def tocfile?(tocfile_name)
  return Rubycdio::is_tocfile(tocfile_name)
end

#    Convert bit mask for miscellaneous drive properties
#    into a dictionary of drive capabilities
def convert_drive_cap_misc(bitmask)
  result={}
  if bitmask & Rubycdio::DRIVE_CAP_ERROR
    result[:DRIVE_CAP_ERROR] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_UNKNOWN
    result[:DRIVE_CAP_UNKNOWN] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_CLOSE_TRAY
    result[:DRIVE_CAP_MISC_CLOSE_TRAY] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_EJECT
    result[:DRIVE_CAP_MISC_EJECT] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_LOCK
    result['DRIVE_CAP_MISC_LOCK'] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_SELECT_SPEED
    result[:DRIVE_CAP_MISC_SELECT_SPEED] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_SELECT_DISC
    result[:DRIVE_CAP_MISC_SELECT_DISC] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_MULTI_SESSION
    result[:DRIVE_CAP_MISC_MULTI_SESSION] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_MEDIA_CHANGED
    result[:DRIVE_CAP_MISC_MEDIA_CHANGED] = true
  end
    if bitmask & Rubycdio::DRIVE_CAP_MISC_RESET
      result[:DRIVE_CAP_MISC_RESET] = true
    end
  if bitmask & Rubycdio::DRIVE_CAP_MISC_FILE
    result[:DRIVE_CAP_MISC_FILE] = true
  end
  return result
end

#    Convert bit mask for drive read properties
#    into a dictionary of drive capabilities
def convert_drive_cap_read(bitmask)
  result={}
  if bitmask & Rubycdio::DRIVE_CAP_READ_AUDIO
    result[:DRIVE_CAP_READ_AUDIO] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_CD_DA
    result[:DRIVE_CAP_READ_CD_DA] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_CD_G
    result[:DRIVE_CAP_READ_CD_G] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_CD_R
    result[:DRIVE_CAP_READ_CD_R] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_CD_RW
    result[:DRIVE_CAP_READ_CD_RW] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_DVD_R
    result[:DRIVE_CAP_READ_DVD_R] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_DVD_PR
    result[:DRIVE_CAP_READ_DVD_PR] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_DVD_RAM
    result[:DRIVE_CAP_READ_DVD_RAM] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_DVD_ROM
    result[:DRIVE_CAP_READ_DVD_ROM] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_DVD_RW
    result[:DRIVE_CAP_READ_DVD_RW] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_DVD_RPW
        result[:DRIVE_CAP_READ_DVD_RPW] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_C2_ERRS
    result[:DRIVE_CAP_READ_C2_ERRS] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_MODE2_FORM1
    result[:DRIVE_CAP_READ_MODE2_FORM1] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_MODE2_FORM2
    result[:DRIVE_CAP_READ_MODE2_FORM2] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_MCN
    result[:DRIVE_CAP_READ_MCN] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_READ_ISRC
    result[:DRIVE_CAP_READ_ISRC] = true
  end
  return result
end

#    Convert bit mask for drive write properties
#    into a dictionary of drive capabilities
def convert_drive_cap_write(bitmask)
  result={}
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_CD_R
    result[:DRIVE_CAP_WRITE_CD_R] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_CD_RW
    result[:DRIVE_CAP_WRITE_CD_RW] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_DVD_R
    result[:DRIVE_CAP_WRITE_DVD_R] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_DVD_PR
    result[:DRIVE_CAP_WRITE_DVD_PR] = true
    end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_DVD_RAM
    result[:DRIVE_CAP_WRITE_DVD_RAM] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_DVD_RW
    result[:DRIVE_CAP_WRITE_DVD_RW] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_DVD_RPW
    result[:DRIVE_CAP_WRITE_DVD_RPW] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_MT_RAINIER
    result[:DRIVE_CAP_WRITE_MT_RAINIER] = true
  end
  if bitmask & Rubycdio::DRIVE_CAP_WRITE_BURN_PROOF
    result[:DRIVE_CAP_WRITE_BURN_PROOF] = true
  end
  return result
end

#  CD Input and control class for discs/devices
class Device
  
  def initialize(source=nil, driver_id=nil,
               access_mode=nil)
    @cd = nil
    if source or driver_id
      open(source, driver_id, access_mode)
    end
  end
  
  # audio_pause(cdio)->status
  # Pause playing CD through analog output.
  # A DeviceError exception may be raised.
  def audio_pause()
    drc=Rubycdio::audio_pause(@cd)
    __possibly_raise_exception__(drc)
  end
  
  # auto_play_lsn(cdio, start_lsn, end_lsn)->status
  # 
  # Playing CD through analog output at the given lsn to the ending lsn
  # A DeviceError exception may be raised.
  def audio_play_lsn(start_lsn, end_lsn)
    drc=Rubycdio::audio_play_lsn(@cd, start_lsn, end_lsn)
    __possibly_raise_exception__(drc)
  end
  
  # audio_resume(cdio)->status
  # Resume playing an audio CD through the analog interface.
  # A DeviceError exception may be raised.
  def audio_resume()
    drc=Rubycdio::audio_resume(@cd)
    __possibly_raise_exception__(drc)
  end
  
  # audio_stop(cdio)->status
  # Stop playing an audio CD through the analog interface.
  # A DeviceError exception may be raised.
  def audio_stop()
    drc=Rubycdio::audio_stop(@cd)
    __possibly_raise_exception__(drc)
  end
  
  # close()
  # Free resources associated with p_cdio.  Call this when done using
  # using CD reading/control operations for the current device.
  def close()
    if @cd 
      Rubycdio::close(@cd)
    else
      puts "***No object to close"
    end
    @cd=nil
  end
  
  # eject_media()
  # Eject media in CD drive if there is a routine to do so.
  # A DeviceError exception may be raised.
  def eject_media()
    drc=Rubycdio::eject_media(@cd)
    @cd = nil
    __possibly_raise_exception__(drc)
  end
  
  # eject_media_drive(drive=nil)
  # Eject media in CD drive if there is a routine to do so.
  # An exception is thrown on error.
  def eject_media_drive(drive=nil)
    ### FIXME: combine into above by testing if drive is the string
    ### nil versus drive = Rubycdio::DRIVER_UNKNOWN
    Rubycdio::eject_media_drive(drive)
  end
  
  # get_arg(key)->string
  # Get the value associatied with key.
  def get_arg(key)
    return Rubycdio::get_arg(@cd, key)
  end
  
  # get_device()->str
  # Get the default CD device.
  # If we haven't initialized a specific device driver), 
  # then find a suitable one and return the default device for that.
  # In some situations of drivers or OS's we can't find a CD device if
  # there is no media in it and it is possible for this routine to return
  # nil even though there may be a hardware CD-ROM.
  def get_device()
    if @cd 
      return Rubycdio::get_arg(@cd, "source")
    end
    return Rubycdio::get_device(@cd)
  end
  
  # get_disc_last_lsn()->int
  # Get the LSN of the end of the CD
  # 
  # DriverError and IOError may raised on error.
  def get_disc_last_lsn()
    lsn = Rubycdio::get_disc_last_lsn(@cd)
    if lsn == Rubycdio::INVALID_LSN:
        raise DriverError('Invalid LSN returned')
    end
    return lsn
  end
  
  # get_disc_mode(p_cdio) -> str
  #   
  # Get disc mode - the kind of CD (CD-DA, CD-ROM mode 1, CD-MIXED, etc.
  # that we've got. The notion of 'CD' is extended a little to include
  # DVD's.
  def get_disc_mode()
    return Rubycdio::get_disc_mode(@cd)
  end
  
  # get_drive_cap()->(read_cap, write_cap, misc_cap)
  #
  # Get drive capabilities of device.
  #
  # In some situations of drivers or OS's we can't find a CD
  # device if there is no media in it. In this situation
  # capabilities will show up as empty even though there is a
  # hardware CD-ROM.  get_drive_cap_dev()->(read_cap, write_cap,
  # misc_cap)
  #
  # Get drive capabilities of device.
  # 
  # In some situations of drivers or OS's we can't find a CD
  # device if there is no media in it. In this situation
  # capabilities will show up as empty even though there is a
  # hardware CD-ROM.
  def get_drive_cap()
    b_read_cap, b_write_cap, b_misc_cap = Rubycdio::get_drive_cap(@cd)
    return [convert_drive_cap_read(b_read_cap),
            convert_drive_cap_write(b_write_cap),
            convert_drive_cap_misc(b_misc_cap)]
  end
  
  def get_drive_cap_dev(device=nil)
    ### FIXME: combine into above by testing on the type of device.
    b_read_cap, b_write_cap, b_misc_cap = 
      Rubycdio::get_drive_cap_dev(device);
    return [convert_drive_cap_read(b_read_cap), 
            convert_drive_cap_write(b_write_cap), 
            convert_drive_cap_misc(b_misc_cap)]
  end
  
  # get_driver_name()-> string
  #        
  # return a string containing the name of the driver in use.
  #        
  # An IOError exception is raised on error.
  def get_driver_name()
    return Rubycdio::get_driver_name(@cd)
  end
  
  # get_driver_id()-> int
  #        
  # Return the driver id of the driver in use.
  # if object has not been initialized or is nil,
  # return Rubycdio::DRIVER_UNKNOWN.
  def get_driver_id()
    return Rubycdio::get_driver_id(@cd)
  end
  
  # get_first_track()->Track
  #
  # return a Track object of the first track. nil is returned 
  # if there was a problem.
  def get_first_track()
    track = Rubycdio::get_first_track_num(@cd)
    if track == Rubycdio::INVALID_TRACK
      return nil
    end
    return Track.new(@cd, track)
  end
  
  # get_hwinfo()->[vendor, model, release]
  # Get the CD-ROM hardware info via a SCSI MMC INQUIRY command.
  def get_hwinfo()
    return Rubycdio::get_hwinfo(@cd)
  end
  
  # get_joliet_level()->int
  #
  #   Return the Joliet level recognized for cdio.
  #    This only makes sense for something that has an ISO-9660
  #    filesystem.
  def get_joliet_level()
    return Rubycdio::get_joliet_level(@cd)
  end
  
  # get_last_session() -> int
  #   Get the LSN of the first track of the last session of on the CD.
  #    An exception is thrown on error.
  def get_last_session()
    drc, session = Rubycdio::get_last_session(@cd)
    __possibly_raise_exception__(drc)
    return session
  end
  
  # get_last_track()->Track
  #
  # return a Track object of the first track. nil is returned 
  # if there was a problem.
  def get_last_track()
    track = Rubycdio::get_last_track_num(@cd)
    if track == Rubycdio::INVALID_TRACK
      return nil
    end
    return Track.new(@cd, track)
  end
  
  # get_mcn() -> str
  # 
  # Get the media catalog number (MCN) from the CD.
  def get_mcn()
    return Rubycdio::get_mcn(@cd)
  end

  # get_media_changed() -> bool
  #
  # Find out if media has changed since the last call.
  # Return true if media has changed since last call. An exception
  # Error is given on error.
  def get_media_changed()
    drc = Rubycdio::get_media_changed(@cd)
    if drc == 0 then return false end
    if drc == 1 then return true end
    __possibly_raise_exception__(drc)
    raise DeviceException('Unknown return value %d' % drc)
  end
  
  # get_num_tracks()->int
  #    
  # Return the number of tracks on the CD. 
  # A TrackError or IOError exception may be raised on error.
  def get_num_tracks()
    track = Rubycdio::get_num_tracks(@cd)
    if track == Rubycdio::INVALID_TRACK:
        raise TrackError('Invalid track returned')
    end
    return track
  end
  
  # get_track(track_num)->track
  #    
  #    Return a track object for the given track number.
  def get_track(track_num)
    return Track.new(@cd, track_num)
  end
  
  # get_track_for_lsn(lsn)->Track
  #    
  #    Find the track which contains lsn.
  #    nil is returned if the lsn outside of the CD or
  #    if there was some error. 
  #    
  #    If the lsn is before the pregap of the first track,
  #    A track object with a 0 track is returned.
  #    Otherwise we return the track that spans the lsn.
  def get_track_for_lsn(lsn)
    track = Rubycdio::get_last_track_num(@cd)
    if track == Rubycdio::INVALID_TRACK:
        return nil
    end
    return Track.new(@cd, track)
  end
  
  # have_ATAPI()->bool
  # return true if CD-ROM understand ATAPI commands.
  def have_ATAPI()
    return Rubycdio::have_ATAPI(@cd)
  end
  
  # lseek(offset, whence)->int
  #     Reposition read offset
  #    Similar to (if not the same as) libc's fseek()
  #    
  #    cdio is object to get adjested, offset is amount to seek and 
  #    whence is like corresponding parameter in libc's lseek, e.g. 
  #    it should be SEEK_SET or SEEK_END.
  #    
  #    the offset is returned or -1 on error.
  def lseek(offset, whence)
    return Rubycdio::lseek(@cd, offset, whence)
  end
  
  # open(source=nil, driver_id=Rubycdio::DRIVER_UNKNOWN,
  #      access_mode=nil)
  #        
  # Sets up to read from place specified by source, driver_id and
  # access mode. This should be called before using any other routine
  # except those that act on a CD-ROM drive by name. 
  #        
  # If nil is given as the source, we'll use the default driver device.
  # If nil is given as the driver_id, we'll find a suitable device driver.
  #
  # If device object was, previously opened it is closed first.
  #        
  # Device is opened so that subsequent operations can be performed.
  def open(source=nil, driver_id=Rubycdio::DRIVER_UNKNOWN,
           access_mode=nil)
    if not driver_id 
      driver_id=Rubycdio::DRIVER_UNKNOWN 
    end
    if not source
      source = '' 
    end
    if not access_mode 
      access_mode = '' 
    end
    if @cd
      close()
    end
    @cd = Rubycdio::open_cd(source, driver_id, access_mode)
  end
  
  # read(size)->[size, data]
  #        
  # Reads the next size bytes.
  # Similar to (if not the same as) libc's read()
  #        
  # The number of bytes read and the data is returned. 
  # A DeviceError exception may be raised.
  def read(size)
    size, data = Rubycdio::read_cd(@cd, size)
    __possibly_raise_exception__(size)
    return [size, data]
  end
  
  # read_data_blocks(blocks, lsn, blocks=1)->[size, data]
  #        
  # Reads a number of data sectors (AKA blocks).
  #      
  # lsn is sector to read, bytes is the number of bytes.
  # A DeviceError exception may be raised.
  def read_data_blocks(lsn, blocks=1)
    size = Rubycdio::ISO_BLOCKSIZE*blocks
    size, data = Rubycdio::read_data_bytes(@cd, size, lsn,
                                           Rubycdio::ISO_BLOCKSIZE)
    if size < 0
      __possibly_raise_exception__(size)
    end
    return [size, data]
  end
  
  # read_sectors(lsn, read_mode, blocks=1)->[blocks, data]
  # Reads a number of sectors (AKA blocks).
  # 
  # lsn is sector to read, bytes is the number of bytes.
  #        
  # If read_mode is Rubycdio::MODE_AUDIO, the return buffer size will be
  # truncated to multiple of Rubycdio::CDIO_FRAMESIZE_RAW i_blocks bytes.
  #        
  # If read_mode is Rubycdio::MODE_DATA, buffer will be truncated to a
  # multiple of Rubycdio::ISO_BLOCKSIZE, Rubycdio::M1RAW_SECTOR_SIZE or
  # Rubycdio::M2F2_SECTOR_SIZE bytes depending on what mode the data is in.
  #
  # If read_mode is Rubycdio::MODE_M2F1, buffer will be truncated to a 
  # multiple of Rubycdio::M2RAW_SECTOR_SIZE bytes.
  #        
  # If read_mode is Rubycdio::MODE_M2F2, the return buffer size will be
  # truncated to a multiple of Rubycdio::CD_FRAMESIZE bytes.
  #        
  # The number of bytes read and the data is returned. 
  # A DeviceError exception may be raised.
  def read_sectors(lsn, read_mode, blocks=1)
    begin
      blocksize = read_mode2blocksize[read_mode]
      size = blocks * blocksize
    rescue KeyError
      raise DriverBadParameterError('Bad read mode %d' % read_mode)
    end
    size, data = Rubycdio::read_sectors(@cd, size, lsn, read_mode)
    if size < 0
      __possibly_raise_exception__(size)
    end
    blocks = size / blocksize
    return [blocks, data]
  end
  
  # set_blocksize(blocksize)
  # Set the blocksize for subsequent reads.
  #  An exception is thrown on error.
  def set_blocksize(blocksize)
    drc = Rubycdio::set_blocksize(@cd, blocksize)
    __possibly_raise_exception__(drc)
  end
  
  # set_speed(speed)
  # Set the drive speed. An exception is thrown on error.
  def set_speed(speed)
    drc = Rubycdio::set_speed(@cd, speed)
    __possibly_raise_exception__(drc)
  end
end # Device

# CD Input and control track class
class Track
  
  def initialize(device, track_num)
    
    if track_num.class != Fixnum
      raise TrackError('track number parameter is not an integer')
    end
    @track = track_num
    
    # See if the device parameter is a string or
    # a device object.
    if device.class == String
      @device = Device.new(device)
    else
      @device = device
    end
  end
  
  # get_audio_channels(track)->int
  #     
  # Return number of channels in track: 2 or 4
  # Not meaningful if track is not an audio track.
  # An exception can be raised on error.
  def get_audio_channels()
    channels = Rubycdio::get_track_channels(@device, @track)
    if -2 == channels
      raise DriverUnsupportedError
    elsif -1 == channels
      raise TrackError
    else
      return channels
    end
  end
  
  # get_copy_permit(track)->int
  #
  # Return copy protection status on a track. Is this meaningful 
  # not an audio track?
  def get_copy_permit()
    if Rubycdio::get_track_copy_permit(@device, @track):
        return "OK"
    else
      return "no"
    end
  end
  
  # get_format()->format
  # Get the format (e.g. 'audio', 'mode2', 'mode1') of track. 
  def get_format()
    return Rubycdio::get_track_format(@device, @track)
  end
  
  # get_last_lsn()->lsn
  #      
  # Return the ending LSN for a track 
  # A TrackError or IOError exception may be raised on error.
  def get_last_lsn()
    lsn = Rubycdio::get_track_last_lsn(@device, @track)
    if lsn == Rubycdio::INVALID_LSN
      raise TrackError('Invalid LSN returned')
    end
    return lsn
  end
  
  # get_lsn()->lba
  #
  # Return the starting LBA for a track
  # A TrackError exception is raised on error.
  def get_lba()
    lba = Rubycdio::get_track_lba(@device, @track)
    if lba == Rubycdio::INVALID_LBA
      raise TrackError('Invalid LBA returned')
    end
    return lba
  end
  
  # get_lsn()->lsn
  #
  # Return the starting LSN for a track
  # A TrackError exception is raised on error.
  def get_lsn()
    lsn = Rubycdio::get_track_lsn(@device, @track)
    if lsn == Rubycdio::INVALID_LSN:
        raise TrackError('Invalid LSN returned')
    end
    return lsn
  end
  
  # get_msf()->str
  # 
  # Return the starting MSF (minutes/secs/frames) for track number track.
  # Track numbers usually start at something greater than 0, usually 1.
  # 
  # Returns string of the form mm:ss:ff if all good, or string nil on
  # error.
  def get_msf()
    return Rubycdio::get_track_msf(@device, @track)
  end
  
  # get_preemphaisis()->result
  # 
  # Get linear preemphasis status on an audio track.
  # This is not meaningful if not an audio track?
  # A TrackError exception is raised on error.
  def get_preemphasis()
        rc = Rubycdio::get_track_preemphasis(@device, @track)
        if rc == Rubycdio::TRACK_FLAG_FALSE:
            return 'none'
        elsif rc == Rubycdio::TRACK_FLAG_TRUE:
            return 'preemphasis'
        elsif rc == Rubycdio::TRACK_FLAG_UNKNOWN:
            return 'unknown'
        else
            raise TrackError('Invalid return value %d' % d)
        end
    end

  # get_track_sec_count()->int
  # 
  # Get the number of sectors between this track an the next.  This
  # includes any pregap sectors before the start of the next track.
  # Track numbers usually start at something 
  # greater than 0, usually 1.
  # 
  # A TrackError exception is raised on error.
    def get_track_sec_count()
        sec_count = Rubycdio::get_track_sec_count(@device, @track)
        if sec_count == 0
            raise TrackError
        end
        return sec_count
    end

    # green?(track) -> bool
    #
    # Return true if we have XA data (green, mode2 form1) or
    # XA data (green, mode2 form2). That is track begins:
    # sync - header - subheader
    # 12     4      -  8
    def green?()
      return Rubycdio::is_track_green(@device, @track)
    end
    
    # set_track(track_num)
    # 
    # Set a new track number.
    def set_track(track_num)
      @track = track_num
    end

    # Return the track track number.
    def track()
      @track
    end
  end # Track

#
# Local variables:
#  mode: Ruby
# End:
