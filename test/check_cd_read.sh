#!/bin/sh
#$Id: check_cd_read.sh,v 1.2 2003/09/19 04:34:53 rocky Exp $
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
# Tests to see that CD reading is correct (via cd-read). 

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

if ! test -x ../src/cd-read ; then
  exit 77
fi

BASE=`basename $0 .sh`

fname=cdda
testnum=CD-DA
test_cd_read "${srcdir}/${fname}.cue 1 0" \
  ${fname}-read.dump ${srcdir}/${fname}-read.right
RC=$?
check_result $RC "cd-read CUE test $testnum"

fname=isofs-m1
testnum=MODE1
test_cd_read "${srcdir}/${fname}.cue 1 0" \
  ${fname}-read.dump ${srcdir}/${fname}-read.right
RC=$?
check_result $RC "cd-read CUE test $testnum"

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
