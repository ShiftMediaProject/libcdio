#!/bin/sh
#$Id: check_cue.sh,v 1.1 2003/03/24 19:01:10 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`

test_cdinfo "--cue-file ${srcdir}/svcd_ogt_test_ntsc.cue" \
  svcd_ogt_test_ntsc.dump ${srcdir}/svcd_ogt_test_ntsc.right
RC=$?
check_result $RC 'cdinfo CUE test 1'

test_cdinfo "-c ${srcdir}/vcd_demo.cue" \
    vcd_demo.dump ${srcdir}/vcd_demo.right
RC=$?
check_result $RC 'cdinfo CUE test 2'

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
