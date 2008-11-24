#!/usr/bin/env python

""" A simple program to show using libiso9660 to list files in a directory of
   an ISO-9660 image.

   If a single argument is given, it is used as the ISO 9660 image to
   use in the listing. Otherwise a compiled-in default ISO 9660 image
   name (that comes with the libcdio distribution) will be used."""

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
import iso9660

# The default ISO 9660 image if none given
ISO9660_IMAGE_PATH="../data"
ISO9660_IMAGE=os.path.join(ISO9660_IMAGE_PATH, "copying.iso")

if len(sys.argv) > 1:
    iso_image_fname = sys.argv[1]
else:    
    iso_image_fname = ISO9660_IMAGE

iso = iso9660.ISO9660.IFS(source=iso_image_fname)
  
if not iso.is_open():
    print "Sorry, couldn't open %s as an ISO-9660 image." %  iso_image_fname
    sys.exit(1)

path = '/'

file_stats = iso.readdir(path)

id = iso.get_application_id()
if id is not None: print "Application ID: %s" % id

id = iso.get_preparer_id()
if id is not None: print "Preparer ID: %s" % id

id = iso.get_publisher_id()
if id is not None: print "Publisher ID: %s" % id

id = iso.get_system_id()
if id is not None: print "System ID: %s" % id

id = iso.get_volume_id()
if id is not None: print "Volume ID: %s" % id

id = iso.get_volumeset_id()
if id is not None: print "Volumeset ID: %s" % id

dir_tr=['-', 'd']

for stat in file_stats:
    # FIXME
    filename = stat[0]
    LSN      = stat[1]
    size     = stat[2]
    sec_size = stat[3]
    is_dir   = stat[4] == 2
    print "%s [LSN %6d] %8d %s%s" % (dir_tr[is_dir], LSN, size, path,
                                     iso9660.name_translate(filename))
iso.close()
sys.exit(0)


