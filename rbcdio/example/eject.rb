#!/usr/bin/env ruby
#$Id: eject.rb,v 1.1 2006/11/13 05:12:43 rocky Exp $
# Program to Eject and close CD-ROM drive
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
        drive_name=@ARGV[1]
        d = Device.new(drive_name)
    rescue IOError
        print "Problem opening CD-ROM: %s" % drive_name
        exit 1
    end
else
    begin
        d = Device.new(nil, Rubycdio::DRIVER_UNKNOWN)
        drive_name = d.get_device()
    rescue IOError
        print "Problem finding a CD-ROM"
        exit 1
    end
end

begin
    puts "Ejecting CD in drive %s" % drive_name
    d.eject_media()
    begin
        Rubycdio::close_tray(drive_name)
        puts "Closed tray of CD-ROM drive %s" % drive_name
    rescue DeviceException
        puts "Closing tray of CD-ROM drive %s failed" % drive_name
    end

rescue DriverUnsupportedError
    puts "Eject not supported for %s" % drive_name
rescue DeviceException
    puts "Eject of CD-ROM drive %s failed" % drive_name
end

    

  
