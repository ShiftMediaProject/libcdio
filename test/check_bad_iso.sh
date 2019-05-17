#!/bin/sh

if test -z $srcdir ; then
  srcdir=$(pwd)
fi

if test "X$top_builddir" = "X" ; then
  top_builddir=$(pwd)/..
fi

. ${top_builddir}/test/check_common_fn

if test ! -x ../src/iso-info ; then
  exit 77
fi

BASE=$(basename $0 .sh)
fname=bad-dir

RC=0

opts="--quiet ${srcdir}/../test/data/${fname}.iso"
cmdname=iso-info
cmd=../src/iso-info
if ! "${cmd}" --quiet --no-header ${opts} 2>&1 ; then
    echo "$0: unexpected failure"
    RC=1
fi

opts="--quiet ${srcdir}/test/data/${fname}.iso --iso9660"
if "${cmd}" --no-header ${opts} 2>&1 ; then
    ((RC+=1))
else
    echo "$0: expected failure"
fi

exit $RC

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
