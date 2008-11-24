#!/usr/bin/env python
"""A program to show using iso9660 to extract a file from an ISO-9660
image.

If a single argument is given, it is used as the ISO 9660 image to use
in the extraction. Otherwise a compiled in default ISO 9660 image name
(that comes with the libcdio distribution) will be used.  A program to
show using iso9660 to extract a file from an ISO-9660 image."""
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
import iso9660

# Python has rounding (round) and trucation (int), but what about an integer
# ceiling function? Until I learn what it is...
def ceil(x):
    return int(round(x+0.5))

# The default CD image if none given
cd_image_path="../data"
cd_image_fname=os.path.join(cd_image_path, "isofs-m1.cue")

# File to extract if none given.
iso9660_path="/"
local_filename="COPYING"

if len(sys.argv) > 1:
    cd_image_fname = sys.argv[1]
    if len(sys.argv) > 2:
        local_filename = sys.argv[1]
        if len(sys.argv) > 3:
            print """
usage: %s [CD-ROM-or-image [filename]]
Extracts filename from CD-ROM-or-image.
""" % sys.argv[0]
            sys.exit(1)

try: 
    cd = iso9660.ISO9660.FS(source=cd_image_fname)
except:
    print "Sorry, couldn't open %s as a CD image." % cd_image_fname
    sys.exit(1)

statbuf = cd.stat (local_filename, False)

if statbuf is None:
    print "Could not get ISO-9660 file information for file %s in %s" \
          % (local_filename, cd_image_fname)
    cd.close()
    sys.exit(2)

try:
    OUTPUT=os.open(local_filename, os.O_CREAT|os.O_WRONLY, 0664)
except:    
    print "Can't open %s for writing" % local_filename

# Copy the blocks from the ISO-9660 filesystem to the local filesystem. 
blocks = ceil(statbuf['size'] / pycdio.ISO_BLOCKSIZE)
for i in range(blocks):
    lsn = statbuf['LSN'] + i
    size, buf = cd.read_data_blocks(lsn)

    if size < 0:
        print "Error reading ISO 9660 file %s at LSN %d" % (
            local_filename, lsn)
        sys.exit(4)
    
    os.write(OUTPUT, buf)


# Make sure the file size has the exact same byte size. Without the
# truncate below, the file will a multiple of ISO_BLOCKSIZE.

os.ftruncate(OUTPUT, statbuf['size'])

print "Extraction of file '%s' from %s successful." % (
    local_filename,  cd_image_fname)

os.close(OUTPUT)
cd.close()
sys.exit(0)
