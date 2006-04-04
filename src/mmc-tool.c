/*
  $Id: mmc-tool.c,v 1.1 2006/04/04 00:20:48 rocky Exp $

  Copyright (C) 2006 Rocky Bernstein <rocky@panix.com>
  
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

/* A program to using the MMC interface to list CD and drive features
   from the MMC GET_CONFIGURATION command . */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include "util.h"
#include "getopt.h"

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  int   i_speed;
  int   i_blocksize;
} opts;

static void
init(const char *argv0) 
{
  program_name = strrchr(argv0,'/');
  program_name = program_name ? strdup(program_name+1) : strdup(argv0);
  opts.i_blocksize = 0;
}

/* Configuration option codes */
typedef enum {
  OP_HANDLED = 0,
  OP_USAGE,

  /* These are the remaining configuration options */
  OP_BLOCKSIZE,
  OP_EJECT,
  OP_MCN,
  OP_SPEED,
  OP_VERSION,
} operation_t;

enum { MAX_OPS = 10 };

unsigned int last_op = 0;
operation_t operation[MAX_OPS] = {OP_HANDLED,};

/* Parse a options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt;

  const char* helpText =
    "Usage: %s [OPTION...]\n"
    " Issues libcdio Multimedia commands\n"
    "options: \n"
    "  -b, --blocksize[=INT]           set blocksize. If no block size or a \n"
    "                                  zero blocksize is given we return the\n"
    "                                  current setting.\n"
    "  -e, --eject                     eject drive\n"
    "  -m, --mcn                       get media catalog number (AKA UPC)\n"
    "  -s, --speed=INT                 Set drive speed to SPEED Kb/s\n"
    "                                  Note: 1x = 4234KB/s          \n"
    "  -V, --version                   display version and copyright information\n"
    "                                  and exit\n"
    "\n"
    "Help options:\n"
    "  -?, --help                      Show this help message\n"
    "  --usage                         Display brief usage message\n";
  
  const char* usageText =
    "Usage: %s [-a|--access-mode STRING] [-m|--mode MODE-TYPE]\n"
    "        [-s|--speed INT]\n"
    "        [-V|--version] [-?|--help] [--usage]\n";
  
  /* Command-line options */
  const char* optionsString = "b::s:V?";
  struct option optionsTable[] = {
  
    {"blocksize", optional_argument, &opts.i_blocksize, 'b' },
    {"eject", no_argument, NULL, 'e'},
    {"mcn",   no_argument, NULL, 'm'},
    {"speed", required_argument, NULL, 's'},

    {"version", no_argument, NULL, 'V'},
    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OP_USAGE },
    { NULL, 0, NULL, 0 }
  };
  
  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0)
    switch (opt)
      {
      case 'b': 
	operation[last_op++] = OP_BLOCKSIZE;
	break;
      case 'e': 
	operation[last_op++] = OP_EJECT;
	break;
      case 'm': 
	operation[last_op++] = OP_MCN;
	break;
      case 's': 
	opts.i_speed = atoi(optarg); 
	operation[last_op++] = OP_SPEED;
	break;
      case 'V':
        print_version(program_name, VERSION, 0, true);
	free(program_name);
        exit (EXIT_SUCCESS);
        break;

      case '?':
	fprintf(stdout, helpText, program_name);
	free(program_name);
	exit(EXIT_INFO);
	break;
	
      case OP_USAGE:
	fprintf(stderr, usageText, program_name);
	free(program_name);
	exit(EXIT_FAILURE);
	break;

      case OP_HANDLED:
	break;
      }

  if (optind < argc) {
    const char *remaining_arg = argv[optind++];

    if (source_name != NULL) {
      report( stderr, "%s: Source specified in option %s and as %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }

    source_name = strdup(remaining_arg);
      
    if (optind < argc) {
      report( stderr, "%s: Source specified in previously %s and %s\n", 
	      program_name, source_name, remaining_arg );
      free(program_name);
      exit (EXIT_FAILURE);
    }
  }
  
  return true;
}

int
main(int argc, char *argv[])
{
  CdIo_t *p_cdio;

  driver_return_code_t rc;
  unsigned int i;

  init(argv[0]);
  
  parse_options(argc, argv);
  p_cdio = cdio_open (source_name, DRIVER_DEVICE);

  if (NULL == p_cdio) {
    printf("Couldn't find CD\n");
    return 1;
  } 

  for (i=0; i < last_op; i++) {
    switch (operation[i]) {
    case OP_SPEED:
      rc = mmc_set_speed(p_cdio, opts.i_speed);
      report(stdout, "%s (mmc_set_speed): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      break;
    case OP_BLOCKSIZE:
      if (opts.i_blocksize) {
	driver_return_code_t rc = mmc_set_blocksize(p_cdio, opts.i_blocksize);
	report(stdout, "%s (mmc_set_blocksize): %s\n", program_name, 
	       cdio_driver_errmsg(rc));
      } else {
	int i_blocksize = mmc_get_blocksize(p_cdio);
	if (i_blocksize > 0) {
	  report(stdout, "%s (mmc_get_blocksize): %d\n", program_name, 
		 i_blocksize);
	} else {
	  report(stdout, "%s (mmc_get_blocksize): can't retrieve.\n", 
		 program_name);
	}
      }
      break;
    case OP_EJECT:
      rc = mmc_eject_media(p_cdio);
      report(stdout, "%s (mmc_eject_media): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      break;
    case OP_MCN: 
      {
	char *psz_mcn = mmc_get_mcn(p_cdio);
	if (psz_mcn) {
	  report(stdout, "%s (mmc_get_mcn): %s\n", program_name, psz_mcn);
	  free(psz_mcn);
	} else
	  report(stdout, "%s (mmc_get_mcn): can't retrieve\n", program_name);
      }
      
      break;
    default:
      ;
    }
  }
  

  free(source_name);
  cdio_destroy(p_cdio);
  
  return rc;
}
