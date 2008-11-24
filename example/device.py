#!/usr/bin/env python
"""Program to show CD-ROM device information"""
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

def sort_dict_keys(dict):
    """Return sorted keys of a dictionary.
    There's probably an easier way to do this that I'm not aware of."""
    keys=dict.keys()
    keys.sort()
    return keys

if sys.argv[1:]:
    try:
        drive_name = sys.argv[1]
        d = cdio.Device(sys.argv[1])
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

# Should there should be no "ok"?
ok, vendor, model, release = d.get_hwinfo()

print "drive: %s, vendor: %s, model: %s, release: %s" \
      % (drive_name, vendor, model, release)

read_cap, write_cap, misc_cap = d.get_drive_cap()
print "Drive Capabilities for %s..." % drive_name

print "\n".join(cap for cap in sort_dict_keys(read_cap) +
                sort_dict_keys(write_cap) + sort_dict_keys(misc_cap))

print "\nDriver Availabiliity..."
for driver_name in sort_dict_keys(cdio.drivers):
    try: 
        if cdio.have_driver(driver_name):
            print "Driver %s is installed." % driver_name
    except ValueError:
        pass
d.close()
