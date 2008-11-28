#!/usr/bin/python
#  $Id: drives.py,v 1.2 2008/11/24 00:53:59 rocky Exp $
"""Program to read CD blocks. See read-cd from the libcdio distribution
for a more complete program."""

#
#    Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>
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

import os, sys
import pycdio
import cdio

def print_drive_class(msg, bitmask, any):
    cd_drives = cdio.get_devices_with_cap(bitmask, any)

    print "%s..." % msg
    for drive in cd_drives:
	print "Drive %s" % drive
    print "-----"

cd_drives = cdio.get_devices(pycdio.DRIVER_DEVICE)
for drive in cd_drives: 
    print "Drive %s" % drive

print "-----"

sys.exit(0)
# FIXME: there's a bug in get_drive_with_cap that corrupts memory.
print_drive_class("All CD-ROM drives (again)", pycdio.FS_MATCH_ALL, False);
print_drive_class("All CD-DA drives...", pycdio.FS_AUDIO, False);
print_drive_class("All drives with ISO 9660...", pycdio.FS_ISO_9660, False);
print_drive_class("VCD drives...", 
                  (pycdio.FS_ANAL_SVCD|pycdio.FS_ANAL_CVD|
                   pycdio.FS_ANAL_VIDEOCD|pycdio.FS_UNKNOWN), True);



