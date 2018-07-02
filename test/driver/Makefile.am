#   Copyright (C) 2009, 2010, 2012, 2017 Rocky Bernstein <rocky@gnu.org>
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

.PHONY: check-leaks test check-short

DATA_DIR = $(abs_top_srcdir)/test/data
AM_CPPFLAGS = $(LIBCDIO_CFLAGS) $(LIBISO9660_CFLAGS) -DDATA_DIR=\"$(DATA_DIR)\"

abs_path_SOURCES   = abs_path.c
abs_path_LDADD     = $(LIBCDIO_LIBS) $(LTLIBICONV)

bincue_SOURCES   = helper.c bincue.c
bincue_LDADD     = $(LIBCDIO_LIBS) $(LTLIBICONV)

cdda_SOURCES     = helper.c cdda.c
cdda_LDADD       = $(LIBCDIO_LIBS) $(LTLIBICONV)

cdrdao_SOURCES   = helper.c cdrdao.c
cdrdao_LDADD     = $(LIBCDIO_LIBS) $(LTLIBICONV)

cdtext_SOURCES   = cdtext.c
cdtext_LDADD     = $(LIBCDIO_LIBS) $(LTLIBICONV)

freebsd_LDADD    = $(LIBCDIO_LIBS) $(LTLIBICONV)

realpath_LDADD   = $(LIBCDIO_LIBS) $(LTLIBICONV)

gnu_linux_SOURCES= helper.c gnu_linux.c
gnu_linux_LDADD  = $(LIBCDIO_LIBS) $(LTLIBICONV)

logger_LDADD     = $(LIBCDIO_LIBS) $(LTLIBICONV)

mmc_read_LDADD   = $(LIBCDIO_LIBS) $(LTLIBICONV)

mmc_write_LDADD  = $(LIBCDIO_LIBS) $(LTLIBICONV)

nrg_SOURCES      = helper.c nrg.c
nrg_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

osx_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

track_SOURCES      = track.c
track_LDADD        = $(LIBCDIO_LIBS)

solaris_LDADD    = $(LIBCDIO_LIBS) $(LTLIBICONV)

win32_LDADD      = $(LIBCDIO_LIBS) $(LTLIBICONV)

check_PROGRAMS   = \
	abs_path bincue cdda cdrdao cdtext freebsd gnu_linux \
	logger mmc_read mmc_write nrg \
	osx realpath solaris track win32

TESTS = $(check_PROGRAMS)

EXTRA_DIST = \
	bincue.c \
	helper.c    \
	helper.h    \
	cdrdao.c \
	cdtext.c \
	nrg.c    \
	track.c

MOSTLYCLEANFILES = \
	$(check_PROGRAMS) \
	core core.* *.dump cdda-orig.wav cdda-try.wav *.raw

#: run regression tests. "test" is the same thing as "check"
test: check-am

#: run valgrind on C progrma
check-leaks: $(check_PROGRAMS)
	for p in $(check_PROGRAMS); do \
	   valgrind ./$$p; \
        done

#: Run all tests without bloated output
check-short:
	$(MAKE) check 2>&1  | ruby @abs_top_srcdir@/make-check-filter.rb
