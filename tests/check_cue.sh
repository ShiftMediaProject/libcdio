#!/bin/sh
#$Id: check_cue.sh,v 1.5 2003/04/19 19:12:06 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`

fname=fsf-tompox
testnum=1
test_cdinfo "--cue-file ${srcdir}/${fname}.cue" \
  ${fname}.dump ${srcdir}/${fname}.right
RC=$?
check_result $RC "cdinfo CUE test $testnum"

fname=vcd_demo
testnum=2
if test -f ${srcdir}/${fname}.cue ; then
  test_cdinfo "-c ${srcdir}/vcd_demo.cue" \
      ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC "cdinfo CUE test $testnum"
else 
  echo "Don't see CUE file ${srcdir}/${fname}.cue. Test $testum skipped."
fi

fname=svcd_ogt_test_ntsc
testnum=3
if test -f ${srcdir}/${fname}.bin ; then
  test_cdinfo "--cue-file ${srcdir}/${fname}.cue" \
    ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC "cdinfo CUE test $testnum"
else 
  echo "Don't see CUE file ${srcdir}/${fname}.bin. Test $testnum skipped."
fi

fname=fsf
testnum=4
if test -f  ${srcdir}/${fname}.bin ; then
  test_cdinfo "--cue-file ${srcdir}/${fname}.cue --no-cddb" \
    ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC "cdinfo CUE test $testnum"
else 
  echo "Don't see CUE file ${srcdir}/${fname}.bin. Test $testum skipped."
fi

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
