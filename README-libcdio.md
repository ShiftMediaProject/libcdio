Building `libcdio` from released source
=======================================

See [README-develop.md](README-develop.md) if you plan use the git or development version.

* To compile the source, you'll need a POSIX shell and utilities (sh, sed, grep, cat), an ANSI C compiler like gcc, and a POSIX "make" program like GNU make or remake. You may also want to have "libtool" installed for building portable shared libraries.

* Uncompress and unpack the source code using for example "tar". Recent
versions of GNU tar can do this in one step like this:

```shell
tar -xpf libcdio-*.bz  # or libcdio-*.gz
```

* Go into the directory, run "configure" followed by "make":

```shell
    cd libcdio-*
    sh ./configure MAKE=make  # or remake or gmake
```

* If step 2 works, compile everything:

```shell
    make  # or remake
```

* Run the regression tests if you want:

```shell
    make check  # or remake check
```

* Install. If the preceding steps were successful:

```shell
    make install  # you may have to do this as root
                  # or "sudo make install"
```

If you have problems linking libcdio or libiso9660, see the BSD
section.  You might also try the option `--without-versioned-libs`. However
this option does help with the situation described below so it is
preferred all other things being equal.

If you are debugging libcdio, the libtool and the dynamic libraries
can make things harder. I suggest setting `CFLAGS` to include
`-fno-inline -g` and using `--disable-shared` on `configure`.

VCD dependency
---------------

One thing that confuses people is the "dependency" on libvcdinfo from
vcdimager, while vcdimager has a dependency on libcdio.  This libcdio
dependency on vcdimager is an optional (i.e. not mandatory) dependency,
while the vcdimager dependency right now is mandatory. libvcdinfo is
used only by the utility program cd-info. If you want cd-info to use
the VCD reporting portion and you don't already have vcdimager
installed, build and install libcdio, then vcdimager, then configure
libcdio again and it should find libvcdinfo.

People who make packages might consider making two packages, a libcdio
package with just the libraries (and no dependency on libvcdinfo) and
a libcdio-utils which contains cd-info and iso-info, cd-read,
iso-read. Should you want cd-info with VCD support then you'd add a
dependency in that package to libvcdinfo.

Another thing one can do is "make install" inside the library, or run
"configure --without-vcd-info --without-cddb" (since libcddb also has
an optional dependency on libcdio).

Microsoft Windows
-----------------

Building under Microsoft Windows is supported for cygwin
(<http://www.cygwin.com>). You need to have the cygwin libiconv-devel
package installed.

For MinGW (<http://www.mingw.org/>) also works provided you have
libiconv installed.

Support for Microsoft compilers (e.g. Visual C) is not officially
supported.  You need to make your own "project" files.  Don't
undertake this unless you are willing to spend time hacking. In the
past xboxmediacenter team folks I believe have gone this route, as has
Pete Batard in his rufus project. You may be able to use their project
files as a starting point.

XBOX
-----

Consult the [xboxmediacenter team](https://www.xboxmediacenter.de)

BSD, FreeBSD, NetBSD
---------------------

Unless you use `--without-versioned-libs` (not recommended), you need to
use GNU make or [remake](http://bashdb.sf.net/remake). GNU make can be
found under the name "gmake".

If you use another make you are likely to get problems linking libcdio
and libiso9660.

Solaris
-------

You may need to use --without-versioned-libs if you get a problem
building libcdio or libiso9660.

If you get a message like:
   libcdio.so: attempted multiple inclusion of file

because you have enable vcd-info and it is installed, then the only
way I know how to get around is to use configure with --disable-shared.

OS Support
-----------

Support for Operating Systems's is really based on the desire, ability
and willingness of others to help out. I use GNU/Linux so that
probably works best. Before a release I'll test on servers I have
available. I also announce a pending release on <libcdio-devel@gnu.org>
and ask others to test out.

Specific libcdio configure options and environment variables
------------------------------------------------------------

```shell
  --disable-cxx            Disable C++ bindings (default enabled)
  --enable-cpp-progs       make C++ example programs (default enabled)
  --disable-example-progs  Don't build libcdio sample programs
  --disable-dependency-tracking  speeds up one-time build
  --enable-dependency-tracking   do not reject slow dependency extractors
  --disable-largefile      omit support for large files
  --enable-shared[=PKGS]   build shared libraries [default=yes]
  --enable-static[=PKGS]   build static libraries [default=yes]
  --enable-fast-install[=PKGS]
                           optimize for fast installation [default=yes]
  --disable-libtool-lock   avoid locking (might break parallel builds)
  --disable-joliet         don't include Joliet extension support (default
                           enabled)
  --disable-rpath          do not hardcode runtime library paths
  --enable-rock            include Rock-Ridge extension support (default
                           enabled)
  --enable-cddb            include CDDB lookups in cd_info (default enabled)
  --enable-vcd-info        include Video CD Info from libvcd
  --enable-maintainer-mode create documentation and manual packages.
                           For this you need texinfo and help2man installed
```

Optional Packages:

```shell
  --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]
  --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)
  --without-cd-drive      don't build program cd-drive (default with)
  --without-cd-info       don't build program cd-info (default with)
  --without-cd-paranoia   don't build program cd-paranoia and paranoia
                          libraries (default with)
  --without-cdda-player   don't build program cdda-player (default with)
  --with-cd-paranoia-name name to use as the cd-paranoia program name (default
                          cd-paranoia)
  --without-cd-read       don't build program cd-read (default with)
  --without-iso-info      don't build program iso-info (default with)
  --without-iso-read      don't build program iso-read (default with)
  --without-versioned-libs
                          build versioned library symbols (default enabled if
                          you have GNU ld)
  --with-pic              try to use only PIC/non-PIC objects [default=use
                          both]
  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]
  --with-gnu-ld           assume the C compiler uses GNU ld default=no
  --with-libiconv-prefix[=DIR]  search for libiconv in DIR/include and DIR/lib
  --without-libiconv-prefix     don't search for libiconv in includedir and libdir
```

Some influential environment variables:

```shell
  CC          C compiler command
  CFLAGS      C compiler flags
  LDFLAGS     linker flags, e.g. `-L` __lib_dir__  if you have libraries in a
              nonstandard directory __lib dir__
  LIBS        libraries to pass to the linker, e.g. -l __library__
  CPPFLAGS    (Objective) C/C++ preprocessor flags, e.g. -I __include dir__ if
              you have headers in a nonstandard directory __include dir__
  CXX         C++ compiler command
  CXXFLAGS    C++ compiler flags
  CPP         C preprocessor
  CXXCPP      C++ preprocessor
  PKG_CONFIG  path to pkg-config utility
  CDDB_CFLAGS C compiler flags for CDDB, overriding pkg-config
  CDDB_LIBS   linker flags for CDDB, overriding pkg-config
  VCDINFO_CFLAGS
              C compiler flags for VCDINFO, overriding pkg-config
  VCDINFO_LIBS
              linker flags for VCDINFO, overriding pkg-config
```
