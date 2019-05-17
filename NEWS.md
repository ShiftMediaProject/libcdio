version 2.1.0
=============
2019-04-17 Holy Wednesday

Most (all?) of the improvements and bug fixes are thanks to Thomas Schmitt and Edd Barrett. This release introduces an OpenBSD driver, courtesy of Edd Barrett, based on the NetBSD driver.

- NetBSD driver fixes. Switch from MSF addressing to LBA addressing in driver. (Edd Barrett and Jasper Lievisse Adriaanse)
- Fix handling in drivers,libcdio-paranoia and `cd-info` when starting track number is greater than 1. (Edd Barrett and Thomas Schmitt)
- More tolerant of non-compliant ISOs such as openSUSE Leap 15.0.
- `cdda-player` fixes, such as adding a missing `-d` in cdda-player help (Thomas Schmitt and Edd Barrett)
- `cdtext_list_languages_v2()` to be preferred over now deprecated `cdtext_list_languages()`. New API call `cdtext_set_language_index()`. (Thomas Schmitt)
- Add `read-disc-struct` example program to show how to use a MMC `DVD_READ_DISC_STRUCTURE`. (Thomas Schmitt)
- Various errors in driver reading turned into warnings. (Edd Barrett and Thomas Schmitt)
- Some code cleanups and documentation improvements.
- Bugs:
 * [Bug 53170: I/O leak when we can't read ISO file ](https://savannah.gnu.org/bugs/index.php?53170)
 * [Bug 53928: Cdtext not displayed in case of `CDTEXT_LANGUAGE_UNKNOWN`](https://savannah.gnu.org/bugs/index.php?3929)
 * [Bug 53929: cd-text with invalid characters failing to convert to utf8](https://savannah.gnu.org/bugs/index.php?3928)

version 2.0.0
=============
2017-12-31

This release bumps library version numbers and bumps the major release number. We should have gone from 1.0.0 to 2.0.0 in the last release
since there is an API incompatibility.

In addition...

- Add NetBSD drive detection; correct drive detection in `cd-info.c`
  Patches from Onno van der Linden
- Fix some MinGW and Windows portability issues
- Remove some memory leaks in some tests
- Lint (a little) with clang static analyzer

There are some programs and bindings that will need to be updated
if you want to use them with this library. Specifically:

- `Device::Cdio` (2.0.0 or greater)
- `vcdimager` (2.0.0 or greater)
- `pycdio` (2.0.0 or greater)
- `rbcdio` (2.0.0 or greater)

version 1.1.0
=============
2017-12-10  Dr. Gecko

Caveats:
`pycdio` and `Device::Perl` will be broken but that'll be fixed later

- Remove many remaining memory leaks, invalid reads, writes (as per valgrind) in library, test and demo code
- Types `CdioISO9660DirList_t` and `CdioISO9660FileList_t`, `iso9660_{dir,file}list_{new,free}` have been added.
- `cdio_list_free()` now takes an additional parameter: a function to
  free list items. This is not compatible with 1.0.0.

More work is needed on MacOS and other OS's where I don't have
`valgrind` accessible.

AIX is left untouched - that is probably heading for removal in the future.


version 1.0.0
=============
2017-11-21  Thanksgiving

This is an API breaking change

- Remove deprecated items:
   * OS/2 driver (never really was supported)
   * BSDI driver remnants
   * mmc_isrc_track_read_subchannel
   * CDIO_MIN_DRIVER, CDIO_MIN_DEVICE_DRIVER, CDIO_MAX_DRIVER, CDIO_MAX_DEVICE_DRIVER
   * CdioList, CdioListNode
- Apple Darwin OS X -> macOS
- Subdir objects breaks symbol versioning. See https://savannah.gnu.org/bugs/?49907
- Handle bad iso 9660 better. Fixes Savannah bug https://savannah.gnu.org/bugs/?52091
- Apple (High) Sierra compatibility
- NetBSD patches
- Fixes for Rock Ridge SUSP (Thomas Schmitt)
- Reduce MinGW compilation warnings
- Add asserts to test memory allocations and misc bug fixes (Pete Batard)
- Enable CD drivers on current and future versions of FreeBSD and macOS,
  so we do not have to add every new OS version explicitly. (Robert Kausch)
- Cross-compiling friendliness (Ozkan Sezer)
- Small texinfo doc fixes (Wieland Hoffmann)
- Simplify making doc from autogen.sh
- Bug fix for https://savannah.gnu.org/bugs/?45015 (Thomas Schmitt)
- Bug fixes for #45017,#52265, and #52264
- Add more compiler warning flags, i.e. -Wshadow, -Wundef, ...
- Reduce numerous memory leaks (more though remain)

version 0.94
============
2016-10-27

- CD-TEXT fixes and improvements
  * Expose mmc_read_cdtext as a publicly accessable function
    Removes some redundant error reporting in `mmc_read_cdtext()`
    Also fixes some incorrect lengths for isrc and mcn.
  * Fix inconsistent maximal length in CD-Text extraction
  * Added new low level functions for READ SUB-CHANNEL and
    READ TOC/PMA/ATIP for CD-TEXT extraction.
  * Add cdtext binary parser and track number to public api
  *  Increase track # for short CD-Text fields

- Eject fixes:
  * Fix disc eject for Cocoa apps and support ejecting CD-Extra discs
    on OS X.
  * Make sure device is opened in read/write mode when trying to eject.

Bugs
  * Add error reporting from_733_with_err. [Bug #45014](https://savannah.gnu.org/bugs/index.php?45014)
  * Guard against malformed rock ridge iso. [Bug #45015](https://savannah.gnu.org/bugs/index.php?45015)
  * Malformed so crashes iso-info [Bug #45013:](https://savannah.gnu.org/bugs/index.php?45013:)
  * Guard against 0-size calloc [Bug #45016](https://savannah.gnu.org/bugs/index.php?45016)
  * Fix testudf segfaults/fails on big endian arches. [Bug #43995](https://savannah.gnu.org/bugs/index.php?43995).
  * add get_last_session to the win32 driver.
    Also fixes cd-paranoia behavior. [Bug #43446](https://savannah.gnu.org/bugs/index.php?43446)
  * GNU/Linux ioctl treats <= 0 as max speed. [Bug #43428](https://savannah.gnu.org/bugs/index.php?43428)
  * Fixed cdio_free leaking
  * Recursion checking in cdio_logv()
  * g++ greater than 4.0 handles "pack" [Bug #48759](https://savannah.gnu.org/bugs/index.php?48759)
  * `configure` fixes

Updates
  * Add newer OSX's
  * Squelch some `clang` error messages


version 0.93
============
2014-09-29

Most of the changes except where noted are courtesy of Robert Kausch

- Add cdio_free, iso9660_stat_free, and iso9660_xa_free functions.
- Deprecate mmc_isrc_track_read_subchannel
- Add mmc_get_track_isrc function.
- Update OS versions we recognize
- OSX, and MS Windows, ISO 9660 and other bug fixes
- Remove Coverty scan warnings and errors
- OS/2 driver performance update - KO Myung-Hun

version 0.92 (late SF Release)
2013-12-15

- Fix the botched the library release numbers, Bump current, and
  set revision to zero

version 0.91 - don't use
========================
2013-12-14

- Report Joliet level on iso-info, and an option to show whether
  Rock-Ridge extensions are present
- More debug logging in reading LSN sectors
- Document how logging works in `libcdio`
- Fixes for reading large ISO 9660 images
- Enable Rock-Ridge handling in configuration by default
- Be able to read an audio CD with exactly 100 tracks
- Microsoft Windows fixes (mingw, cygwin, Visual Studio)
- Fix UDF library bug on BigEndian CPUs (POWER, SPARC, HP/UX)
- libudf: Add udf_get_logical_volume_id() to retreive a UDF Logical Volume
- libcdio: Add cdio_default_log_handler
- libiso9660: Add iso9660_have_rr() to show if Rock-Ridge extensions are present

version 0.90
============
2012-10-27
- CD-Text overhaul and API change (Leon Merten Lohse)
- Works again (somewhat) on MinGW; tolerence for Microsoft's C compiler (Pete Batard)
- UDF, Joliet and Rock-Ridge fixes (Pete Batard)
- OSX fixes (Natalia Portillo and Robert William Fuller)
- paranoia library removed as that is GPL 2-ish. This is now a separate project
- file names in cue files are relative to the cue file rather than cwd.
- Update mmc.h to include MMC-5 commands. (Or MMC-6 since it adds nothing new)
- Add mmc_cmd2str() to show MMC command name. Show that in some errors
- Add UDF reading to iso-read and iso-info via --udf or -U (Christophe Fergeau)
- bug fixes, more tests, update documentation

version 0.83
============
2011-10-27

- Add retrieval SCSI sense reply from the most-recent MMC command. (Thomas Schmitt)
- Add exclusive read/write access for devices which is used for experimental
  writing/burning. Currently only on GNU/Linux and FreeBSD.  (Thomas Schmitt)
- MMC bug fixes
- FreeBSD drive list now shows empty drives.
- Add ability to retrieve SCSI tuple for a name and/or fake one up for
  programs that want to be cd-record compatible.
- Tolerance for OS's without timezone in their struct tm (e.g. Solaris)
  added iso9660_set_{d,l}time_with_timezone
- Add mmc_get_disk_erasable
- Update MMC Feature Profile list, DVD Book types
- Reduce range of seek in paranoia_seek to be int32_t
- Remove some potential flaws found by Coverty's static analysis tool
- Add ISRC track info to cd-info output.
- Don't wrap-around volume adjustment for cdda-player.
- Handle double-byte strings in CD-text
- --no-header on cd-info omits copyright and warranty

version 0.82
============
2009-10-27

- Remove all uses of CDIO_MIN_DRIVER,
  CDIO_MAX_DRIVER, CDIO_MIN_DEVICE_DRIVER or CDIO_MAX_DEVICE_DRIVER.
- FreeBSD get_media_changed fixes
- MingGW/Msys compilation issues
- Add OS/2 driver
- Cross compilations fixes and uclinix is like GNU/Linux
- Numerous other bug fixes

version 0.81
============
2008-10-27

- license of manual now GFDL 1.2 or later, with no invariant sections.
  Source is GPL 3.

  Thanks to Karl Berry.

- Nero image handling more complete.
    CD-Text processing.
    DAO in read_audio_sectors.
    ISRC processing.

- ISRC query for image files.

  Thanks to Robert William Fuller on the above two items

- Allow reading pregap of a track via `get_track_pregap_lsn()`. Add
  Section on "CD-DA pregap" in libcdio manual

- Allow cross-compiling to mingw32. Patch from Peter Hartley.

- Make iso9660 time setting/getting routines (iso9660_{g,s}et_{d,l}time)
  reentrant and remove bugs in that code. Courtesy Nicolas Boullis.

- OSX fixes

- Add NetBSD driver

version 0.80
============
2008-03-15

- Add get_media_changed for FreeBSD
- Add option to log summary output in cd-paranoia
- More string bounds checking to eliminate known string overflow conditions,
  e.g. Savannah bug #21910
- add --mode="any" on cd-read which uses a mmc_read_sectors with read-type
       CDIO_MMC_READ_TYPE_ANY.
- add --log-summary option to cd-paranoia. Unused option --output-info (-i) removed
- some small packaging bugs fixed

Note: this is probably the last GPL v2 release; GPL v3 on the horizon.

version 0.79
==============
2007-10-27

- iso-read: Add --ignore -k to ignore errors.

- Fix Savannah bugs:
   * [Bug #18522](https://savannah.gnu.org/bugs/index.php?18522),
   * [Bug #18563](https://savannah.gnu.org/bugs/index.php?18563),
   * [Bug #18131](https://savannah.gnu.org/bugs/index.php?18131),
   * [Bug #19221](https://savannah.gnu.org/bugs/index.php?19221),
   * [Bug #19880](https://savannah.gnu.org/bugs/index.php?19880),
   * [Bug #21147](https://savannah.gnu.org/bugs/index.php?21147), and other miscellaneous bugs and memory leaks

- `cd-info`: force CDDB disc id to be 32-bits. Problem reported by Eric Shattow.
- `cd-paranoia`: allow ripping before the first track. Problem reported
               by Eric Shattow. Fix erroneous #defines when `DO_NO_WANT_PARANOIA_COMPATIBILITY` is set. Reported by David Stockwell.
- Support for multisession CD-Extra Discs. Patch from Patrick Guimond

- Add `iso9660_fs_find_lsn_with_path` and `iso9660_ifs_find_lsn_with_path` to report the full filename path of LSN.

- improve eject code for OSX

version 0.78.2
==============
2006-10-31
- preprocessor symbol `LIBCDIO_VERSION` number has to be an integer.
  (Bug caused by naming version 0.78._1_)

version 0.78.1
==============
2006-10-27

- Fix bug in libcdio.so version numbering. Also another small bug.
  Thanks to Janos Farkas

version 0.78
============
2006-10-27
- add `mmc-tool`
- add `mmc-close-tray`
- `libudf`: can now read (extract) file data, at least for ICB strategy
          type 4.
- libcdio is starting to get updated for UTF-8 support. Strings,
  which are guaranteed to be in UTF-8, are returned as a new type
  `cdio_utf8_t`, which is typedef'd to char.
- fixes to eject. On GNU/Linux we unmount filesystems first.

version 0.77
============
2006-03-17

- Add an object-oriented C++ wrapper. (libcdio++ and libiso9660++)

- replace libpopt with getopt in `cd-drive`, `cd-info`, `iso-info`, `iso-read`
  (Peter J. Creath)
- Document cd-paranoia (Peter J. Creath)
- Add `cdio_eject_media_drive()`.
- Add more generic `read_sectors()`
- Document that `NULL` also uses default drive in `close_tray()`, `cdio_open()`
  and `cdio_open_am()`. Document `b_mode2` parameter not used in cdio ISO
  read.
- Some provision for handling Rock-Ridge device numbers.
- block read routines return success if asked to read 0 blocks.
- Start UDF handling
- increase use of enumerations more and decrease use of #defines
- OS Support:
  * DragonFly recognized as FreeBSD,
  * MinGW better tolerated,
  * GNU/Linux (and others?) LARGEFILE support
  * OpenBSD tested (no native CD driver though)

- Doxygen formatting improvements.

- Misc bugs:
  * fixed bincue driver caused core dump on set_speed and
    set_blocksize; it also called the wrong routine (from NRG) to get a
    list of cd-images.
  * read.h didn't make sure off_t was defined.
  * fixed bug in is_device() when driver_id = DRIVER_UNKNOWN or DRIVER_DEVICE.
  * OSX was freeing too much in listing drives.
  * get_hwinfo was not respecting fixed-length field boundaries in
    image drivers (strcpy->strncpy).
  * A number ISO 9660 time conversion routines corrected with respect to
    various timezone offsets, daylight savings time, and tm capabilities
- small `cdda-player` improvements - shows more CD-TEXT, and fix bug in
  non-interactive use (Yes, I sometimes use it.)
- NRG checking parses file. string tests were invalid on short < 4
  character filenames.
- Revise and improve example programs
- Security: replace all uses of strcat and strcpy with strncat and strncpy

version 0.76
============
2005-09-23

- Better compatibility with C++
- a better `eject()` routine for FreeBSD
- Fix bug in not specifying a device name in `libcio_cdda`
- Add `S_ISSOCK()` or `S_ISLNK()` macros for Rock-Ridge when environment doesn't have it, e.g. MSYS 1.0.10 with MinGW 3.4.2.
- Allow building `cd-paranoia` if Perl is not installed.
- More accurate library dependency tracking in linking and `pkg-config`
- Miscellaneous minor bug fixes.
- `cdio/cdda.h` headers no longer depends on cdio/paranoia.h but vice versa
  is true. This may require an #include <cdio/cdda.h> in some applications that
  used `<cdio/paranoia.h>` but didn't include it.

version 0.75
============
2005-07-11

- audio volume level fix on Microsoft Windows
- fix build when `--enable-shared`, `--disable-static`
- CD-Text retrieval fix
- allow the MMC timeout to be adjusted by the application
- cd-paranoia: Add option --mmc-timeout (-m) to set MMC timeout.
  We now check that integer arguments are integers and are within
  range.
- changes for `libcddb 1.1.0` API change
- remove `gcc` 4.0 warnings
- miscellaneous small bug fixes, removal of questionable idioms or
  memory leak fixes

version 0.74
============
2005-05-13

- `cd-paranoia` fixes
- `cdda-player` fixes
- `cd-drive` shows MMC level
- CD Text improvements/fixes
- eject of empty CD-ROM drives on GNU/Linux
- FreeBSD audio sub-channel time reporting fixed

version 0.73
============
2005-04-15

- Rock Ridge Extension support added
- CD audio support (play track/index, pause, set volume, read audio subchannel)
- add close tray interface (may need more work on more OSes)
- utility `cdda-player` to (show off audio audio support) added
- file time/size attributes fixes
- `cd-info`/`iso-info` show more ls-like attributes and more often
- ISO 9660 more accurate more often
- Add ability to look for ISO 9660 filesystem in unknown Disc image formats
- Add routine for getting ISO 9660 long date; short date fixes
- remove even more memory leaks
- Add enumerations and symbols to facilitate debugging
- Break out C++ example programs into a separate directory. More C++ programs.
- `gcc` 4 fixes

version 0.72
============
2005-01-31

- `cdparanoia` included -  with regression tests and sample library programs
- added setting/getting CD speed, finding the track containing an LSN.
- improve `cdrdao` image reading
- `iso-info` options more like cdrtools isoinfo.
- `cd-drive`/`cd-info` show more reading capabilities and show that.
- `cd-info` now shows the total disc size.
- Filesystem reorganization to better support growth and paranoia inclusion
- FreeBSD 6 tolerated, CAM audio read mode works.
- improve Win32 driver, e.g. audio read mode works better for ioctl.
- mode detection fixes
- all read routines check and adjust the LSN so we don't try to access
  beyond the end of the disc
- C++ fixes
- Update documentation

version 0.71
============
2005-11-20

- Some Joliet support.
- Portability fixes for C++ and older C compilers.
- Work towards XBOX support.
- TOC for DVD's works more often
- Make generic list routines and declarations and byte swapping
  routines public. Eventually everything will use glib.
- list-returning routines like `iso9660_fs_readdir()` and `iso9660_ifs_readdir()` no longer return `void *` (and require casting)
  but return the correct type.
- Some example programs have been renamed to more give meaningful names.
- Add `iso9660_ifs_is_xa()` a routine to determine if an iso image has
  XA attributes.
- `iso-info` now shows XA attributes if that is available.
- Some bug fixes


version 0.70
============
2004-09-02

- SCSI MMC interface routine (all except Darwin)
- CD-Text support (all except Darwin)
- Distinguish DVD's from CD's
- Code clean-ups and reduced code duplication
- Better CUE parsing
- Reporting drive capability is more accurate
- add constant driver_id for kind of hardware driver in build
- new drive scanning routines which pass back driver as well
  as drive string. Speeds up subsequent opens.

version 0.69
============
2004-06-25

- Add interface returning drive capabilities (`cdio_get_drive_cap()`).
- Minimal `cdrdao` image reading (thanks to Svend S. Sorensen)
- Some important (I think) bug fixes
- Redo types of LSN and LBA to allow negative values. Should model MMC3
  specs. Add max/min values for LSN.
- More complete MMC command set
- FreeBSD driver ioctl and CAM reading works better (thanks to Heiner)
- OSX drive reading works better (thanks to Justin F. Hallett)
- `cd-read` allows dumping bytes to stdout and hexdumps to a file
  via options `--no-hexdump` and `--hexdump`
- fewer error exits in drivers. Instead, a failure code is returned.
- better NRG reading (thanks to Michael Kukat via extractnrg.pl)
- better tracking of allocated variables (cd-read, `cd-info`, FreeBSD)
- iso9660: Add interface to read PVD and pick out some of the fields in that.
  `cd-info` now shows more PVD info for ISO 9660 filesystems
- `cd-info`: X-Box CD detection (via xbox team mediacenter)

version 0.68
============
2004-03-23

- More honest about mode1 reading in backends. Remove some of the bogusness.
- Fixes and simplifications to Solaris (from Ian MacIntosh): no longer
  requires root access on Sunray environments
- Win32 ioctl works now on win2k and XP (and probably NT and ME)
- compiles on cygwin with `-mno-cygwin` (needed for videolan's _vlc_)
- option `--with-versioned-libs` now checks for GNU ld.

version 0.67
============
2004-03-01

-  portability for ARM
- add `iso-read` program and regression tests
- `libiso9960`: stat routines that match level 1 ISO-9600 filenames
  translating them into Unix-style names (i.e. lowercase letters,
  with version numbers dropped.)
- expand/improve documentation.
- more graceful exits when there is no CD or can't read it.
- add `--without-versioned-libs`
- add README.libcdio and note possible problems on different OSs
  without GNU make

version 0.66
============
2004-02-15

-  Add interface for reading an ISO-9660 image
-  portability fixes (Solaris, cygwin)
-  Microsoft Windows ASPI/ DeviceIoControl code reorganization
-  NRG image reading improvements
-  Remove memory leaks
-  library symbol versioning (from Nicolas Boullis)
-  Go over documentation

version 0.65
============
2003-12-13

-  tag headers to give doxygen API documentation
-  `cd-info`/`cd-read` now can specify library level of output
-  sample program using `libiso9660` added.

version 0.64
============
2003-11-22

-  add routines to return a list of devices or scan a list of devices
   which satisfy any/all things in a capability mask. Should be useful
   for plugins that want to find a CD-DA to play or find a plugin that handles
   a particular device.
-  cd-read: new program to help diagnose reading problems.
-  cd-info: now displays date on iso9660 listing and translates filename
   to normal conventions, gives track "green" info
-  Add/expose routines to get/set time. time is reported back in entry
   stat. Routines to create ISO-9660 directories and entries must now
   supply the time to set on the entry.
-  Darwin and FreeBSD drivers closer to having native CD support, MinGW
   fixes (but not complete either)
-  BSDI fixes
-  Document more functions.

version 0.63
============

-  create libiso9660 library and install that.
-  More sample programs.
-  add library routine cdio_guess_cd_type to analyze/guess what type of
   CD or CD image we've got.
-  `cd-info` can list the files of a ISO-9660 filesystem via libiso9660 with option `--iso9660`

version 0.62
============

-  Some minimal documentation. More will follow.
-  Add a simple sample programs.
-  Add a simple regression test driver.
-  "Smart" open was scanning devices rather than devices + image drivers.

version 0.61
============

-   Cygwin/MinGW port.
-   get-default-device reworked to be smarter about finding devices.
-   cd-info: add `--no-headers`. version ID is from package now. Show default device on `--version` output.
-   API: add routine report if string refers to a device or not
-   Make use of features in libcddb 0.9.4.

version 0.6
===========

-   Bug: eject wouldn't.
-   If given `.bin`, find corresponding `.cue`. If no cue, complain.

version 0.5
===========

-  Add RPM spec file. Thanks to Manfred Tremmel <Manfred.Tremmel@iiv.de>
-  `cdinfo` renamed to `cd-info` to avoid conflicts with other existing programs
-  bug in ejecting CD's fixed
-  find `cue` file if given `bin`.
-  `cd-info`: If `libvcdinfo` is installed show general Video CD properties

version 0.4
===========

-  More regression tests.
-  Use `pkg-config(1)` support
-  NRG may be closer to being correct.

version 0.3
===========

-  reduced overall size of package. Some regression moved to a separate (large)
   package.
-  facilitate inclusion into another project's local source tree (e.g. xine)
-  version number in include
-  `cdinfo`: lists number of CDDB matches, display error message on failure,
   and can set CDDB port and http proxy
-  Bug: Narrow drivers to devices when source is a device.
-  fix some small compile warnings and configure bugs. Require libcddb 0.9.0
   or greater.

version 0.2
===========

-  Added Support for reading audio sectors
-  cdinfo can use [libcddb](http://libcddb.sourceforge.net). If installed and
   we have a CD-DA disk, we dump out CDDB information.
-  Regression tests added.
-  Don't need to open device to give get a default device.
-  Better device driver selection: We test for file/device-ness.
-  Bugs fixed (default device name on GNU/Linux),

version 0.1
===========

Routines split off from VCDImager.
