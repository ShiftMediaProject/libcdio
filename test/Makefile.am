#   $Id: Makefile.am,v 1.7 2003/09/14 04:59:40 rocky Exp $
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
# Things to regression testing
####################################################
#
noinst_PROGRAMS = testassert testischar testiso9660 check_sizeof

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

testassert_LDADD = $(LIBCDIO_LIBS)
testischar_LDADD = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
testiso9660_LDADD = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
check_sizeof_LDADD = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)

check_SCRIPTS = check_nrg.sh check_cue.sh check_opts.sh

check_PROGRAMS = $(noinst_PROGRAMS)

check_DATA = vcd_demo.right \
	     videocd.right videocd.nrg \
	     cdda.right cdda.cue cdda.bin \
	     isofs-m1.right isofs-m1.cue isofs-m1.bin \
	     check_opts0.right check_opts1.right check_opts2.right \
             check_opts3.right check_opts4.right check_opts5.right \
             check_opts6.right check_opts7.right	

EXTRA_DIST = $(check_SCRIPTS) $(check_DATA) \
	check_common_fn check_cue.sh.in check_nrg.sh.in

TESTS_ENVIRONMENT=$(SHELL)
TESTS = testischar testiso9660 check_sizeof $(check_SCRIPTS) 

MOSTLYCLEANFILES = core.* *.dump
