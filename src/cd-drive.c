/*
  $Id: cd-drive.c,v 1.2 2004/04/25 03:52:37 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  
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

/* Program to show drivers installed and capibilites of CD drives. */

#include "util.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <cdio/cdio.h>

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  uint32_t       debug_level;
  int            version_only;
  int            silent;
  source_image_t source_image;
} opts;
     
/* Configuration option codes */
enum {
  OP_SOURCE_DEVICE,
  
  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

/* Parse a all options. */
static bool
parse_options (int argc, const char *argv[])
{
  int opt;

  struct poptOption optionsTable[] = {
    {"debug",       'd', POPT_ARG_INT, &opts.debug_level, 0,
     "Set debugging to LEVEL"},
    
    {"cdrom-device", 'i', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, 
     &source_name, OP_SOURCE_DEVICE,
     "show only info about CD-ROM device", "DEVICE"},
    
    {"quiet",       'q', POPT_ARG_NONE, &opts.silent, 0,
     "Don't produce warning output" },
    
    {"version", 'V', POPT_ARG_NONE, &opts.version_only, 0,
     "display version and copyright information and exit"},
    POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
  };
  poptContext optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv[0]);

  while ((opt = poptGetNextOpt (optCon)) != -1) {
    switch (opt) {
      
    case OP_SOURCE_DEVICE: 
      if (opts.source_image != IMAGE_UNKNOWN) {
	fprintf(stderr, 
		"%s: another source type option given before.\n", 
		program_name);
	fprintf(stderr, "%s: give only one source type option.\n", 
		program_name);
	break;
      } else {
	opts.source_image  = IMAGE_DEVICE;
	source_name = fillout_device_name(source_name);
	break;
      }
      break;
      
    default:
      poptFreeContext(optCon);
      return false;
    }
  }
  {
    const char *remaining_arg = poptGetArg(optCon);
    if ( remaining_arg != NULL) {
      if (opts.source_image != IMAGE_UNKNOWN) {
	fprintf (stderr, 
		 "%s: Source specified in option %s and as %s\n", 
		 program_name, source_name, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
      }
      
      if (opts.source_image == IMAGE_DEVICE)
	source_name = fillout_device_name(remaining_arg);
      else 
	source_name = strdup(remaining_arg);
      
      if ( (poptGetArgs(optCon)) != NULL) {
	fprintf (stderr, 
		 "%s: Source specified in previously %s and %s\n", 
		 program_name, source_name, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
	
      }
    }
  }
  
  poptFreeContext(optCon);
  return true;
}

/* CDIO logging routines */

static cdio_log_handler_t gl_default_cdio_log_handler = NULL;

static void 
_log_handler (cdio_log_level_t level, const char message[])
{
  if (level == CDIO_LOG_DEBUG && opts.debug_level < 2)
    return;

  if (level == CDIO_LOG_INFO  && opts.debug_level < 1)
    return;
  
  if (level == CDIO_LOG_WARN  && opts.silent)
    return;
  
  gl_default_cdio_log_handler (level, message);
}

/* Initialize global variables. */
static void 
init(void) 
{
  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);

  /* Default option values. */
  opts.silent        = false;
  opts.debug_level   = 0;
  opts.source_image  = IMAGE_UNKNOWN;
}

int
main(int argc, const char *argv[])
{
  CdIo *cdio=NULL;
  
  init();

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
     
  print_version(program_name, VERSION, false, opts.version_only);

  if (opts.debug_level == 3) {
    cdio_loglevel_default = CDIO_LOG_INFO;
  } else if (opts.debug_level >= 4) {
    cdio_loglevel_default = CDIO_LOG_DEBUG;
  }

  if (NULL == source_name) {
    char *default_device;

    cdio = cdio_open (NULL, DRIVER_UNKNOWN);

    if (NULL != cdio) {
      default_device = cdio_get_default_device(cdio);
      
      printf("The driver selected is %s\n", cdio_get_driver_name(cdio));

      if (default_device) {
	printf("The default device for this driver is %s\n", default_device);
      }
    
      free(default_device);
      cdio_destroy(cdio);
      cdio=NULL;
      printf("\n");
    }
  }
  
  /* Print out a drivers available */
  {
    driver_id_t driver_id;

    printf("Drivers available...\n");
    for (driver_id=CDIO_MIN_DRIVER; driver_id<=CDIO_MAX_DRIVER; driver_id++)
      printf("  %-35s: %s\n", 
	     cdio_driver_describe(driver_id),
	     cdio_have_driver(driver_id) ? "Yes" : "No");
    printf("\n\n");
  }
  
    
  if (NULL == source_name) {
    /* Print out a list of CD-drives */

    char **cd_drives=NULL, **cd;
    
    cd_drives = cdio_get_devices(DRIVER_DEVICE);
    if (NULL != cd_drives) 
      for( cd = cd_drives; *cd != NULL; cd++ ) {
	cdio_drive_cap_t i_drive_cap =  cdio_get_drive_cap_dev(*cd);

	printf("Drive %s\n", *cd);
	print_drive_capabilities(i_drive_cap);
	printf("\n");
      }
    
    cdio_free_device_list(cd_drives);
    free(cd_drives);
    cd_drives = NULL;
  } else {
    /* Print CD-drive info for given source */

    cdio_drive_cap_t i_drive_cap =  cdio_get_drive_cap_dev(source_name);
    
    printf("Drive %s\n", source_name);
    print_drive_capabilities(i_drive_cap);
    printf("\n");
  }

  myexit(cdio, EXIT_SUCCESS);
  /* Not reached:*/
  return(EXIT_SUCCESS);
}
