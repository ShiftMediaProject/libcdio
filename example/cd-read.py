#!/usr/bin/env python
"""Program to read CD blocks. See read-cd from the libcdio distribution
for a more complete program.
"""
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

import sys, os
from optparse import OptionParser

libdir = os.path.join(os.path.dirname(__file__), '..')
if libdir[-1] != os.path.sep:
    libdir += os.path.sep
sys.path.insert(0, libdir)
import pycdio
import cdio

read_modes = {
    'audio': pycdio.READ_MODE_AUDIO,
    'm1f1' : pycdio.READ_MODE_M1F1,
    'm1f2' : pycdio.READ_MODE_M1F2,
    'm2f1' : pycdio.READ_MODE_M2F1,
    'm2f2' : pycdio.READ_MODE_M2F2,
    'data' : None
    }

def process_options():
    program = os.path.basename(sys.argv[0])
    usage_str="""%s --mode *mode* [options] [device]
    Read blocks of a CD""" % program

    optparser = OptionParser(usage=usage_str)

    optparser.add_option("-m", "--mode", dest="mode",
                         action="store", type='string',	
                         help="CD Reading mode: audio, m1f1, m1f2, " + 
	 	              "m2f1 or m2f2")
    optparser.add_option("-s", "--start", dest="start",
                         action="store", type='int', default=1,
                         help="Starting block")
    optparser.add_option("-n", "--number", dest="number",
                         action="store", type='int', default=1,
                         help="Number of blocks")
    (opts, argv) = optparser.parse_args()
    if opts.mode is None:
        print "Mode option must given " + \
              "(and one of audio, m1f1, m1f2, m1f2 or m1f2)."
        sys.exit(1)
    try:
        read_mode = read_modes[opts.mode]
    except KeyError:
        print "Need to use the --mode option with one of" + \
              "audio, m1f1, m1f2, m1f2 or m1f2"
        sys.exit(2)
        
    return opts, argv, read_mode

import re
PRINTABLE = r'[ -~]'
pat = re.compile(PRINTABLE)
def isprint(c):
    global pat
    return pat.match(c)

def hexdump (buffer, just_hex=False):
    i = 0
    while i < len(buffer):
        if (i % 16) == 0:
            print ("0x%04x: " % i),
        print "%02x%02x" % (ord(buffer[i]), ord(buffer[i+1])),
        if (i % 16) == 14:
            if not just_hex:
                s = "  "
                for j in range(i-14, i+2):
                    if isprint(buffer[j]):
                        s += buffer[j]
                    else:
                        s += '.'
                print s,
            print
        i += 2
    print
    return

opts, argv, read_mode = process_options()
# While sys.argv[0] is a program name and sys.argv[1] the first
# option, argv[0] is the first unprocessed option -- roughly
# the equivalent of sys.argv[1].
if argv[0:]:
    try:
        d = cdio.Device(argv[0])
    except IOError:
        print "Problem opening CD-ROM: %s" % argv[0]
        sys.exit(1)
else:
    try:
        d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
    except IOError:
        print "Problem finding a CD-ROM"
        sys.exit(1)

## All this setup just to issue this one of these commands.
if read_mode == None:
    blocks, data=d.read_data_blocks(opts.start, opts.number)
else:
    blocks, data=d.read_sectors(opts.start, read_mode, opts.number)
hexdump(data)

