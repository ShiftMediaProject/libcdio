#!/bin/sh
#$Id: check_fuzzyiso.sh,v 1.4 2005/03/03 01:04:27 rocky Exp $

if test -z $srcdir ; then
  srcdir=`pwd`
fi

if test -z $top_srcdir ; then
  top_srcdir=`pwd`/..
fi

if test -z $top_builddir ; then
  top_builddir=`pwd`/..
fi

check_program="$top_builddir/example/isofuzzy"

if test ! -x $check_program ; then
  exit 77
fi

cd $srcdir; src_dir=`pwd`
for file in $src_dir/*.bin $src_dir/*.iso $src_dir/*.nrg ; do 
  case "$file" in
  $src_dir/cdda.bin | $src_dir/cdda-mcn.nrg | $src_dir/svcdgs.nrg )
    good_rc=1
    ;;
  *) 
    good_rc=0
    ;;
  esac
  $check_program $file
  if test $? -ne $good_rc ; then 
    echo "$0: failed running:"
    echo "	$check_program $file"
    exit 1
  fi
done
exit 0

#;;; Local Variables: ***
#;;; mode:shell-script ***
#;;; eval: (sh-set-shell "bash") ***
#;;; End: ***
