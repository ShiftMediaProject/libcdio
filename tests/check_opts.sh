#!/bin/sh
#$Id: check_opts.sh,v 1.1 2003/04/14 08:51:14 rocky Exp $
# Check cdinfo options

if test -z $srcdir ; then
  srcdir=`pwd`
fi

. ${srcdir}/check_common_fn

BASE=`basename $0 .sh`

fname=fsf
opts=('-T' '--no-tracks' '-A', '--no-analyze' '-I' '-no-ioctl' 
      '-q' '--quiet')

for (( i=0 ; i<${#opts[@]} ; i++ )) ; do 
  testname=${BASE}$i
  test_cdinfo "--cue-file ${srcdir}/${fname}.cue --no-cddb ${opts[$i]}" \
    ${testname}.dump ${srcdir}/${testname}.right
  RC=$?
  check_result $RC "cdinfo option test $i"
done

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
