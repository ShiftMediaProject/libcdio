#!/usr/bin/python
#
#  $Id: iso9660.py,v 1.12 2008/05/01 16:55:03 karl Exp $
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

import pyiso9660
import cdio
import types

check_types = {
    'nocheck'   : pyiso9660.NOCHECK,
    '7bit'      : pyiso9660.SEVEN_BIT,
    'achars'    : pyiso9660.ACHARS,
    'dchars'    : pyiso9660.DCHARS
    }

def dirname_valid_p(path):
    """dirname_valid_p(path)->bool

Check that path is a valid ISO-9660 directory name.

A valid directory name should not start out with a slash (/), 
dot (.) or null byte, should be less than 37 characters long, 
have no more than 8 characters in a directory component 
which is separated by a /, and consist of only DCHARs. 

True is returned if path is valid."""
    return pyiso9660.dirname_valid(path)


def is_achar(achar):
    """is_achar(achar)->bool

Return 1 if achar is an ACHAR. achar should either be a string of 
length one or the ord() of a string of length 1.

These are the DCHAR's plus some ASCII symbols including the space 
symbol."""   
    if type(achar) != types.IntType:
	# Not integer. Should be a string of length one then.
        # We'll turn it into an integer.
        try:
            achar = ord(achar)
        except:
            return 0 
    else:
	# Is an integer. Is it too large?
        if achar > 255: return 0 
    return pyiso9660.is_achar(achar)


def is_dchar(dchar):
    """is_dchar(dchar)->bool

Return 1 if dchar is a DCHAR - a character that can appear in an an
ISO-9600 level 1 directory name. These are the ASCII capital
letters A-Z, the digits 0-9 and an underscore.

dchar should either be a string of length one or the ord() of a string
of length 1."""

    if type(dchar) != types.IntType:
	# Not integer. Should be a string of length one then.
        # We'll turn it into an integer.
        try:
            dchar = ord(dchar)
        except:
            return 0 
    else:
	# Is an integer. Is it too large?
        if dchar > 255: return 0 
    return pyiso9660.is_dchar(dchar)


def pathname_valid_p(path):
    """pathname_valid_p(path)->bool

Check that path is a valid ISO-9660 pathname.  

A valid pathname contains a valid directory name, if one appears and
the filename portion should be no more than 8 characters for the
file prefix and 3 characters in the extension (or portion after a
dot). There should be exactly one dot somewhere in the filename
portion and the filename should be composed of only DCHARs.
  
True is returned if path is valid."""
    return pyiso9660.pathame_valid(path)


def pathname_isofy(path, version=1):
    """pathname_valid_p(path, version=1)->string

Take path and a version number and turn that into a ISO-9660 pathname.
(That's just the pathname followed by ';' and the version number. For
example, mydir/file.ext -> MYDIR/FILE.EXT;1 for version 1. The
resulting ISO-9660 pathname is returned."""
    return pyiso9660.pathname_isofy(path, version)

def name_translate(filename, joliet_level=0):
    """name_translate(name, joliet_level=0)->str

Convert an ISO-9660 file name of the kind that is that stored in a ISO
9660 directory entry into what's usually listed as the file name in a
listing.  Lowercase name if no Joliet Extension interpretation. Remove
trailing ;1's or .;1's and turn the other ;'s into version numbers.

If joliet_level is not given it is 0 which means use no Joliet
Extensions. Otherwise use the specified the Joliet level. 

The translated string is returned and it will be larger than the input
filename."""
    return pyiso9660.name_translate_ext(filename, joliet_level)


# FIXME: should be
# def stat_array_to_dict(*args):
# Probably have a SWIG error.
def stat_array_to_dict(filename, LSN, size, sec_size, is_dir):
    """stat_array_to_href(filename, LSN, size, sec_size, is_dir)->stat

Convert a ISO 9660 array to an hash reference of the values.

Used internally in convert from C code."""

    stat = {}
    stat['filename'] = filename
    stat['LSN']      = LSN
    stat['size']     = size
    stat['sec_size'] = sec_size
    stat['is_dir']   = is_dir == 2
    return stat
 
def strncpy_pad(name, len, check):
    """strncpy_pad(src, len, check='nocheck')->str

Pad string 'name' with spaces to size len and return this. If 'len' is
less than the length of 'src', the return value will be truncated to
the first len characters of 'name'.

'name' can also be scanned to see if it contains only ACHARs, DCHARs,
or 7-bit ASCII chars, and this is specified via the 'check' parameter. 
If the I<check> parameter is given it must be one of the 'nocheck',
'7bit', 'achars' or 'dchars'. Case is not significant."""
    if check not in check_types:
        print "*** A CHECK parameter must be one of %s\n" % \
              ', '.join(check_types.keys())
        return None
    return pyiso9660.strncpy_pad(name, len, check_types[check])

class ISO9660:
    """ """
    class IFS:
        """ISO 9660 Filesystem image reading"""

        def __init__(self, source=None, iso_mask=pyiso9660.EXTENSION_NONE):

            """Create a new ISO 9660 object.  If source is given, open()
            is called using that and the optional iso_mask parameter;
            iso_mask is used only if source is specified.  If source is
            given but opening fails, undef is returned.  If source is not
            given, an object is always returned."""
            
            self.iso9660 = None
            if source is not None:
                self.open(source, iso_mask)

        def close(self):
            """close(self)->bool
                    
            Close previously opened ISO 9660 image and free resources
            associated with ISO9660.  Call this when done using using
            an ISO 9660 image."""
            
            if self.iso9660 is not None:
                pyiso9660.close(self.iso9660)
            else:
                print "***No object to close"
            self.iso9660 = None

        def find_lsn(self, lsn):
            """find_lsn(self, lsn)->[stat_href]

            Find the filesystem entry that contains LSN and return
            file stat information about it. None is returned on
            error."""

            if pycdio.VERSION_NUM <= 76:
                print "*** Routine available only in libcdio versions >= 0.76"
                return None
            
            filename, LSN, size, sec_size, is_dir = \
                      pyiso9660.ifs_find_lsn(self.iso9660, lsn)
            return stat_array_to_href(filename, LSN, size, sec_size, is_dir)
    

        def get_application_id(self):
            """get_application_id(self)->id
            
            Get the application ID stored in the Primary Volume Descriptor.
            None is returned if there is some problem."""
            
            return pyiso9660.ifs_get_application_id(self.iso9660)

        def get_preparer_id(self):
            """get_preparer_id(self)->id
            
            Get the preparer ID stored in the Primary Volume Descriptor.
            None is returned if there is some problem."""
            
            return pyiso9660.ifs_get_preparer_id(self.iso9660)


        def get_publisher_id(self):
            """get_publisher_id()->id
            
            Get the publisher ID stored in the Primary Volume Descriptor.
            undef is returned if there is some problem."""
            return pyiso9660.ifs_get_publisher_id(self.iso9660)
    
        def get_root_lsn(self):
            """get_root_lsn(self)->lsn
            
            Get the Root LSN stored in the Primary Volume Descriptor.
            undef is returned if there is some problem."""
            
            return pyiso9660.ifs_get_root_lsn(self.iso9660)
        
        def get_system_id(self):
            """get_system_id(self)->id
            
            Get the Volume ID stored in the Primary Volume Descriptor.
            undef is returned if there is some problem."""
            
            return pyiso9660.ifs_get_system_id(self.iso9660)

        def get_volume_id(self):
            """get_volume_id()->id
            
            Get the Volume ID stored in the Primary Volume Descriptor.
            undef is returned if there is some problem."""
            
            return pyiso9660.ifs_get_volume_id(self.iso9660)

        def get_volumeset_id(self):
            """get_volume_id(self)->id
            
            Get the Volume ID stored in the Primary Volume Descriptor.
            undef is returned if there is some problem."""
            
            return pyiso9660.ifs_get_volumeset_id(self.iso9660)
        
        def is_open(self):
            """is_open(self)->bool
            Return true if we have an ISO9660 image open.
            """
            return self.iso9660 is not None

        def open(self, source, iso_mask=pyiso9660.EXTENSION_NONE):
            """open(source, iso_mask=pyiso9660.EXTENSION_NONE)->bool
            
            Open an ISO 9660 image for reading. Subsequent operations
            will read from this ISO 9660 image.
            
            This should be called before using any other routine
            except possibly new. It is implicitly called when a new is
            done specifying a source.
            
            If device object was previously opened it is closed first.
            
            See also open_fuzzy."""
            
            if self.iso9660 is not None: self.close() 

            self.iso9660 = pyiso9660.open_ext(source, iso_mask)
            return self.iso9660 is not None


        def open_fuzzy(self, source, iso_mask=pyiso9660.EXTENSION_NONE,
                       fuzz=20):
            """open_fuzzy(source, iso_mask=pyiso9660.EXTENSION_NONE,
                          fuzz=20)->bool
            
            Open an ISO 9660 image for reading. Subsequent operations
            will read from this ISO 9660 image. Some tolerence allowed
            for positioning the ISO9660 image. We scan for
            pyiso9660.STANDARD_ID and use that to set the eventual
            offset to adjust by (as long as that is <= fuzz).
            
            This should be called before using any other routine
            except possibly new (which must be called first. It is
            implicitly called when a new is done specifying a source.
            
            See also open."""

            if self.iso9660 is not None: self.close() 
            
            if type(fuzz) != types.IntType:
                print "*** Expecting fuzz to be an integer; got 'fuzz'"
                return False
            
            self.iso9660 = pyiso9660.open_fuzzy_ext(source, iso_mask, fuzz)
            return self.iso9660 is not None
        
        def read_fuzzy_superblock(self, iso_mask=pyiso9660.EXTENSION_NONE,
                                  fuzz=20):
            """read_fuzzy_superblock(iso_mask=pyiso9660.EXTENSION_NONE,
                                     fuzz=20)->bool
            
            Read the Super block of an ISO 9660 image but determine
            framesize and datastart and a possible additional
            offset. Generally here we are not reading an ISO 9660 image
            but a CD-Image which contains an ISO 9660 filesystem."""
            
            if type(fuzz) != types.IntType:
                print "*** Expecting fuzz to be an integer; got 'fuzz'"
                return False
            
            return pyiso9660.ifs_fuzzy_read_superblock(self.iso9660, iso_mask,
                                                       fuzz)
        
        def readdir(self, dirname): 
            """readdir(dirname)->[LSN, size, sec_size, filename, XA, is_dir]
            
            Read path (a directory) and return a list of iso9660 stat
            references
            
            Each item of @iso_stat is a hash reference which contains

            * LSN       - the Logical sector number (an integer)
            * size      - the total size of the file in bytes
            * sec_size  - the number of sectors allocated
            * filename  - the file name of the statbuf entry
            * XA        - if the file has XA attributes; 0 if not
            * is_dir    - 1 if a directory; 0 if a not;
            
            FIXME: If you look at iso9660.h you'll see more fields, such as for
            Rock-Ridge specific fields or XA specific fields. Eventually these
            will be added. Volunteers?"""
            
            return pyiso9660.ifs_readdir(self.iso9660, dirname)


        def read_pvd(self):
            """read_pvd()->pvd
            
            Read the Super block of an ISO 9660 image. This is the
            Primary Volume Descriptor (PVD) and perhaps a Supplemental
            Volume Descriptor if (Joliet) extensions are
            acceptable."""
            
            return pyiso9660.ifs_read_pvd(self.iso9660)

        def read_superblock(self, iso_mask=pyiso9660.EXTENSION_NONE):
            """read_superblock(iso_mask=pyiso9660.EXTENSION_NONE)->bool
            
            Read the Super block of an ISO 9660 image. This is the
            Primary Volume Descriptor (PVD) and perhaps a Supplemental
            Volume Descriptor if (Joliet) extensions are
            acceptable."""
            
            return pyiso9660.ifs_read_superblock(self.iso9660, iso_mask)

        def seek_read(self, start, size=1):
            """seek_read(self, start, size=1)
            ->(size, str)
            
            Seek to a position and then read n blocks. A block is
            pycdio.ISO_BLOCKSIZE (2048) bytes. The Size in BYTES (not blocks)
            is returned."""
            size *= pyiso9660.ISO_BLOCKSIZE
            return pyiso9660.seek_read(self.iso9660, start, size)

        def stat(self, path, translate=False):
            """stat(self, path, translate=False)->{stat}
            
            Return file status for path name path. None is returned on
            error.  If translate is True, version numbers in the ISO 9660
            name are dropped, i.e. ;1 is removed and if level 1 ISO-9660
            names are lowercased.
            
            Each item of the return is a hash reference which contains:
            
            * LSN      - the Logical sector number (an integer)
            * size     - the total size of the file in bytes
            * sec_size - the number of sectors allocated
            * filename - the file name of the statbuf entry
            * XA       - if the file has XA attributes; False if not
            * is_dir   - True if a directory; False if a not."""
            
            if translate:
                values = pyiso9660.ifs_stat_translate(self.iso9660, path)
            else:
                values = pyiso9660.ifs_stat(self.iso9660, path)
            return stat_array_to_dict(values[0], values[1], values[2],
                                      values[3], values[4])

    class FS(cdio.Device):
        """ISO 9660 CD reading"""

        def __init__(self, source=None, driver_id=None,
                     access_mode=None):

            """Create a new ISO 9660 object.  If source is given, open()
            is called using that and the optional iso_mask parameter;
            iso_mask is used only if source is specified.  If source is
            given but opening fails, undef is returned.  If source is not
            given, an object is always returned."""
            
            cdio.Device.__init__(self, source, driver_id, access_mode)


        def find_lsn(self, lsn):
            """find_lsn(self, lsn)->[stat_href]

            Find the filesystem entry that contains LSN and return
            file stat information about it. None is returned on
            error."""

            filename, LSN, size, sec_size, is_dir = \
                      pyiso9660.fs_find_lsn(self.cd, lsn)
            return stat_array_to_href(filename, LSN, size, sec_size, is_dir)
    
        
        def readdir(self, dirname): 
            """readdir(dirname)->[LSN, size, sec_size, filename, XA, is_dir]
            
            Read path (a directory) and return a list of iso9660 stat
            references
            
            Each item of @iso_stat is a hash reference which contains

            * LSN       - the Logical sector number (an integer)
            * size      - the total size of the file in bytes
            * sec_size  - the number of sectors allocated
            * filename  - the file name of the statbuf entry
            * XA        - if the file has XA attributes; 0 if not
            * is_dir    - 1 if a directory; 0 if a not;
            
            FIXME: If you look at iso9660.h you'll see more fields, such as for
            Rock-Ridge specific fields or XA specific fields. Eventually these
            will be added. Volunteers?"""
            
            return pyiso9660.fs_readdir(self.cd, dirname)


        def read_pvd(self):
            """read_pvd()->pvd
            
            Read the Super block of an ISO 9660 image. This is the
            Primary Volume Descriptor (PVD) and perhaps a Supplemental
            Volume Descriptor if (Joliet) extensions are
            acceptable."""
            
            return pyiso9660.fs_read_pvd(self.cd)

        def read_superblock(self, iso_mask=pyiso9660.EXTENSION_NONE):
            """read_superblock(iso_mask=pyiso9660.EXTENSION_NONE)->bool
            
            Read the Super block of an ISO 9660 image. This is the
            Primary Volume Descriptor (PVD) and perhaps a Supplemental
            Volume Descriptor if (Joliet) extensions are
            acceptable."""
            
            return pyiso9660.fs_read_superblock(self.cd, iso_mask)

        def stat(self, path, translate=False):
            """stat(self, path, translate=False)->{stat}
            
            Return file status for path name path. None is returned on
            error.  If translate is True, version numbers in the ISO 9660
            name are dropped, i.e. ;1 is removed and if level 1 ISO-9660
            names are lowercased.
            
            Each item of the return is a hash reference which contains:
            
            * LSN      - the Logical sector number (an integer)
            * size     - the total size of the file in bytes
            * sec_size - the number of sectors allocated
            * filename - the file name of the statbuf entry
            * XA       - if the file has XA attributes; False if not
            * is_dir   - True if a directory; False if a not."""
            
            if translate:
                values = pyiso9660.fs_stat_translate(self.cd, path)
            else:
                values = pyiso9660.fs_stat(self.cd, path)
            return stat_array_to_dict(values[0], values[1], values[2],
                                      values[3], values[4])
