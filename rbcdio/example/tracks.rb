#!/usr/bin/env ruby
#$Id: tracks.rb,v 1.1 2006/11/13 05:12:43 rocky Exp $
# Program to show CD information
#
#    Copyright (C) 2006 Rocky Bernstein <rocky@gnu.org>
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
$: << File.dirname(__FILE__) + '/../lib/'
require "cdio"

if ARGV.length() > 0
    begin
        d = Device.new(@ARGV[1])
    rescue IOError
        puts "Problem opening CD-ROM: %s" % @ARGV[1]
        exit(1)
    end
else
    begin
        d = Device.new("", Rubycdio::DRIVER_UNKNOWN)
    rescue IOError
        puts "Problem finding a CD-ROM"
        exit(1)
    end
end

t = d.get_first_track()
if not t
    puts "Problem getting first track"
    exit(2)
end

first_track = t.track
num_tracks  = d.get_num_tracks()
last_track  = first_track+num_tracks-1

begin
    last_session = d.get_last_session()
    puts "CD-ROM %s has %d track(s) and %d session(s)." % 
    [d.get_device(), d.get_num_tracks(), last_session]
rescue DriverUnsupportedError:
    puts "CD-ROM %s has %d track(s). " % [d.get_device(), d.get_num_tracks()]
end

puts "Track format is %s." %  d.get_disc_mode()

mcn = d.get_mcn()
if mcn
    puts "Media Catalog Number: %s" % mcn 
end
    
puts "%3s: %-6s  %s" % ["#", "LSN", "Format"]
i=first_track
while (i <= last_track):
    begin 
        t = d.get_track(i)
        puts "%3d: %06u  %-6s %s"  % [t.track, t.get_lsn(),
                                      t.get_msf(), t.get_format()]
    rescue TrackError
    end
    i += 1
end

puts "%3X: %06u  leadout" \
      % [Rubycdio::CDROM_LEADOUT_TRACK, d.get_disc_last_lsn()]
d.close()
