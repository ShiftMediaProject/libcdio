AC_REVISION([$Id: configure.ac,v 1.8 2003/04/14 10:03:27 rocky Exp $])dnl
AC_INIT(lib/cdio.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(libcdio, 0.2)
     
AM_SANITY_CHECK

AC_ARG_WITH(cdinfo,
[  --without-cdinfo	build sample program cdinfo (enabled by default)],
enable_cdinfo="${withval}", enable_cdinfo=yes)

dnl Checks for programs.
AC_PROG_CC

if test "x$GCC" != "xyes" 
then
  echo "*** non GNU CC compiler detected."
  echo "*** This package has not been tested very well with non GNU compilers"
  echo "*** you should try to use 'gcc' for compiling this package."
else
  WARN_CFLAGS="-Wall -Wchar-subscripts -Wmissing-prototypes -Wmissing-declarations -Wunused -Wpointer-arith -Wwrite-strings -Wnested-externs -Wsign-promo -Wno-sign-compare"

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

dnl We use a perl for documentation and regression tests
AC_PATH_PROG(PERL, perl, no)
AC_SUBST(PERL)dnl

AM_PATH_LIBPOPT(, [enable_cdinfo=no])

dnl headers

AC_STDC_HEADERS
AC_CHECK_HEADERS(stdint.h inttypes.h stdbool.h)

AC_CHECK_HEADER(cddb/cddb.h, , 
           [ AC_MSG_RESULT([*** CDDB support be disabled from cdinfo program])
           enable_cddb=no ], 
	        )	   

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

dnl system
AC_CYGWIN

LIBS="$LIBS -lm"
CFLAGS="$CFLAGS $WARN_CFLAGS"

AM_CONDITIONAL(CYGWIN, test "x$CYGWIN" = "xyes")
AM_CONDITIONAL(BUILD_CDINFO, test "x$enable_cdinfo" = "xyes")
AM_CONDITIONAL(BUILD_CDINFO_LINUX, test "x$enable_cdinfo_linux" = "xyes")
AM_CONDITIONAL(BUILD_CDIOTEST, test "x$enable_cdiotest" = "xyes")

AM_PROG_LIBTOOL
     
dnl Checks for header files.
AC_STDC_HEADERS
     
AC_HAVE_HEADERS( stdbool.h  stdlib.h stdint.h stdio.h string.h strings.h linux/version.h sys/cdio.h sys/stat.h sys/types.h )

LIBCDIO_CFLAGS='-I$(top_srcdir)/lib/ -I$(top_srcdir)/include/'
LIBCDIO_LIBS='$(top_builddir)/lib/libcdio.la'
AC_SUBST(LIBCDIO_CFLAGS)
AC_SUBST(LIBCDIO_LIBS)

ACLOCAL_AMFLAGS='-I .'
AC_SUBST(ACLOCAL_AMFLAGS)

case $host_os in
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
	fi
	;;
     bsdi*)
        AC_CHECK_HEADERS(dvd.h, [have_bsdi_dvd_h="yes"])
	if test "x$have_bsdi_dvd_h" = "xyes" ; then
     	  AC_DEFINE([HAVE_BSDI_CDROM], [1], 
                    [Define 1 if you have BSDI-type CD-ROM support])
	  LIBS="$LIBS -ldvd"
	fi
	;;
     sunos*|sun*|solaris*)
        AC_CHECK_HEADERS(sys/cdio.h)
     	AC_DEFINE([HAVE_SOLARIS_CDROM], [1], 
                  [Define 1 if you have Solaris CD-ROM support])
	;;
     *)
	AC_MSG_WARN(Don't have OS CD-reading support for ${host_os}...)
	AC_MSG_WARN(Will use generic support.)
	;;
esac
AC_SUBST(LINUX_CDROM_TIMEOUT)
AC_SUBST(HAVE_LINUX_CDROM)
AC_SUBST(HAVE_BSDI_CDROM)
AC_SUBST(HAVE_SOLARIS_CDROM)

if test x$enable_cddb = x; then
AC_ARG_ENABLE(cddb,
	[  --disable-cddb          don't include CDDB lookups in cdinfo],
		enable_cddb=no,
		enable_cddb=yes)
fi
if test x$enable_cddb = xyes; then
    AC_CHECK_LIB(cddb, cddb_disc_calc_discid,
                 [ CDDB_LIB="-lcddb"; ac_have_cddb=yes ])
    AC_DEFINE([HAVE_CDDB],1,
	      [Define this if you have libcddb installed])
fi


AC_SUBST(CDDB_LIB)

AC_OUTPUT([ \
       Makefile \
       lib/Makefile \
       src/Makefile \
       tests/check_opts.sh \
       tests/Makefile \
       ])

chmod +x tests/check_opts.sh
