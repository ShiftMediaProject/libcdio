#!/bin/sh
#$Id: check_cue.sh,v 1.3 2003/04/14 04:28:40 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`

fname=fsf
test_cdinfo "--cue-file ${srcdir}/${fname}.cue --nocddb" \
  ${fname}.dump ${srcdir}/${fname}.right
RC=$?
check_result $RC 'cdinfo CUE test 1'

fname=fsf-tompox
test_cdinfo "--cue-file ${srcdir}/${fname}.cue" \
  ${fname}.dump ${srcdir}/${fname}.right
RC=$?
check_result $RC 'cdinfo CUE test 2'

fname=svcd_ogt_test_ntsc
if test -f ${srcdir}/${fname}.cue ; then
  test_cdinfo "--cue-file ${srcdir}/${fname}.cue" \
    ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC 'cdinfo CUE test 3'
fi

fname=vcd_demo
if test -f ${srcdir}/${fname}.cue ; then
  test_cdinfo "-c ${srcdir}/vcd_demo.cue" \
      ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC 'cdinfo CUE test 4'
fi

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
