#   $Id: Makefile.am,v 1.19 2004/02/09 10:32:35 rocky Exp $
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
# Things to make the libcdio and libiso9660 libraries
########################################################
#
# From libtool documentation:
#  1. Start with version information of `0:0:0' for each libtool library.
#
#  2. Update the version information only immediately before a public
#     release of your software.  More frequent updates are unnecessary,
#     and only guarantee that the current interface number gets larger
#     faster.
#
#  3. If the library source code has changed at all since the last
#     update, then increment REVISION (`C:R:A' becomes `C:r+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public release,
#     then set AGE to 0.

libcdio_la_CURRENT := 0
libcdio_la_REVISION := 1
libcdio_la_AGE := 0

libiso9660_la_CURRENT := 0
libiso9660_la_REVISION := 1
libiso9660_la_AGE := 0

noinst_HEADERS = cdio_assert.h bytesex.h bytesex_asm.h \
	cdio_private.h ds.h 

libcdio_sources = \
	_cdio_bincue.c \
	_cdio_bsdi.c \
	_cdio_freebsd.c \
	_cdio_generic.c \
	_cdio_linux.c \
	_cdio_osx.c \
	_cdio_nrg.c \
	_cdio_stdio.c \
	_cdio_stdio.h \
	_cdio_stream.c \
	_cdio_stream.h \
	_cdio_sunos.c \
	_cdio_win32.c \
	_cdio_win32.h \
	bytesex.h \
	bytesex_asm.h \
	cdio.c \
	cd_types.c \
	ds.c \
	ds.h \
	logging.c \
	scsi_mmc.h \
	sector.c \
	util.c \
	win32ioctl.c \
	wnaspi32.c \
	wnaspi32.h

lib_LTLIBRARIES    = libcdio.la libiso9660.la

libcdio_la_SOURCES = $(libcdio_sources)
libcdio_la_LDFLAGS = -version-info $(libcdio_la_CURRENT):$(libcdio_la_REVISION):$(libcdio_la_AGE)

libiso9660_la_SOURCES = \
	iso9660.c \
	iso9660_private.h \
	iso9660_fs.c \
	xa.c

libiso9660_la_LIBADD = $(LIBCDIO_LIBS)
libiso9660_la_LDFLAGS = -version-info $(libiso9660_la_CURRENT):$(libiso9660_la_REVISION):$(libiso9660_la_AGE)

INCLUDES = -I$(LIBCDIO_CFLAGS)
