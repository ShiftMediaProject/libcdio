/*
  $Id: cd-read.c,v 1.4 2003/09/21 03:35:40 rocky Exp $

  Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
  
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

/* Program to debug read routines audio, mode1, mode2 forms 1 & 2. */

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

#define err_exit(fmt, args...) \
  fprintf(stderr, "%s: "fmt, program_name, ##args); \
  myexit(cdio, EXIT_FAILURE)		     
  
char *source_name = NULL;
char *program_name;

typedef enum
{
  IMAGE_AUTO,
  IMAGE_DEVICE,
  IMAGE_BIN,
  IMAGE_CUE,
  IMAGE_NRG,
  IMAGE_UNKNOWN
} source_image_t;

/* Configuration option codes */
enum {

  /* These correspond to driver_id_t in cdio.h and have to MATCH! */
  OP_SOURCE_UNDEF       = DRIVER_UNKNOWN,
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_NRG         = DRIVER_NRG,
  OP_SOURCE_DEVICE      = DRIVER_DEVICE,

  /* These are the remaining configuration options */
  OP_READ_MODE,
  OP_VERSION,

};

typedef enum
{
  READ_MODE_UNINIT,
  READ_AUDIO,
  READ_M1F1,
  READ_M1F2,
  READ_M2F1,
  READ_M2F2
} read_mode_t;

/* Structure used so we can binary sort and set the --mode switch. */
typedef struct
{
  char name[30];
  read_mode_t read_mode;
  lsn_t       start_lsn;
} subopt_entry_t;


/* Sub-options for --mode.  Note: entries must be sorted! */
subopt_entry_t modes_sublist[] = {
  {"audio",      READ_AUDIO},
  {"m1f1",       READ_M1F1},
  {"m1f2",       READ_M1F2},
  {"m2f1",       READ_M2F1},
  {"m2f2",       READ_M2F2},
  {"mode1form1", READ_M1F1},
  {"mode1form2", READ_M1F2},
  {"mode2form1", READ_M2F1},
  {"mode2form2", READ_M2F2},
  {"red",        READ_AUDIO},
};

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  int            debug_level;
  read_mode_t    read_mode;
  int            version_only;
  int            no_header;
  int            print_iso9660;
  source_image_t source_image;
  lsn_t          start_lsn;
} opts;
     
/* Comparison function called by bearch() to find sub-option record. */
static int
compare_subopts(const void *key1, const void *key2) 
{
  subopt_entry_t *a = (subopt_entry_t *) key1;
  subopt_entry_t *b = (subopt_entry_t *) key2;
  return (strncmp(a->name, b->name, 30));
}

/* CDIO logging routines */
static cdio_log_handler_t gl_default_cdio_log_handler = NULL;

static void 
_log_handler (cdio_log_level_t level, const char message[])
{
  if (level == CDIO_LOG_DEBUG && opts.debug_level < 3)
    return;

  if (level == CDIO_LOG_INFO  && opts.debug_level < 2)
    return;
  
  if (level == CDIO_LOG_WARN  && opts.debug_level < 1)
    return;
  
  gl_default_cdio_log_handler (level, message);
}


static void
hexdump (uint8_t * buffer, unsigned int len)
{
  unsigned int i;
  for (i = 0; i < len; i++, buffer++)
    {
      if (i % 16 == 0)
	printf ("0x%04x: ", i);
      printf ("%02x", *buffer);
      if (i % 2 == 1)
	printf (" ");
      if (i % 16 == 15) {
	printf ("  ");
	uint8_t *p; 
	for (p=buffer-15; p <= buffer; p++) {
	  printf("%c", isprint(*p) ?  *p : '.');
	}
	printf ("\n");
      }
    }
  printf ("\n");
}

/* Do processing of a --mode sub option. 
   Basically we find the option in the array, set it's corresponding
   flag variable to true as well as the "show.all" false. 
*/
static void
process_suboption(const char *subopt, subopt_entry_t *sublist, const int num,
                  const char *subopt_name) 
{
  subopt_entry_t *subopt_rec = 
    bsearch(subopt, sublist, num, sizeof(subopt_entry_t), 
            &compare_subopts);
  if (subopt_rec != NULL) {
    opts.read_mode = subopt_rec->read_mode;
    return;
  } else {
    unsigned int i;
    bool is_help=strcmp(subopt, "help")==0;
    if (is_help) {
      fprintf (stderr, "The list of sub options for \"%s\" are:\n", 
               subopt_name);
    } else {
      fprintf (stderr, "Invalid option following \"%s\": %s.\n", 
               subopt_name, subopt);
      fprintf (stderr, "Should be one of: ");
    }
    for (i=0; i<num-1; i++) {
      fprintf(stderr, "%s, ", sublist[i].name);
    }
    fprintf(stderr, "or %s.\n", sublist[num-1].name);
    exit (is_help ? EXIT_SUCCESS : EXIT_FAILURE);
  }
}

#define DEV_PREFIX "/dev/"
static char *
fillout_device_name(const char *device_name) 
{
#if defined(HAVE_WIN32_CDROM)
  return strdup(device_name);
#else
  unsigned int prefix_len=strlen(DEV_PREFIX);
  if (0 == strncmp(device_name, DEV_PREFIX, prefix_len))
    return strdup(device_name);
  else {
    char *full_device_name=malloc(strlen(device_name)+prefix_len);
    sprintf(full_device_name, DEV_PREFIX "%s", device_name);
    return full_device_name;
  }
#endif
}

static void
print_version (void)
{
  
  if (!opts.no_header)
    printf( _("cd-read %s (c) 2003 R. Bernstein\n"),
	    VERSION);
  printf( _("This is free software; see the source for copying conditions.\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
"));

  exit(100);
}

/* Parse a options. */
static bool
parse_options (int argc, const char *argv[])
{

  int opt;
  char *opt_arg;

  /* Command-line options */
  struct poptOption optionsTable[] = {
  
    {"mode", 'm', 
     POPT_ARG_STRING, &opt_arg, 
     OP_READ_MODE,
     "set CD-ROM read mode (audio, m1f1, m1f2, m2mf1, m2f2)", 
     "MODE-TYPE"},
    
    {"debug",       'd', 
     POPT_ARG_INT, &opts.debug_level, 0,
     "Set debugging to LEVEL"},
    
    {"start",       's', 
     POPT_ARG_INT, &opts.start_lsn, 0,
     "Set LBA to start reading from"},
    
    {"bin-file", 'b', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_BIN, "set \"bin\" CD-ROM disk image file as source", "FILE"},
    
    {"cue-file", 'c', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_CUE, "set \"cue\" CD-ROM disk image file as source", "FILE"},
    
    {"input", 'i', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_AUTO,
     "set source and determine if \"bin\" image or device", "FILE"},
    
    {"cdrom-device", 'C', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_DEVICE,
     "set CD-ROM device as source", "DEVICE"},
    
    {"nrg-file", 'N', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_NRG, "set Nero CD-ROM disk image file as source", "FILE"},
    
    {"version", 'V', POPT_ARG_NONE, NULL, OP_VERSION,
     "display version and copyright information and exit"},
    POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
  };
  poptContext optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);
  
  while ((opt = poptGetNextOpt (optCon)) != -1)
    switch (opt)
      {
      case OP_SOURCE_AUTO:
      case OP_SOURCE_BIN: 
      case OP_SOURCE_CUE: 
      case OP_SOURCE_NRG: 
      case OP_SOURCE_DEVICE: 
	if (opts.source_image != IMAGE_UNKNOWN) {
	  fprintf(stderr, 
		  "%s: another source type option given before.\n", 
		  program_name);
	  fprintf(stderr, "%s: give only one source type option.\n", 
		  program_name);
	  break;
	} 
	switch (opt) {
	case OP_SOURCE_BIN: 
	  opts.source_image  = IMAGE_BIN;
	  break;
	case OP_SOURCE_CUE: 
	  opts.source_image  = IMAGE_CUE;
	  break;
	case OP_SOURCE_NRG: 
	  opts.source_image  = IMAGE_NRG;
	  break;
	case OP_SOURCE_AUTO:
	  opts.source_image  = IMAGE_AUTO;
	  break;
	case OP_SOURCE_DEVICE: 
	opts.source_image  = IMAGE_DEVICE;
	source_name = fillout_device_name(source_name);
	break;
	}
	break;
      
      case OP_READ_MODE:
	process_suboption(opt_arg, modes_sublist,
			  sizeof(modes_sublist) / sizeof(subopt_entry_t),
                            "--mode");
        break;
      case OP_VERSION:
        print_version();
        exit (EXIT_SUCCESS);
        break;

      default:
        fprintf (stderr, "%s: %s\n", 
                 poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
                 poptStrerror(opt));
        fprintf (stderr, "error while parsing command line - try --help\n");
        exit (EXIT_FAILURE);
      }

  if (poptGetArgs (optCon) != NULL)
    {
      fprintf (stderr, "error - no arguments expected! - try --help\n");
      exit (EXIT_FAILURE);
    }

  {
    const char *remaining_arg = poptGetArg(optCon);
    if ( remaining_arg != NULL) {
      if (opts.source_image != IMAGE_UNKNOWN) {
	fprintf (stderr, 
		 "%s: Source specified in option %s and as %s\n", 
		 program_name, source_name, remaining_arg);
	exit (EXIT_FAILURE);
      }
      
      if (opts.source_image == OP_SOURCE_DEVICE)
	source_name = fillout_device_name(remaining_arg);
      
      if ( (poptGetArgs(optCon)) != NULL) {
	fprintf (stderr, 
		 "%s: Source specified in previously %s and %s\n", 
		 program_name, source_name, remaining_arg);
	exit (EXIT_FAILURE);
	
      }
    }
  }
  
  if (opts.read_mode == READ_MODE_UNINIT) {
    fprintf(stderr, 
	    "%s: Need to give a read mode (audio, m1f1, m1f2, m1f2 or m2f2)\n",
	    program_name);
    exit(10);
  }

  if (opts.start_lsn == CDIO_INVALID_LSN) {
    fprintf(stderr, 
	    "%s: Need to give a starting LBA\n",
	    program_name);
    exit(11);
  }


  return true;
}

static void 
myexit(CdIo *cdio, int rc) 
{
  if (NULL != cdio) 
    cdio_destroy(cdio);
  exit(rc);
}

int
main(int argc, const char *argv[])
{
  uint8_t buffer[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  unsigned int blocklen=CDIO_CD_FRAMESIZE_RAW;
  CdIo *cdio=NULL;
  
  program_name = strrchr(argv[0],'/');
  program_name = program_name ? program_name+1 : strdup(argv[0]);

  opts.debug_level   = 0;
  opts.start_lsn     = CDIO_INVALID_LSN;
  opts.read_mode     = READ_MODE_UNINIT;
  opts.source_image  = IMAGE_UNKNOWN;

  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
     
  /* end of local declarations */

  switch (opts.source_image) {
  case IMAGE_UNKNOWN:
  case IMAGE_AUTO:
    cdio = cdio_open (source_name, DRIVER_UNKNOWN);
    if (cdio==NULL) {
      err_exit("%s: Error in automatically selecting driver with input\n", 
	       program_name);
    } 
    break;
  case IMAGE_DEVICE:
    cdio = cdio_open (source_name, DRIVER_DEVICE);
    if (cdio==NULL) {
      err_exit("%s: Error in automatically selecting device with input\n", 
	       program_name);
    } 
    break;
  case IMAGE_BIN:
    cdio = cdio_open (source_name, DRIVER_BINCUE);
    if (cdio==NULL) {
      err_exit("%s: Error in opeing bin/cue\n", 
	       program_name);
    } 
    break;
  case IMAGE_CUE:
    cdio = cdio_open_cue(source_name);
    if (cdio==NULL) {
      err_exit("%s: Error in opening cue/bin with input\n", 
	       program_name);
    } 
    break;
  case IMAGE_NRG:
    cdio = cdio_open (source_name, DRIVER_NRG);
    if (cdio==NULL) {
      err_exit("%s: Error in opening NRG with input\n", 
	       program_name);
    } 
    break;
  }

  switch (opts.read_mode) {
  case READ_AUDIO:
    cdio_read_audio_sector(cdio, &buffer, opts.start_lsn);
    break;
  case READ_M1F1:
    cdio_read_mode1_sector(cdio, &buffer, opts.start_lsn, false);
    blocklen=CDIO_CD_FRAMESIZE;
    break;
  case READ_M1F2:
    cdio_read_mode1_sector(cdio, &buffer, opts.start_lsn, true);
    break;
  case READ_M2F1:
    cdio_read_mode2_sector(cdio, &buffer, opts.start_lsn, false);
    break;
  case READ_M2F2:
    cdio_read_mode2_sector(cdio, &buffer, opts.start_lsn, true);
    break;
  case READ_MODE_UNINIT:
    err_exit("%s: Reading mode not set\n", program_name);
    break;
  }

  hexdump(buffer, blocklen);
  cdio_destroy(cdio);
  return 0;
}
