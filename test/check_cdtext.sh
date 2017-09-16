#!/bin/sh
#   Copyright (C) 2016 Leon Merten Lohse <leon@green-side.de>
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
#

if test "X$srcdir" = "X" ; then
  exit $SKIP_TEST_EXITCODE
fi

. $srcdir/check_common_fn

CDTEXT_RAW=$srcdir/../example/cdtext-raw

if test ! -x $CDTEXT_RAW ; then
  exit $SKIP_TEST_EXITCODE
fi

if test "X$DIFF" = "X" ; then
  exit $SKIP_TEST_EXITCODE
fi

if test "$DIFF" = "no"; then
  echo "$0: No diff(1) or cmp(1) found - cannot test ${cmdname}"
  exit $SKIP_TEST_EXITCODE
fi


test_cdtext_raw() {
  opts="$1"
  outfile="$2"
  rightfile="$3"

  if "${CDTEXT_RAW}" ${opts} >"${outfile}" 2>&1 ; then
    if $DIFF $DIFF_OPTS "${outfile}" "${rightfile}" ; then
      $RM "${outfile}"
      return 0
    else
      return 3
    fi
  else
    echo "$0 failed running: cdtext-raw ${opts}"
    return 2
  fi
}

fname=cdtext
opts="${srcdir}/data/${fname}.cdt"
test_cdtext_raw "$opts" ${srcdir}/${fname}.dump ${srcdir}/${fname}.right
RC=$?
check_result $RC "CD-Text parser" "${CDTEXT_RAW} $opts"

fname=cdtext-libburnia
opts="${srcdir}/data/${fname}.cdt"
test_cdtext_raw "$opts" ${srcdir}/${fname}.dump ${srcdir}/${fname}.right
RC=$?
check_result $RC "CD-Text libburnia roundtrip" "${CDTEXT_RAW} $opts"

exit 0
