#!/usr/bin/env ruby
#$Id: device.rb,v 1.1 2006/11/13 05:12:43 rocky Exp $
# Program to show CD-ROM device information
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

# Return sorted keys of a dictionary.
# There's probably an easier way to do this that I'm not aware of.
def sort_dict_keys(dict)
    keys=dict.keys()
    keys.sort()
    return keys
end

if ARGV.length() > 0
    begin
        drive_name = ARGV[0]
        d = Device.new(drive_name)
    rescue IOError
        puts "Problem opening CD-ROM: %s" % drive_name
        exit(1)
    end
else
    begin
        d = Device.new("", Rubycdio::DRIVER_UNKNOWN)
        drive_name = d.get_device()
    rescue IOError:
        puts "Problem finding a CD-ROM"
        exit(1)
    end
end

# Should there should be no "ok"?
ok, vendor, model, release = d.get_hwinfo()

puts "drive: %s, vendor: %s, model: %s, release: %s" \
      % [drive_name, vendor, model, release]

read_cap, write_cap, misc_cap = d.get_drive_cap()
puts "Drive Capabilities for %s..." % drive_name

# FIXME
# puts "\n".join(cap for cap in sort_dict_keys(read_cap) +
#                sort_dict_keys(write_cap) + sort_dict_keys(misc_cap))

puts "\nDriver Availabiliity..."
drivers.each_pair { |driver_name, driver_id|
    begin 
        if Rubycdio::have_driver(driver_id) == 1:
            puts "Driver %s is installed." % driver_name
        end
    rescue ValueError
    end
}
d.close()
