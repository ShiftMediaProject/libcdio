#!/usr/bin/env python
"""Program to Eject and close CD-ROM drive"""
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

import os, sys
libdir = os.path.join(os.path.dirname(__file__), '..')
if libdir[-1] != os.path.sep:
    libdir += os.path.sep
sys.path.insert(0, libdir)
import pycdio
import cdio

if sys.argv[1:]:
    try:
        drive_name=sys.argv[1]
        d = cdio.Device(drive_name)
    except IOError:
        print "Problem opening CD-ROM: %s" % drive_name
        sys.exit(1)
else:
    try:
        d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
        drive_name = d.get_device()
    except IOError:
        print "Problem finding a CD-ROM"
        sys.exit(1)

try:
    
    print "Ejecting CD in drive %s" % drive_name
    d.eject_media()
    try:
        cdio.close_tray(drive_name)
        print "Closed tray of CD-ROM drive %s" % drive_name
    except cdio.DeviceException:
        print "Closing tray of CD-ROM drive %s failed" % drive_name

except cdio.DriverUnsupportedError:
    print "Eject not supported for %s" % drive_name
except cdio.DeviceException:
    print "Eject of CD-ROM drive %s failed" % drive_name

    

  
