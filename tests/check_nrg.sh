#!/bin/sh
#$Id: check_nrg.sh,v 1.2 2003/04/19 19:12:06 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`
nrg_file=${srcdir}/monvoisin.nrg

if test -f  $nrg_file ; then
  test_cdinfo "--nrg-file $nrg_file" \
    monvoisin.dump ${srcdir}/monvoisin.right
  RC=$?
  check_result $RC 'cdinfo NRG test 1'
  
  exit $RC
else 
  echo "Don't see NRG file ${nrg_file}. Test skipped."
  exit $SKIP_TEST_EXITCODE
fi

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
