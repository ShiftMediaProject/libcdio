#   $Id: Makefile.am,v 1.17 2004/02/08 03:35:24 rocky Exp $
#
#    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
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
####################################################
# Things to make the libcdio library
####################################################
#
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

lib_LTLIBRARIES     = libcdio.la libiso9660.la
libcdio_la_SOURCES = $(libcdio_sources)
libcdio_la_LDFLAGS = -no-undefined -version-info 0:1:0

libiso9660_la_SOURCES = \
	iso9660.c \
	iso9660_private.h \
	iso9660_fs.c \
	xa.c

libiso9660_la_LDFLAGS = -no-undefined -version-info 0:1:0

INCLUDES = -I$(LIBCDIO_CFLAGS)
