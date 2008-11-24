#!/usr/bin/env python
"""Program to show CD media changing"""
#
#  Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>
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
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
import sys, time
libdir = os.path.join(os.path.dirname(__file__), '..')
if libdir[-1] != os.path.sep:
    libdir += os.path.sep
sys.path.insert(0, libdir)
import pycdio
import cdio
from time import sleep

sleep_time = 15
if sys.argv[1:]:
    try:
        d = cdio.Device(sys.argv[1])
    except IOError:
        print "Problem opening CD-ROM: %s" % sys.argv[1]
        sys.exit(1)
    if sys.argv[2:]:
        try: 
            sleep_time = int(sys.argv[2])
        except ValueError, msg:
            print "Invalid sleep parameter %s" % sys.argv[2]
            sys.exit(2)
else:
    try:
        d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
    except IOError:
        print "Problem finding a CD-ROM"
        sys.exit(1)

if d.get_media_changed():
    print "Initial media status: changed"
else:
    print "Initial media status: not changed"

print "Giving you %lu seconds to change CD if you want to do so." % sleep_time
sleep(sleep_time)
if d.get_media_changed():
    print "Media status: changed"
else:
    print "Media status: not changed"
d.close()
