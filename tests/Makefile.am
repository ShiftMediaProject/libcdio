#   $Id: Makefile.am,v 1.2 2003/04/06 06:46:52 rocky Exp $
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
check_SCRIPTS = check_nrg.sh check_cue.sh

check_DATA = monvoisin.right svcd_ogt_test_ntsc.right vcd_demo.right \
	     fsf-tompox.right fsf-tompox.cue fsf-tompox.bin

EXTRA_DIST = $(check_SCRIPTS) $(check_DATA) \
	check_common_fn 

TESTS = $(check_SCRIPTS)

MOSTLYCLEANFILES = core.* *.dump
