#!/bin/sh
#$Id: check_nrg.sh,v 1.1 2003/03/24 19:01:10 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`

test_cdinfo "--nrg-file ${srcdir}/monvoisin.nrg" \
  monvoisin.dump ${srcdir}/monvoisin.right
RC=$?
check_result $RC 'cdinfo NRG test 1'

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
