#!/bin/sh
#$Id: check_opts.sh,v 1.3 2003/04/20 17:24:49 rocky Exp $
# Check cdinfo options
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
  check_result $RC "cdinfo option test $opt"
  i=`expr $i + 1`
done

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
