#!/bin/sh
#$Id: check_opts.sh,v 1.6 2004/05/07 02:15:56 rocky Exp $
# Check cd-info options
if test -z "$srcdir" ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

if test ! -x ../src/cd-info ; then
  exit 77
fi

BASE=`basename $0 .sh`

fname=isofs-m1
i=0
for opt in '-T' '--no-tracks' '-A' '--no-analyze' '-I' '--no-ioctl' \
      '-q' '--quiet' ; do 
  testname=${BASE}$i
  opts="--cue-file ${srcdir}/${fname}.cue $opt --quiet"
  test_cdinfo  "$opts" ${testname}.dump ${srcdir}/${testname}.right
  RC=$?
  check_result $RC "cd-info option test $opt" "cd-info $opts"
  i=`expr $i + 1`
done

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
