#   $Id: Makefile.am,v 1.22 2004/05/08 20:36:01 rocky Exp $
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
####################################################
# Things to regression testing
####################################################
#
#
# There's a problem with doing make distcheck for testdefault. 
# A reminder of why I hate automake.
hack = check_sizeof testassert testischar testiso9660 testtoc
noinst_PROGRAMS = $(hack) testdefault 

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

check_sizeof_LDADD  = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
testassert_LDADD    = $(LIBCDIO_LIBS)
testdefault_LDADD   = $(LIBCDIO_LIBS)
testischar_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
testiso9660_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
testtoc_LDADD       = $(LIBCDIO_LIBS)

check_SCRIPTS = check_nrg.sh  check_cue.sh  check_cd_read.sh \
                check_iso.sh  check_opts.sh 

check_PROGRAMS = $(hack)

check_DATA = vcd_demo.right \
	     videocd.right videocd.nrg \
	     cdda.right cdda.toc cdda.cue cdda.bin \
	     isofs-m1.right isofs-m1.toc isofs-m1.cue isofs-m1.bin \
	     check_opts0.right check_opts1.right check_opts2.right \
             check_opts3.right check_opts4.right check_opts5.right \
             check_opts6.right check_opts7.right \
	     isofs-m1-read.right cdda-read.right \
	     t1.toc t2.toc t3.toc t4.toc t5.toc t6.toc t7.toc t8.toc t9.toc \
	     data1.toc data2.toc data5.toc data6.toc data7.doc \
	     bad-mode1.toc bad-msf-1.toc bad-msf-2.toc \
	     copying.iso copying.right

EXTRA_DIST = $(check_SCRIPTS) $(check_DATA) \
	check_common_fn check_cue.sh.in check_nrg.sh.in check_iso.sh.in

TESTS = $(check_PROGRAMS) $(check_SCRIPTS) 
XFAIL_TESTS = testassert

MOSTLYCLEANFILES = core.* *.dump

# This is a really bad hack to make sure check_nrg and check_cue.sh
# are executable. Automake will remake check_nrg.sh and check_cue.sh
# but not run the configure default commands for them to make sure
# they are executable. You know it would be nice one could just set
# permissions and mode when it makes the files. I'm sure there's some
# cleaner a way to do this, but frankly I've wasted far too much of my
# life the crappy automess system that I've really lost interest in
# learning any more of this awful system than I need to.
check-am:	make-executable
make-executable: check_nrg.sh check_cue.sh
	chmod +x *.sh