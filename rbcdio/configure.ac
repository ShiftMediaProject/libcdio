# Copyright (C) 2006 Rocky Bernstein <rocky@panix.com>

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

## Note: the version number (the 2nd parameter in AC_INIT) 
## is picked up inside the Python debugger script.

AC_INIT([rubycdio],[0.01],[libcdio-rubycdio-devel@gnu.org])

##
## Check for Python interpreter and make we have a recent version.
##
AC_REVISION([$Id: configure.ac,v 1.1 2006/11/13 05:12:41 rocky Exp $])dnl

##
## In maintainer mode we produce a ChangeLog
##
AM_MAINTAINER_MODE

##
## make sure we are using a recent autoconf version
##
AC_PREREQ(2.53)

AC_ARG_PROGRAM
AC_PROG_CC
AM_INIT_AUTOMAKE

#
# There are a number of compiler warnings that gcc produces via SWIG code.
# Turn off warnings that are to be expected.
wall=''
AC_TRY_CFLAGS("-Wno-strict-aliasing", cflgs="-Wno-strict-aliasing",)
AC_TRY_CFLAGS("-Wno-unused-function", cflgs="$cflgs -Wno-unused-function",)
CFLAGS="$CFLAGS $cflgs"

##
## cvs2cl is used to create the ChangeLog when we create a distribution
##
AM_MISSING_PROG(CVS2CL, cvs2cl, $missing_dir)

##
## SWIG is essential - test for it.
##
AC_PATH_PROG(SWIG, swig, no)
if test "$SWIG" = no ; then
   AC_MSG_WARN([Can't find SWIG installed; using C wrapper from distributor.])
   AC_MSG_WARN([Check http://www.swig.org if you want to install SWIG])
fi

PKG_CHECK_MODULES(libcdio, libcdio >= 0.76, [], 
	[AC_MSG_WARN([Required libcdio library, version 0.76 or greater, not found.])
	AC_MSG_ERROR([Please get libcdio from http://www.gnu.org/software/libcdio/ and install it.])])

PKG_CHECK_MODULES(libiso9660, libiso9660 >= 0.76, [], 
	[AC_MSG_WARN([Required libiso9660 library, version 0.76 greater not found.])
	AC_MSG_ERROR([Please get libcdio from http://www.gnu.org/software/libcdio/ and install it.])])


##
## A symbolic links is used to link a name in users PATH to the python
## script.
## 
AC_PROG_LN_S

##
## Automake doesn't get Python's install path correct. So we 
## figure it out and set it ourself.
##
AC_ARG_WITH(site-packages,
           AC_HELP_STRING([--with-site-packages], 
	                  [place to install module]), 
	   PY_PKGDIR)

AC_ARG_WITH(ruby,
           AC_HELP_STRING([--with-ruby], 
	                  [set path to Ruby]), 
	   RUBY=$withval,[RUBY=`ruby -e 'require "mkmf"; puts $ruby'`])

AC_SUBST(RUBY)dnl

RUBY_INC=`$RUBY -e 'puts "-I " + $:.join(" -I ")'`
AC_SUBST(RUBY_INC)dnl

AC_CANONICAL_HOST
case $host_os in
  cygwin* | mingw* | pw32*)
    SO='.dll'
    CFLAGS="$CFLAGS -DUSE_DL_IMPORT"
     # Should we check the 32-ness?
    SWIG_FLAGS=-DNEED_LONG
    if test "$GCC" = yes; then
      LDSHARED="$CC -shared -Wl,--enable-auto-image-base"
    else
      if test "cl" = $CC ;  then
        # Microsoft Visual C++ (MSVC)
        LDSHARED="$CC -nologo -LD"
      else
        # Unknown compiler try gcc approach
        LDSHARED="$CC -shared"
      fi
    fi
    ;;
  linux*) 
    SO=.so
    LDSHARED="gcc -shared"
    ;;
  darwin6*|darwin7*|darwin8*)
    SO=.so
    LDSHARED="cc -bundle -undefined suppress -flat_namespace"
    ;;
  solaris*)
    SO=.so
    LDSHARED="gcc -shared"
    AC_TRY_CFLAGS("-fPIC", cflgs="$cflgs -fPIC",)
    CFLAGS="$CFLAGS $cflgs"
    ;;
  *)
    AC_TRY_CFLAGS("-fPIC", cflgs="$cflgs -fPIC",)
    CFLAGS="$CFLAGS $cflgs"
    SO='.so'
    PYTHON_LIBS=
esac
AC_SUBST(SO)
AC_SUBST(RUBY_LIBS)
AC_SUBST(LDSHARED)
AC_SUBST(SWIG_FLAGS)

##
## Test program needs location of pycdio shared library
## This is a hack, but I at present I'm not sure what the best thing to
## do is.
RUBYCDIO_LIBDIR=`pwd`
AC_SUBST(RUBYCDIO_LIBDIR)dnl

##
## Produced derived files.
##
AC_CONFIG_FILES([Makefile])

AC_OUTPUT

