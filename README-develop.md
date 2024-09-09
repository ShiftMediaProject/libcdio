# Building `libcdio` from git source

Here is software you'll need to build a development version libcdio

- git
- GNU make (or even better "remake")
- autoconf version 2.67 or better
   (which contains programs autoconf and autoreconf)
- automake version 1.11.1 or better
- libtool (for building shared libraries)
- m4 (used by autoconf)
- texinfo (for building documentation)
- help2man (turns help for libcdio standalone programs into manual pages)

This is in addition to the software needed to build starting from a the
source tar. See README-libcdio.md for that additional software.

Older versions of autoconf and automake might work, I've just not
tested that.

The source code lives the [github](https://github.com/libcdio/libcdio.git).
The older GNU Savannah main page is [here](https://savannah.gnu.org/projects/libcdio/).

If you check out the source code, you'll need `git` installed.

Once you have git:

    git clone https://github.com/libcdio/libcdio.git

Change into the libcdio directory that just created and run the "autogen.sh"
shell script:

    cd libcdio
    sh ./autogen.sh

Please see [README-libcdio.md](README-libcdio.md) and follow those instructions starting at step 3.
