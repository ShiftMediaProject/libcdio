/*
  Copyright (C) 2003, 2004, 2005, 2008, 2011, 2012, 2024
  Rocky Bernstein <rocky@gnu.org>
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Miscellaneous things common to standalone programs. */

#ifndef UTIL_H
#define UTIL_H
#if defined(HAVE_CONFIG_H) && !defined(__CDIO_CONFIG_H__)
# include "config.h"
# define __CDIO_CONFIG_H__ 1
#endif

#include <cdio/cdio.h>
#include <cdio/logging.h>
#include <cdio/iso9660.h>
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>

#ifdef HAVE_STDARG_H
/* Get a definition for va_list.  */
#include <stdarg.h>
#endif

/* FreeBSD 4 has getopt in unistd.h. So we include that before
   getopt.h */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef ENABLE_NLS
#    include <locale.h>
#    include <libintl.h>
#    define _(String) dgettext ("cdinfo", String)
#else
/* Stubs that do something close enough.  */
#    define _(String) (String)
#endif

/* The following test is to work around the gross typo in
   systems like Sony NEWS-OS Release 4.0C, whereby EXIT_FAILURE
   is defined to 0, not 1.  */
#if !EXIT_FAILURE
# undef EXIT_FAILURE
# define EXIT_FAILURE 1
#endif

#ifndef EXIT_SUCCESS
# define EXIT_SUCCESS 0
#endif

#ifndef EXIT_INFO
# define EXIT_INFO 100
#endif

#define DEBUG 1
#if DEBUG
#define dbg_print(level, s, ...) \
   if (opts.debug_level >= level) \
     report(stderr, "%s: "s, __func__ , __VA_ARGS__)
#else
#define dbg_print(level, s, args...) 
#endif

#define err_exit(fmt, ...) \
  report(stderr, "%s: "fmt, program_name, __VA_ARGS__); \
  myexit(p_cdio, EXIT_FAILURE)

typedef enum
{
  INPUT_AUTO,
  INPUT_DEVICE,
  INPUT_BIN,
  INPUT_CUE,
  INPUT_NRG,
  INPUT_CDRDAO,
  INPUT_UNKNOWN
} source_image_t;

extern char *source_name;
extern char *program_name;
extern cdio_log_handler_t gl_default_cdio_log_handler;

/*! Common error exit routine which frees p_cdio. rc is the 
    return code to pass to exit.
*/
void myexit(CdIo_t *p_cdio, int rc);

/*! Print our version string */
void print_version (char *psz_program, const char *psz_version,
		    int no_header, bool version_only);

/*! Device input routine. If successful we return an open CdIo_t
    pointer. On error the program exits.
 */
CdIo_t *
open_input(const char *psz_source, source_image_t source_image, 
	   const char *psz_access_mode);

/*! On Unixish OS's we fill out the device name, from a short name.
    For example cdrom might become /dev/cdrom.
*/
char *fillout_device_name(const char *device_name);

/*! Prints out SCSI-MMC drive features  */
void  print_mmc_drive_features(CdIo *p_cdio);

/*! Prints out drive capabilities */
void print_drive_capabilities(cdio_drive_read_cap_t  p_read_cap,
			      cdio_drive_write_cap_t p_write_cap,
			      cdio_drive_misc_cap_t  p_misc_cap);

/*! Common place for output routine. In some environments, like XBOX,
  it may not be desireable to send output to stdout and stderr. */
void report (FILE *stream, const char *psz_format, ...);

/* Prints "ls"-like file attributes */
void print_fs_attrs(iso9660_stat_t *p_statbuf, bool b_rock, bool b_xa, 
		    const char *psz_name_untranslated, 
		    const char *psz_name_translated);

#endif /* UTIL_H */
