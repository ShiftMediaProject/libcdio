#!/bin/sh
#$Id: check_fuzzyiso.sh,v 1.3 2005/03/02 12:49:28 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

if test -z $top_srcdir ; then
  top_srcdir=`pwd`/..
fi

check_program="$top_srcdir/example/isofuzzy"

if test ! -x $check_program ; then
  exit 77
fi

for file in *.bin *.iso *.nrg ; do 
  case "$file" in
  cdda.bin | cdda-mcn.nrg | svcdgs.nrg )
    good_rc=1
    ;;
  *) 
    good_rc=0
    ;;
  esac
  $check_program $file
  if test $? -ne $good_rc ; then 
    exit 1
  fi
done
exit 0

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
