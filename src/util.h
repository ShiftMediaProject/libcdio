/*
  $Id: util.h,v 1.8 2004/11/04 10:08:23 rocky Exp $

  Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
  
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* Miscellaneous things common to standalone programs. */

#ifndef UTIL_H
#define UTIL_H
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/logging.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <ctype.h>

#ifdef HAVE_STDARG_H
/* Get a definition for va_list.  */
#include <stdarg.h>
#endif

#include <popt.h>
/* Accomodate to older popt that doesn't support the "optional" flag */
#ifndef POPT_ARGFLAG_OPTIONAL
#define POPT_ARGFLAG_OPTIONAL 0
#endif

#ifdef ENABLE_NLS
#include <locale.h>
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

#define DEBUG 1
#if DEBUG
#define dbg_print(level, s, args...) \
   if (opts.debug_level >= level) \
     fprintf(stderr, "%s: "s, __func__ , ##args)
#else
#define dbg_print(level, s, args...) 
#endif

#define err_exit(fmt, args...) \
  fprintf(stderr, "%s: "fmt, program_name, ##args); \
  myexit(p_cdio, EXIT_FAILURE)		     
  
typedef enum
{
  IMAGE_AUTO,
  IMAGE_DEVICE,
  IMAGE_BIN,
  IMAGE_CUE,
  IMAGE_NRG,
  IMAGE_CDRDAO,
  IMAGE_UNKNOWN
} source_image_t;

extern char *source_name;
extern char *program_name;
extern cdio_log_handler_t gl_default_cdio_log_handler;

void myexit(CdIo *cdio, int rc);

void print_version (char *program_name, const char *version,
		    int no_header, bool version_only);

char *fillout_device_name(const char *device_name);

/*! Prints out SCSI-MMC drive features  */
void  print_mmc_drive_features(CdIo *p_cdio);

/*! Prints out drive capabilities */
void print_drive_capabilities(cdio_drive_read_cap_t  p_read_cap,
			      cdio_drive_write_cap_t p_write_cap,
			      cdio_drive_misc_cap_t  p_misc_cap);


/* Common place for Output routine. Some environments like XBOX may not
   stdout and stderr. */
void report (FILE *stream, const char *psz_format, ...);

#endif /* UTIL_H */
