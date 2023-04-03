#!/bin/sh
#   Copyright (C) 2020 Pete Batard <pete@akeo.ie>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# Tests multiextents (via iso-info and iso-read). 

if test -z "$srcdir" ; then
  srcdir=`pwd`
fi

if test "X$top_builddir" = "X" ; then
  top_builddir=`pwd`/..
fi

. ${top_builddir}/test/check_common_fn

if test ! -x ../src/iso-info ; then
  exit 77
fi

if test ! -x ../src/iso-read ; then
  exit 77
fi

BASE=`basename $0 .sh`

for i in multi_extent_8k.iso \
         multi_extent_8k_emul_toc_padded.iso \
         multi_extent_8k_joliet.iso 
do
  if test "$i" = multi_extent_8k.iso
  then
    fname=multiextent
    aspect="ISO9660+RockRidge"
  elif test "$i" = multi_extent_8k_emul_toc_padded.iso
  then
    fname=multiextent_emultocpad
    aspect="ISO9660+RockRidge+EmulTOC+Padding"
  else
    fname=multiextent_joliet
    aspect="Joliet"
  fi

  # File listing
  iso_image="${srcdir}/data/${i}"
  opts="--no-header --quiet -l ${iso_image}"
  test_iso_info "$opts" ${fname}.dump ${srcdir}/${fname}.right
  RC=$?
  check_result $RC "$aspect"' multiextent listing test' "$ISO_INFO $opts"

  # File dump and comparison
  opts="--ignore --image ${iso_image} --extract multi_extent_file"
  test_iso_read "$opts" ${fname} ${srcdir}/data/multi_extent_file
  RC=$?
  check_result $RC "$aspect"' multiextent read test' "$ISO_READ $opts"
done

exit $RC
