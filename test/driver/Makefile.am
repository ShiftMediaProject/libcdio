#   Copyright (C) 2009, 2010 Rocky Bernstein <rocky@gnu.org>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS) $(LIBISO9660_CFLAGS)
DATA_DIR = $(abs_top_srcdir)/test/data

bincue_LDADD     = $(LIBCDIO_LIBS) $(LTLIBICONV)
bincue_CFLAGS    = -DDATA_DIR=\"$(DATA_DIR)\"

freebsd_LDADD    = $(LIBCDIO_LIBS) $(LTLIBICONV)
freebsd_CFLAGS   = -DDATA_DIR=\"$(DATA_DIR)\"

realpath_LDADD   = $(LIBCDIO_LIBS) $(LTLIBICONV)
realpath_CFLAGS  = -DDATA_DIR=\"$(DATA_DIR)\"

gnu_linux_LDADD  = $(LIBCDIO_LIBS) $(LTLIBICONV)
gnu_linux_CFLAGS = -DDATA_DIR=\"$(DATA_DIR)\"

mmc_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)
mmc_CFLAGS       = -DDATA_DIR=\"$(DATA_DIR)\"

nrg_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)
nrg_CFLAGS       = -DDATA_DIR=\"$(DATA_DIR)\"

osx_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)
osx_CFLAGS       = -DDATA_DIR=\"$(DATA_DIR)\"

solaris_LDADD    = $(LIBCDIO_LIBS) $(LTLIBICONV)
solaris_CFLAGS   = -DDATA_DIR=\"$(DATA_DIR)\"

win32_LDADD      = $(LIBCDIO_LIBS) $(LTLIBICONV)
win32_CFLAGS     = -DDATA_DIR=\"$(DATA_DIR)\"

check_PROGRAMS   = \
	bincue freebsd gnu_linux mmc nrg \
	osx realpath solaris win32

TESTS = $(check_PROGRAMS)

EXTRA_DIST = \
	bincue.c.in \
	nrg.c.in

MOSTLYCLEANFILES = \
	$(check_PROGRAMS) \
	core core.* *.dump cdda-orig.wav cdda-try.wav *.raw

test: check-am

