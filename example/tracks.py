#!/usr/bin/env python
"""A program to show CD information."""
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
        d = cdio.Device(sys.argv[1])
    except IOError:
        print "Problem opening CD-ROM: %s" % sys.argv[1]
        sys.exit(1)
else:
    try:
        d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
    except IOError:
        print "Problem finding a CD-ROM"
        sys.exit(1)

t = d.get_first_track()
if t is None: 
    print "Problem getting first track"
    sys.exit(2)

first_track = t.track
num_tracks  = d.get_num_tracks()
last_track  = first_track+num_tracks-1

try:
    last_session = d.get_last_session()
    print "CD-ROM %s has %d track(s) and %d session(s)." % \
          (d.get_device(), d.get_num_tracks(), last_session)
except cdio.DriverUnsupportedError:
    print "CD-ROM %s has %d track(s). " % (d.get_device(), d.get_num_tracks())

print "Track format is %s." %  d.get_disc_mode()

mcn = d.get_mcn()
if mcn:
    print "Media Catalog Number: %s" % mcn 
    
print "%3s: %-6s  %s" % ("#", "LSN", "Format")
i=first_track
while (i <= last_track):
    try: 
        t = d.get_track(i)
        print "%3d: %06lu  %-6s %s"  % (t.track, t.get_lsn(),
                                        t.get_msf(), t.get_format())
    except cdio.TrackError:
        pass
    i += 1

print "%3X: %06lu  leadout" \
      % (pycdio.CDROM_LEADOUT_TRACK, d.get_disc_last_lsn())
d.close()
