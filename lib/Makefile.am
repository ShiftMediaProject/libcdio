#   $Id: Makefile.am,v 1.48 2004/09/02 00:49:31 rocky Exp $
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
#     update, then increment REVISION (`C:R:A' becomes `C:R+1:A').
#
#  4. If any interfaces have been added, removed, or changed since the
#     last update, increment CURRENT, and set REVISION to 0.
#
#  5. If any interfaces have been added since the last public release,
#     then increment AGE.
#
#  6. If any interfaces have been removed since the last public release,
#     then set AGE to 0.

libcdio_la_CURRENT := 2
libcdio_la_REVISION := 0
libcdio_la_AGE := 2

libiso9660_la_CURRENT := 2
libiso9660_la_REVISION := 0
libiso9660_la_AGE := 0

EXTRA_DIST = image/Makefile FreeBSD/Makefile MSWindows/Makefile 

noinst_HEADERS = cdio_assert.h bytesex.h bytesex_asm.h \
	cdio_private.h ds.h 

libcdio_sources = \
	_cdio_bsdi.c \
	_cdio_generic.c \
	_cdio_linux.c \
	_cdio_osx.c \
	_cdio_stdio.c \
	_cdio_stdio.h \
	_cdio_stream.c \
	_cdio_stream.h \
	_cdio_sunos.c \
	bytesex.h \
	bytesex_asm.h \
	cd_types.c \
	cdio.c \
	cdtext.c \
	cdtext_private.h \
	ds.c \
	ds.h \
        FreeBSD/freebsd.c \
        FreeBSD/freebsd.h \
        FreeBSD/freebsd_cam.c \
	FreeBSD/freebsd_ioctl.c \
	generic.h \
	image.h \
	image/bincue.c \
	image/cdrdao.c \
	image_common.h \
	image/nrg.c \
	image/nrg.h \
	logging.c \
	MSWindows/aspi32.c \
	MSWindows/aspi32.h \
	MSWindows/win32_ioctl.c \
	MSWindows/win32.c \
	MSWindows/win32.h \
	scsi_mmc.c \
	scsi_mmc_private.h \
	sector.c \
	util.c

lib_LTLIBRARIES    = libcdio.la libiso9660.la

libcdio_la_SOURCES = $(libcdio_sources)
libcdio_la_ldflags = -version-info $(libcdio_la_CURRENT):$(libcdio_la_REVISION):$(libcdio_la_AGE)

libiso9660_la_SOURCES = \
	iso9660.c \
	iso9660_private.h \
	iso9660_fs.c \
	xa.c

libiso9660_la_LIBADD = libcdio.la
libiso9660_la_ldflags = -version-info $(libiso9660_la_CURRENT):$(libiso9660_la_REVISION):$(libiso9660_la_AGE)
libiso9660_la_dependencies = libcdio.la

INCLUDES = $(LIBCDIO_CFLAGS)

########################################################
# Things to version the symbols in the libraries
########################################################

# An explanation of the versioning problem from Nicolas Boullis and
# the versioned symbol solution he uses below...
# 
# Currently, libvcdinfo uses the cdio_open function from libcdio.
# Let's imagine a program foobar that uses both the vcdinfo_open
# function from libvcdinfo and the cdio_open function from libcdio.

# Currently, libcdio has SONAME libcdio.so.0, libvcdinfo has SONAME
# libvcdinfo.so.0 and requires libcdio.so.0, and foobar requires both
# libvcdinfo.so.0 and libcdio.so.0. Everything looks fine.
#
# Now, for some reason, you decide to change the cdio_open function.
# That's your right, but you have to bump the CURRENT version and (if I
# understand it correctly, athough this is not that clear in libtool's
# documentation) set the AGE to 0. Anyway, this bumps the SONAME, which is
# sane since the interface changes incompatibly.

# Now, you have a new libcdio with SONAME libcdio.so.1. But libvcdinfo and
# foobar still require libcdio.so.0. Everything is still fine.

# Now, after some minor changes, the author of foobar recompiles foobar.
# Then, foobar now requires libvcdinfo.so.0 and libcdio.so.1. And foobar
# now segfaults...

# What is happening? When you run foobar, if brings both libvcdinfo.so.0
# and libcdio.so.1, but libvcdinfo.so.0 also brings libcdio.so.0. So you
# have both libcdio.so.0 and libcdio.so.1 that bring their symbols to the
# global namespace. Hence, you have to incompatible versions of the
# cdio_open function in the name space. When foobar calls cdio_open, it
# may choose the wrong function, and segfaults...

# With versioned symbols, the cdio_open function from libcdio.so.0 may be
# known as (something that looks like) cdio_open@@CDIO_0. An the cdio_open
# function from libcdio.so.1 as cdio_open@@CDIO_1. Both versions of
# libcdio would still be brought in by the most recent foobar, but foobar
# (and libvcdinfo) know which versioned function to use and then use the
# good one.


# This is some simple versioning where every symbol is versioned with
# something that looks like the SONAME of the library. More complex (and
# better) versioning is possible; it is for example what is used by glibc.
# But good complex versioning is something that requires much more
# work...


# The below is a impliments symbol versioning.  First of all, I
# compute MAJOR as CURENT - AGE; that is what is used within libtool
# (at least on GNU/Linux systems) for the number in the SONAME.  The
# nm command gives the list of symbols known in each of the object
# files that will be part of the shared library. And the sed command
# extracts from this list those symbols that will be shared. (This sed
# command comes from libtool.)

libcdio_la_MAJOR := $(shell expr $(libcdio_la_CURRENT) - $(libcdio_la_AGE))
if BUILD_VERSIONED_LIBS
libcdio_la_LDFLAGS = $(libcdio_la_ldflags) -Wl,--version-script=libcdio.la.ver
libcdio_la_DEPENDENCIES = libcdio.la.ver

libcdio.la.ver: $(libcdio_la_OBJECTS)
	echo 'CDIO_$(libcdio_la_MAJOR) { ' > $@
	echo "  global:" >> $@
	nm $(patsubst %.lo,%.o,$(libcdio_la_OBJECTS)) | sed -n -e 's/^.*[ 	][ABCDGIRSTW][ABCDGIRSTW]*[ 	][ 	]*\([_A-Za-z][_A-Za-z0-9]*\)$$/    \1;/p' | sort | uniq >> $@
	echo '};' >> $@
else 
libcdio_la_LDFLAGS = $(libcdio_la_ldflags)
endif 

libiso9660_la_MAJOR := $(shell expr $(libiso9660_la_CURRENT) - $(libiso9660_la_AGE))
if BUILD_VERSIONED_LIBS
libiso9660_la_LDFLAGS = $(libiso9660_la_ldflags) -Wl,--version-script=libiso9660.la.ver
libiso9660_la_DEPENDENCIES = $(libcdio9660_la_dependencies) libiso9660.la.ver

libiso9660.la.ver: $(libiso9660_la_OBJECTS)
	echo 'ISO9660_$(libiso9660_la_MAJOR) {' > $@
	echo "  global:" >> $@
	nm $(patsubst %.lo,%.o,$(libiso9660_la_OBJECTS)) | sed -n -e 's/^.*[ 	][ABCDGIRSTW][ABCDGIRSTW]*[ 	][ 	]*\([_A-Za-z][_A-Za-z0-9]*\)$$/    \1;/p' | sort | uniq >> $@
	echo '};' >> $@

MOSTLYCLEANFILES = libcdio.la.ver libiso9660.la.ver
else 
libiso9660_la_LDFLAGS = $(libiso9660_la_ldflags)
libiso9660_la_DEPENDENCIES = $(libcdio9660_la_dependencies)
endif
