<!-- markdown-toc start - Don't edit this section. Run M-x markdown-toc-refresh-toc -->

Table of Contents
=================

- [Let people know of a pending release](#let-people-know-of-a-pending-release)
- [Coordinate with libcdio-paranoia release](#coordinate-with-libcdio-paranoia-release)
- [Test](#test)
- [Go over version number, Update Release notes](#go-over-version-number-update-release-notes)
- [Update Documention](#update-documention)
- [Are sources up to date?](#are-sources-up-to-date)
- [Bump release](#bump-release)
- [Push and tag release](#push-and-tag-release)
- [Upload to ftp.gnu.org](#upload-to-ftpgnuorg)
- [Update documentation](#update-documentation)
    - [copy libcdio manual to web page](#copy-libcdio-manual-to-web-page)
- [Bump to git version](#bump-to-git-version)
- [Let libcdio-devel know of release](#let-libcdio-devel-know-of-release)

<!-- markdown-toc end -->
Let people know of a pending release
====================================

Let people know on the [libcdio-devel mailing list](mailto://libcdio-devel@gnu.org).

No major changes before release, please.

Coordinate with libcdio-paranoia release
=========================================

... to be filled in


Test
====

* Test on lots of platforms; gcc compile farm, for example.
* `make distcheck` should work.
* Look for/fix/apply [patches](https://savannah.gnu.org/patch/?group=libcdio) and [outstanding bugs](https://savannah.gnu.org/bugs/?group=libcdio) on Savannah GNU.

Go over version number, Update Release notes
=============================================

```
$ make ChangeLog`
```

Go over `Changelog` and add to `NEWS`. Update date of release.

Remove "git" from `configure.ac`'s release name. E.g.

```
    define(RELEASE_NUM, 2.1.0)
    define(LIBCDIO_VERSION_STR, $11git)
                                   ^^^
    ...
```

Update Documention
==================

Make doxygen documentation:


```
  $ (cd doc/doxygen; doxygen -u; ./run_doxygen)
```

remove any errors.

Are sources up to date?
=======================

Make sure sources are current and checked in:

```
    $ git pull
    $ ./autogen.sh && make && make check
```

Bump release
============

The whole semantic versioning thing should have been decided on in advance


```
  $ export LIBCDIO_VERSION=2.0.0 # adjust
  $ git commit -m"Get ready for release ${LIBCDIO_VERSION}" .
```

Push and tag release
====================

```
  $ make ChangeLog
```

Tag release in git:

```
  $ git push
  $ git tag release-${LIBCDIO_VERSION}
  $ git push --tags
```

`make distcheck` one more time.


Upload to ftp.gnu.org
=====================

Get onto http://ftp.gnu.org.


Use `gnupload` from the `automake` distribution.

```
$ locate gnupload
$ /src/external-vcs/coreutils/build-aux/gnupload --to ftp.gnu.org:libcdio libcdio-${LIBCDIO_VERSION}.tar.*  # (Use "is" password)
```

Update documentation
====================

Copy doxygen and html to web pages:

```
  $ cd ../libcdio-www/doxygen
  $ rm *.html
  $ cp ../../libcdio/doc/doxygen/html/*.html .
  $ cvs update .
```

For each "U" html *except libcdio.html* that is put back, remove it with `rm` and then `cvs remove`. For example, put in file `/tmp/libcdio-remove.txt`.

```
$ rm `cat /tmp/libcdio-remove.txt`
$ cvs remove `cat /tmp/libcdio-remove.txt`
```

For each new "?" html add it. For example, put in file `/tmp/libcdio-new.txt` and run:

```
$ cvs add `cat /tmp/libcdio-new.txt`
$ cvs -m"Update for $LIBCDIO_VERSION release" commit .
```

copy libcdio manual to web page
--------------------------------

```
     cd libcdio-www
     (cd ../libcdio/doc && make libcdio.html)
     cp ../libcdio/doc/libcdio.html .
     cvs commit -m"Update for $LIBCDIO_VERSION release" libcdio.html
```

Bump to git version
===================

Bump version in `configure.ac` and add "git". See place above in removal.

Let libcdio-devel know of release
=================================

Again: [libcdio-devel mailing list](mailto://libcdio-devel@gnu.org).
