#   Copyright (C) 2009 Rocky Bernstein <rocky@gnu.org>
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

gnu_linux_LDADD  = $(LIBCDIO_LIBS) $(LTLIBICONV)
gnu_linux_CFLAGS = -DTEST_DIR=\"$(srcdir)\"

mmc_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)
mmc_CFLAGS       = -DTEST_DIR=\"$(srcdir)\"

osx_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)
osx_CFLAGS       = -DTEST_DIR=\"$(srcdir)\"

solaris_LDADD    = $(LIBCDIO_LIBS) $(LTLIBICONV)
solaris_CFLAGS   = -DTEST_DIR=\"$(srcdir)\"

win32_LDADD      = $(LIBCDIO_LIBS) $(LTLIBICONV)
win32_CFLAGS     = -DTEST_DIR=\"$(srcdir)\"

check_PROGRAMS   = gnu_linux mmc osx solaris win32

TESTS = $(check_PROGRAMS)

MOSTLYCLEANFILES = core core.* *.dump cdda-orig.wav cdda-try.wav *.raw

test: check-am

