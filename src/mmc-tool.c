/*
  $Id: mmc-tool.c,v 1.5 2006/04/12 03:23:46 rocky Exp $

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
  OPT_HANDLED = 0,
  OPT_USAGE,
  OPT_VERSION,
} option_t;

typedef enum {
  /* These are the remaining configuration options */
  OP_FINISHED = 0,
  OP_BLOCKSIZE,
  OP_CLOSETRAY,
  OP_EJECT,
  OP_IDLE,
  OP_MCN,
  OP_SPEED,
} operation_enum_t;

typedef struct 
{
  operation_enum_t op;
  union 
  {
    long int i_num;
    char * psz;
  } arg;
} operation_t;
  

enum { MAX_OPS = 10 };

unsigned int last_op = 0;
operation_t operation[MAX_OPS] = { {OP_FINISHED, {0}} };

static void 
push_op(operation_t *p_op) 
{
  if (last_op < MAX_OPS) {
    memcpy(&operation[last_op], p_op, sizeof(operation_t));
    last_op++;
  }
}

/* Parse a options. */
static bool
parse_options (int argc, char *argv[])
{
  int opt;
  operation_t op;

  const char* helpText =
    "Usage: %s [OPTION...]\n"
    " Issues libcdio Multimedia commands\n"
    "options: \n"
    "  -b, --blocksize[=INT]           set blocksize. If no block size or a \n"
    "                                  zero blocksize is given we return the\n"
    "                                  current setting.\n"
    "  -e, --eject [drive]             eject drive via ALLOW_MEDIUM_REMOVAL\n"
    "                                  and a MMC START/STOP command\n"
    "  -I, --idle                      set CD-ROM to idle or power down\n"
    "                                  via MMC START/STOP command"
    "  -m, --mcn                       get media catalog number (AKA UPC)\n"
    "  -s, --speed-KB=INT              Set drive speed to SPEED K bytes/sec\n"
    "                                  Note: 1x = 176 KB/s          \n"
    "  -S, --speed-X=INT               Set drive speed to INT X\n"
    "                                  Note: 1x = 176 KB/s          \n"
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
  const char* optionsString = "b::c:e::Is:V?";
  struct option optionsTable[] = {
  
    {"blocksize", optional_argument, &opts.i_blocksize, 'b' },
    /*    {"close", required_argument, NULL, 'c'}, */
    {"eject", optional_argument, NULL, 'e'},
    {"idle", no_argument, NULL, 'I'},
    {"mcn",   no_argument, NULL, 'm'},
    {"speed-KB", required_argument, NULL, 's'},
    {"speed-X",  required_argument, NULL, 'S'},

    {"version", no_argument, NULL, 'V'},
    {"help", no_argument, NULL, '?' },
    {"usage", no_argument, NULL, OPT_USAGE },
    { NULL, 0, NULL, 0 }
  };
  
  while ((opt = getopt_long(argc, argv, optionsString, optionsTable, NULL)) >= 0)
    switch (opt)
      {
      case 'b': 
	op.op = OP_BLOCKSIZE;
	op.arg.i_num = opts.i_blocksize;
	push_op(&op);
	break;
      case 'c': 
	op.op = OP_CLOSETRAY;
	op.arg.psz = strdup(optarg);
	push_op(&op);
	break;
      case 'e': 
	op.op = OP_EJECT;
	op.arg.psz=NULL;
	if (optarg) op.arg.psz = strdup(optarg);
	push_op(&op);
	break;
      case 'I': 
	op.op = OP_IDLE;
	op.arg.psz=NULL;
	push_op(&op);
	break;
      case 'm': 
	op.op = OP_MCN;
	op.arg.psz=NULL;
	push_op(&op);
	break;
      case 's': 
	op.op = OP_SPEED;
	op.arg.i_num=atoi(optarg);
	push_op(&op);
	break;
      case 'S': 
	op.op = OP_SPEED;
	op.arg.i_num=176 * atoi(optarg);
	push_op(&op);
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
	
      case OPT_USAGE:
	fprintf(stderr, usageText, program_name);
	free(program_name);
	exit(EXIT_FAILURE);
	break;

      case OPT_HANDLED:
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
    const operation_t *p_op = &operation[i];
    switch (p_op->op) {
    case OP_SPEED:
      rc = mmc_set_speed(p_cdio, p_op->arg.i_num);
      report(stdout, "%s (mmc_set_speed): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      break;
    case OP_BLOCKSIZE:
      if (p_op->arg.i_num) {
	driver_return_code_t rc = mmc_set_blocksize(p_cdio, p_op->arg.i_num);
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
#if 0
    case OP_CLOSETRAY:
      rc = mmc_close_tray(p_cdio, op.arg.psz);
      report(stdout, "%s (mmc_close_tray): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      free(p_op->arg.psz);
      break;
#endif
    case OP_EJECT:
      rc = mmc_eject_media(p_cdio);
      report(stdout, "%s (mmc_eject_media): %s\n", program_name, 
	     cdio_driver_errmsg(rc));
      if (p_op->arg.psz) free(p_op->arg.psz);
      break;
    case OP_IDLE:
      rc = mmc_start_stop_media(p_cdio, false, false, true);
      report(stdout, "%s (mmc_start_stop_media - powerdown): %s\n", 
	     program_name, cdio_driver_errmsg(rc));
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
