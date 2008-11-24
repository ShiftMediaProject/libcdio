#!/usr/bin/env python
"""A program to show use of audio controls. See cdda-player from the
libcdio distribution for a more complete program.
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

def process_options():
    program = os.path.basename(sys.argv[0])
    usage_str="""%s [options] [device]
    Issue analog audio CD controls - like playing""" % program

    optparser = OptionParser(usage=usage_str)

    optparser.add_option("-c", "--close", dest="close",
                         action="store", type='string',	
                         help="close CD tray")
    optparser.add_option("-p", "--play", dest="play",
                         action="store_true", default=False,
                         help="Play entire CD")
    optparser.add_option("-P", "--pause", dest="pause",
                         action="store_true", default=False,
                         help="pause playing")
    optparser.add_option("-E", "--eject", dest="eject",
                         action="store_true", default=False,
                         help="Eject CD")
    optparser.add_option("-r", "--resume", dest="resume",
                         action="store_true", default=False,
                         help="resume playing")
    optparser.add_option("-s", "--stop", dest="stop",
                         action="store_true", default=False,
                         help="stop playing")
    optparser.add_option("-t", "--track", dest="track",
                         action="store", type='int',
                         help="play a single track")
    return optparser.parse_args()

opts, argv = process_options()

# Handle closing the CD-ROM tray if that was specified.
if opts.close:
    try:
        device_name = opts.close
        cdio.close_tray(device_name)
    except cdio.DeviceException:
        print "Closing tray of CD-ROM drive %s failed" % device_name
        

# While sys.argv[0] is a program name and sys.argv[1] the first
# option, argv[0] is the first unprocessed option -- roughly
# the equivalent of sys.argv[1].
if argv[0:]:
    try:
        d = cdio.Device(argv[0])
    except IOError:
        print "Problem opening CD-ROM: %s" % device_name
        sys.exit(1)
else:
    try:
        d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
    except IOError:
        print "Problem finding a CD-ROM"
        sys.exit(1)

device_name=d.get_device()
if opts.play:
    if d.get_disc_mode() != 'CD-DA':
        print "The disc on %s I found was not CD-DA, but %s" \
              % (device_name, d.get_disc_mode())
        print "I have bad feeling about trying to play this..."

    try: 
        start_lsn = d.get_first_track().get_lsn()
        end_lsn=d.get_disc_last_lsn()
        print "Playing entire CD on %s"  % device_name
        d.audio_play_lsn(start_lsn, end_lsn)
    except cdio.TrackError:
        pass

elif opts.track is not None:
    try:
        if opts.track > d.get_last_track().track:
            print "Requested track %d but CD only has %d tracks" \
                  % (opts.track, d.get_last_track().track)
            sys.exit(2)
        if opts.track < d.get_first_track().track:
            print "Requested track %d but first track on CD is %d" \
                  % (opts.track, d.get_first_track().track)
            sys.exit(2)
        print "Playing track %d on %s"  % (opts.track, device_name)
        start_lsn = d.get_track(opts.track).get_lsn()
        end_lsn = d.get_track(opts.track+1).get_lsn()
        d.audio_play_lsn(start_lsn, end_lsn)
    except cdio.TrackError:
         pass
        
elif opts.pause:
    try:
        print "Pausing playing in drive %s" % device_name
        d.audio_pause()
    except cdio.DeviceException:
        print "Pause failed on drive %s" % device_name
elif opts.resume:
    try:
        print "Resuming playing in drive %s" % device_name
        d.audio_resume()
    except cdio.DeviceException:
        print "Resume failed on drive %s" % device_name
elif opts.stop:
    try:
        print "Stopping playing in drive %s" % device_name
        d.audio_stop()
    except cdio.DeviceException:
        print "Stopping failed on drive %s" % device_name
elif opts.eject:
    try:
        print "Ejecting CD in drive %s" % device_name
        d.eject_media()
    except cdio.DriverUnsupportedError:
        print "Eject not supported for %s" % device_name
    except cdio.DeviceException:
        print "Eject of CD-ROM drive %s failed" % device_name
        
d.close()
