dnl Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
dnl
dnl This program is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 2, or (at your option)
dnl any later version.
dnl
dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl
dnl You should have received a copy of the GNU General Public License
dnl along with this program; if not, write to the Free Software
dnl Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
dnl 02111-1307, USA.

define(RELEASE_NUM, 71)
define(CDIO_VERSION_STR, 0.$1cvs)

AC_PREREQ(2.52)
AC_REVISION([$Id: configure.ac,v 1.105 2004/10/31 03:10:25 rocky Exp $])dnl
AC_INIT(libcdio, CDIO_VERSION_STR(RELEASE_NUM))
AC_CONFIG_SRCDIR(src/cd-info.c)
AM_INIT_AUTOMAKE

AC_CANONICAL_HOST
AM_CONFIG_HEADER(config.h)

LIBCDIO_VERSION_NUM=RELEASE_NUM
AC_SUBST(LIBCDIO_VERSION_NUM)

AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)
AM_MISSING_PROG(CVS2CL, cvs2cl, $missing_dir)
AM_MAINTAINER_MODE

AM_SANITY_CHECK

AC_ARG_WITH(cd_info,
[  --without-cd-info	build program cd-info (enabled by default)],
enable_cd_info="${withval}", enable_cd_info=yes)

AC_ARG_WITH(cd_read,
[  --without-cd-read	build program cd-read (enabled by default)],
enable_cd_read="${withval}", enable_cd_read=yes)

AC_ARG_WITH(versioned_libs,
[  --without-versioned-libs build versioned library symbols (enabled by default if you have GNU ld)],
enable_versioned_libs="${withval}", enable_versioned_libs=yes)

AC_ARG_WITH(cd_info,
[  --without-cd-info	build program cd-info (enabled by default)],
enable_cd_info="${withval}", enable_cd_info=yes)



dnl Checks for programs.
AC_PROG_CC

cd_drivers='cdrdao, BIN/CUE, NRG'

if test "x$GCC" != "xyes" 
then
  echo "*** non GNU CC compiler detected."
  echo "*** This package has not been tested very well with non GNU compilers"
  echo "*** you should try to use 'gcc' for compiling this package."
else
  WARN_CFLAGS="-Wall -Wchar-subscripts -Wmissing-prototypes -Wmissing-declarations -Wunused -Wpointer-arith -Wwrite-strings -Wnested-externs -Wno-sign-compare"

  for WOPT in $WARN_CFLAGS; do
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $WOPT"
    AC_MSG_CHECKING([whether $CC understands $WOPT])
    AC_TRY_COMPILE([], [], has_option=yes, has_option=no,)
    CFLAGS="$SAVE_CFLAGS"
    AC_MSG_RESULT($has_option)
    if test $has_option = yes; then
      warning_flags="$warning_flags $option"
    fi
    unset has_option
    unset SAVE_CFLAGS
  done
  WARNING_FLAGS="$warning_flags"
  unset warning_flags
fi

# We use Perl for documentation and regression tests
AC_PATH_PROG(PERL, perl, no)
AC_SUBST(PERL)

# We use a diff in regression testing
AC_PATH_PROG(DIFF, diff, no)
DIFF_OPTS=

if test "$DIFF" = no ; then
   AC_PATH_PROG(DIFF, cmp, no)
else 
  # Try for GNU diff options.
  # MSDOG output uses \r\n rather than \n in tests
  for diff_opt in -w --unified ; do 
    if $DIFF $diff_opt ./configure ./configure > /dev/null 2>&1; then
      AC_MSG_RESULT([adding $diff_opt to diff in regression tests])
      DIFF_OPTS="$DIFF_OPTS $diff_opt"
    fi
  done
fi
AC_SUBST(DIFF)
AC_SUBST(DIFF_OPTS)

AM_PATH_LIBPOPT(, 
  [AC_MSG_WARN([Diagnostic programs cd-drive, cd-info, cd-read, iso-info, and is-read will not get built.])
  		  enable_cd_info=no])

dnl headers

AC_HEADER_STDC
AC_CHECK_HEADERS(glob.h stdbool.h langinfo.h)

dnl compiler
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE

dnl ISOC99_PRAGMA
AC_MSG_CHECKING([whether $CC supports ISOC99 _Pragma()])
AC_TRY_COMPILE([],[_Pragma("pack(1)")], [
  ISOC99_PRAGMA=yes 
  AC_DEFINE(HAVE_ISOC99_PRAGMA, [], [Supports ISO _Pragma() macro])
],ISOC99_PRAGMA=no)
AC_MSG_RESULT($ISOC99_PRAGMA)

dnl empty_array_size
AC_MSG_CHECKING([how to create empty arrays])

empty_array_size="xxx"
AC_TRY_COMPILE([],[struct { int foo; int bar[]; } doo;], empty_array_size="")

if test "x$empty_array_size" = "xxxx";then
 AC_TRY_COMPILE([],[struct { int foo; int bar[0]; } doo;], empty_array_size="0")
fi

if test "x$empty_array_size" = "xxxx"
then
  AC_MSG_ERROR([compiler is unable to creaty empty arrays])
else
  AC_DEFINE_UNQUOTED(EMPTY_ARRAY_SIZE, $empty_array_size, 
	[what to put between the brackets for empty arrays])
  changequote(`,')
  msg="[${empty_array_size}]"
  changequote([,])
  AC_MSG_RESULT($msg)
fi
dnl empty_array_size

dnl bitfield order
AC_MSG_CHECKING(bitfield ordering in structs)
AC_TRY_RUN([
int
 main() {
  struct { char bit_0:1, bit_12:2, bit_345:3, bit_67:2; }
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  __attribute__((packed))
#endif
  bf = { 1,1,1,1 };
  if (sizeof (bf) != 1) return 1;
  return *((unsigned char*) &bf) != 0x4b; }
], bf_lsbf=1, AC_TRY_RUN([ 
int
main() {
  struct { char bit_0:1, bit_12:2, bit_345:3, bit_67:2; }
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  __attribute__((packed))
#endif
  bf = { 1,1,1,1 };
  if (sizeof (bf) != 1) return 1;
  return *((unsigned char*) &bf) != 0xa5; }
], bf_lsbf=0, AC_MSG_ERROR([unsupported bitfield ordering])))
if test "x$bf_lsbf" = "x1"; then
 AC_MSG_RESULT(LSBF)
 AC_DEFINE(BITFIELD_LSBF, [], [compiler does lsbf in struct bitfields])
else
 AC_MSG_RESULT(MSBF)
fi
dnl

dnl AM_PROG_LIBTOOL tests whether we have GNU ld
dnl this must come before checking --with-versioned-libs
dnl which requires GNU ld.
AM_PROG_LIBTOOL
     
dnl system

LIBS="$LIBS -lm"
CFLAGS="$CFLAGS $WARN_CFLAGS"

# We need either GNU make or --without-versioned-libs
if test "x$enable_versioned_libs" = "xyes" ; then 
  if test -n "$MAKE" ; then
    $MAKE --version 2>/dev/null >/dev/null
    if test "$?" -ne 0 ; then
       AC_MSG_ERROR(Either set MAKE variable to GNU make or use 
   --without-versioned-libs option)
    fi
  else 
    make --version 2>/dev/null >/dev/null
    if test "$?" -ne 0 ; then
       AC_MSG_ERROR(Either set MAKE variable to GNU make or use 
   --without-versioned-libs option)
    else 
      # Do we have GNU ld? We need that too.
      if test "$with_gnu_ld" != yes; then
        AC_MSG_WARN([I don't see GNU ld. I'm going to assume --without-versioned-libs])
	enable_versioned_libs='no'
      fi
    fi
  fi
fi

AM_CONDITIONAL(CYGWIN, test "x$CYGWIN" = "xyes")
AM_CONDITIONAL(BUILD_CDINFO, test "x$enable_cd_info" = "xyes")
AM_CONDITIONAL(BUILD_CDINFO_LINUX, test "x$enable_cd_info_linux" = "xyes")
AM_CONDITIONAL(BUILD_CDIOTEST, test "x$enable_cdiotest" = "xyes")
AM_CONDITIONAL(BUILD_VERSIONED_LIBS, test "x$enable_versioned_libs" = "xyes")

dnl Checks for header files.
AC_STDC_HEADERS
     
AC_HAVE_HEADERS( errno.h fcntl.h stdio.h linux/version.h )

LIBCDIO_CFLAGS='-I$(top_srcdir)/lib/ -I$(top_srcdir)/include/'
LIBCDIO_LIBS='$(top_builddir)/lib/libcdio.la'
LIBISO9660_LIBS='$(top_builddir)/lib/libiso9660.la'
AC_SUBST(LIBCDIO_CFLAGS)
AC_SUBST(LIBCDIO_LIBS)
AC_SUBST(LIBISO9660_LIBS)

case $host_os in
     darwin6*|darwin7*)
       AC_CHECK_HEADERS(IOKit/IOKitLib.h CoreFoundation/CFBase.h, 
                        [have_iokit_h="yes"])
       if test "x$have_iokit_h" = "xyes" ; then 
         AC_DEFINE([HAVE_DARWIN_CDROM], [1],
	      [Define 1 if you have Darwin OS X-type CD-ROM support])
         LIBCDIO_LIBS="$LIBCDIO_LIBS  -Wl,-framework -Wl,CoreFoundation -Wl,-framework -Wl,IOKit"
	 DARWIN_PKG_LIB_HACK=" -Wl,-framework -Wl,CoreFoundation \"-Wl,-framework -Wl,IOKit\""
         cd_drivers="${cd_drivers}, Darwin"
       fi
     ;;
     linux*)
        AC_CHECK_HEADERS(linux/version.h)
        AC_CHECK_HEADERS(linux/cdrom.h, [have_linux_cdrom_h="yes"])
	if test "x$have_linux_cdrom_h" = "xyes" ; then
	   AC_TRY_COMPILE(,[
#include <linux/cdrom.h>
struct cdrom_generic_command test;
int has_timeout=sizeof(test.timeout);],
            [AC_DEFINE([HAVE_LINUX_CDROM_TIMEOUT], [1], 
                   [Define 1 if timeout is in cdrom_generic_command struct])])
	   AC_DEFINE([HAVE_LINUX_CDROM], [1], 
                     [Define 1 if you have Linux-type CD-ROM support])
	   cd_drivers="${cd_drivers}, GNU/Linux"
	fi
	;;
     bsdi*)
        AC_CHECK_HEADERS(dvd.h, [have_bsdi_dvd_h="yes"])
	if test "x$have_bsdi_dvd_h" = "xyes" ; then
     	  AC_DEFINE([HAVE_BSDI_CDROM], [1], 
                    [Define 1 if you have BSDI-type CD-ROM support])
	  LIBS="$LIBS -ldvd"
	  cd_drivers="${cd_drivers}, BSDI"
	fi
	;;
     sunos*|sun*|solaris*)
        AC_CHECK_HEADERS(sys/cdio.h)
     	AC_DEFINE([HAVE_SOLARIS_CDROM], [1], 
                  [Define 1 if you have Solaris CD-ROM support])
	  cd_drivers="${cd_drivers}, Solaris"
	;;
     cygwin*)
	AC_DEFINE([CYGWIN], [1], 
                  [Define 1 if you are compiling using cygwin])
     	AC_DEFINE([HAVE_WIN32_CDROM], [1], 
                  [Define 1 if you have MinGW CD-ROM support])
 	LIBS="$LIBS -lwinmm"
  	cd_drivers="${cd_drivers}, MinGW"
	;;
     mingw*)
	AC_DEFINE([MINGW32], [1], 
                  [Define 1 if you are compiling using MinGW])
     	AC_DEFINE([HAVE_WIN32_CDROM], [1], 
                  [Define 1 if you have MinGW CD-ROM support])
 	LIBS="$LIBS -lwinmm -mwindows"
  	cd_drivers="${cd_drivers}, MinGW "
	;;
     freebsd4.*|freebsd5.*)
     	AC_DEFINE([HAVE_FREEBSD_CDROM], [1], 
                  [Define 1 if you have FreeBSD CD-ROM support])
 	LIBS="$LIBS -lcam"
  	cd_drivers="${cd_drivers}, FreeBSD "
	;;
     *)
	AC_MSG_WARN([Don't have OS CD-reading support for ${host_os}...])
	AC_MSG_WARN([Will use generic support.])
	;;
esac
AC_SUBST(LINUX_CDROM_TIMEOUT)
AC_SUBST(DARWIN_PKG_LIB_HACK)
AC_SUBST(HAVE_BSDI_CDROM)
AC_SUBST(HAVE_DARWIN_CDROM)
AC_SUBST(HAVE_FREEBSD_CDROM)
AC_SUBST(HAVE_LINUX_CDROM)
AC_SUBST(HAVE_SOLARIS_CDROM)
AC_SUBST(HAVE_WIN32_CDROM)

LIBCDIO_SOURCE_PATH="`pwd`"
AC_DEFINE_UNQUOTED(LICDIO_SOURCE_PATH, "$LIBCDIO_SOURCE_PATH",
	[Full path to libcdio top_sourcedir.])
AC_SUBST(LIBCDIO_SOURCE_PATH)

AC_CHECK_FUNCS( memset bzero memcpy tzset nl_langinfo )

AC_CHECK_MEMBER([struct tm.tm_gmtoff],
                [AC_DEFINE(HAVE_TM_GMTOFF, 1,
                        [Define if struct tm has the tm_gmtoff member.])],
                 ,
                 [#include <time.h>])

if test $ac_cv_member_struct_tm_tm_gmtoff = yes ; then
  AC_MSG_CHECKING([whether time.h defines daylight and timezone variables])
  AC_TRY_RUN([
#include <time.h>


       extern char *tzname[2];
       extern long timezone;
       extern int daylight;

int
 main() {
  return (timezone != 0) + daylight;
}
  ],[AC_DEFINE(HAVE_DAYLIGHT, 1,
   [Define if time.h defines extern long timezone and int daylight vars.])
   has_daylight=yes
   ],[has_daylight=no])
  AC_MSG_RESULT($has_daylight)
  AC_MSG_CHECKING([whether time.h defines tzname variable])
  AC_TRY_RUN([
#include <time.h>


       extern char *tzname[2];

int
 main() {
  return (tzname != NULL);
}
  ],[AC_DEFINE(HAVE_TZNAME, 1,
   [Define if time.h defines extern extern char *tzname[2] variable])
   has_tzname=yes
   ],[has_tzname=no])
   AC_MSG_RESULT($has_tzname)
fi

if test x$enable_joliet = x; then
AC_ARG_ENABLE(joliet,
	[  --disable-joliet          don't include Joliet extension support],
		enable_joliet=no,
		enable_joliet=yes)
fi
if test x$enable_joliet = xyes; then
  AM_ICONV
  if test "$am_cv_func_iconv" = yes ; then 
     AC_DEFINE(HAVE_JOLIET, [1], 
     [Define 1 if you want ISO-9660 Joliet extension support. 
     You must have also libiconv installed to get Joliet extension support.])
  else
     AC_MSG_WARN([You must have iconv installed to get Joliet extension support.])
  fi
fi

if test x$enable_cddb = x; then
AC_ARG_ENABLE(cddb,
	[  --disable-cddb          don't include CDDB lookups in cd_info],
		enable_cddb=no,
		enable_cddb=yes)
fi
if test x$enable_cddb = xyes; then
   PKG_CHECK_MODULES(CDDB, libcddb >= 0.9.4, [
    HAVE_CDDB=yes 
    AC_DEFINE(HAVE_CDDB, [], [Define this if you have libcddb installed])
    ],
    [AC_MSG_WARN([new enough libcddb not found. CDDB access disabled. Get libcddb from http://libcddb.sourceforge.net])
    HAVE_CDDB=no])
    AC_CHECK_LIB(socket, connect)
    AC_CHECK_FUNC(gethostbyname, , AC_CHECK_LIB(nsl, gethostbyname))
fi


AC_SUBST(CDDB_LIBS)

if test x$enable_vcd_info = x; then
AC_ARG_ENABLE(vcd_info,
	[  --disable-vcd-info        don't include Video CD Info from libvcd],
		enable_vcd_info=no,
		enable_vcd_info=yes)
fi
if test x$enable_vcd_info = xyes; then
  PKG_CHECK_MODULES(VCDINFO, libvcdinfo >= 0.7.20, 
  [AC_DEFINE([HAVE_VCDINFO],1, 
  [Define this if you have libvcdinfo installed])], 
  [AC_MSG_WARN(a new enough libvcdinfo not found. 
VCD info display in cd-info disabled.
libvcdinfo is part of vcdimager. Get it from http://vcdimager.org)
   enable_vcd_info=no])
fi

AC_SUBST(VCDINFO_LIBS)
AC_SUBST(VCDINFO_CFLAGS)

AC_CONFIG_COMMANDS([checks], 
		   [chmod +x test/check_cue.sh; chmod +x test/check_nrg.sh
		   ])

AC_CONFIG_FILES([ \
       Makefile \
       libcdio.pc \
       libcdio.spec \
       libiso9660.pc \
       example/Makefile \
       include/Makefile \
       include/cdio/Makefile \
       include/cdio/version.h \
       doc/doxygen/Doxyfile \
       doc/Makefile \
       lib/Makefile \
       src/Makefile \
       test/check_nrg.sh \
       test/check_cue.sh \
       test/check_iso.sh \
       test/check_common_fn \
       test/Makefile \
       ])

AC_CONFIG_COMMANDS([default],[[
# Make sure scripts are executable
chmod +x test/check_nrg.sh
chmod +x test/check_cue.sh
chmod +x test/check_iso.sh
]],[[]])

AC_OUTPUT

echo "Using CD-ROM drivers: $cd_drivers"
