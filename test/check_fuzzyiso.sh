#!/bin/sh
#$Id: check_fuzzyiso.sh,v 1.2 2005/02/09 02:50:47 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

check_program="../example/isofuzzy"

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
