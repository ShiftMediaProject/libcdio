#!/bin/sh
#$Id: check_nrg.sh,v 1.3 2003/04/26 14:24:45 rocky Exp $

if test -n "-lvcd -lvcdinfo" ; then
  vcd_opt='--no-vcd'
fi

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`
fname=videocd

test_cdinfo "--nrg-file ${srcdir}/${fname}.nrg $vcd_opt " \
  ${fname}.dump ${srcdir}/${fname}.right
RC=$?
check_result $RC 'cd-info NRG test 1'

BASE=`basename $0 .sh`
nrg_file=${srcdir}/monvoisin.nrg

if test -f  $nrg_file ; then
  test_cdinfo "--nrg-file $nrg_file $vcd_opt " \
    monvoisin.dump ${srcdir}/monvoisin.right
  RC=$?
  check_result $RC 'cd-info NRG test 1'
else 
  echo "Don't see NRG file ${nrg_file}. Test skipped."
  exit $SKIP_TEST_EXITCODE
fi

nrg_file=${srcdir}/svcdgs.nrg
if test -f  $nrg_file ; then
  test_cdinfo "--nrg-file $nrg_file" \
    svcdgs.dump ${srcdir}/svcdgs.right
  RC=$?
  check_result $RC 'cd-info NRG test 2'
  
  exit $RC
else 
  echo "Don't see NRG file ${nrg_file}. Test skipped."
  exit $SKIP_TEST_EXITCODE
fi

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
