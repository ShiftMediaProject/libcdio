#   $Id: Makefile.am,v 1.30 2006/02/27 10:10:08 flameeyes Exp $
#
#    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
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
########################################################
# Things to make the install (public) libcdio headers
########################################################
#

libcdioincludedir=$(includedir)/cdio
libcdioinclude_HEADERS = \
	audio.h \
	bytesex.h \
	bytesex_asm.h \
	paranoia.h \
	cdda.h \
	cdio.h \
	cdio_config.h \
	cdtext.h \
	cdtext.h \
	cd_types.h \
	device.h \
	disc.h \
	ds.h \
	dvd.h \
	ecma_167.h \
	iso9660.h \
	logging.h \
	mmc.h \
	posix.h \
	read.h \
	rock.h \
	sector.h \
        track.h \
        types.h \
	udf.h \
	udf_file.h \
	udf_time.h \
	util.h \
	version.h \
	xa.h 

EXTRA_DIST = version.h.in
BUILT_SOURCES = version.h

DISTCLEANFILES = cdio_config.h
