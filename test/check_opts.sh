#!/bin/sh
#$Id: check_opts.sh,v 1.2 2003/04/25 21:28:39 rocky Exp $
# Check cd-info options
if test -z "$srcdir" ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`

fname=isofs-m1
i=0
for opt in '-T' '--no-tracks' '-A' '--no-analyze' '-I' '-no-ioctl' \
      '-q' '--quiet' ; do 
  testname=${BASE}$i
  test_cdinfo "--cue-file ${srcdir}/${fname}.cue $opt" \
    ${testname}.dump ${srcdir}/${testname}.right
  RC=$?
  check_result $RC "cd-info option test $opt"
  i=`expr $i + 1`
done

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
