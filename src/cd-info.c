/*
    $Id: cd-info.c,v 1.21 2003/08/16 17:31:40 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 1996,1997,1998  Gerd Knorr <kraxel@bytesex.org>
         and       Heiko Eiﬂfeldt <heiko@hexco.de>

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
/*
  CD Info - prints various information about a CD, and detects the type of 
  the CD.
 
*/
#define PROGRAM_NAME "CD Info"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <popt.h>
/* Accomodate to older popt that doesn't support the "optional" flag */
#ifndef POPT_ARGFLAG_OPTIONAL
#define POPT_ARGFLAG_OPTIONAL 0
#endif

#include "config.h"

#ifdef HAVE_CDDB
#include <cddb/cddb.h>
#endif

#ifdef HAVE_VCDINFO
#include <libvcd/logging.h>
#include <libvcd/files.h>
#include <libvcd/info.h>
#endif

#include <cdio/cdio.h>
#include <cdio/logging.h>
#include <cdio/util.h>
#include <cdio/cd_types.h>

#include <fcntl.h>
#ifdef __linux__
# include <linux/version.h>
# include <linux/cdrom.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,50)
#  include <linux/ucdrom.h>
# endif
#endif
 
#include <errno.h>

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
  myexit(cdio, EXIT_FAILURE)		     
  

#if 0
#define STRONG "\033[1m"
#define NORMAL "\033[0m"
#else
#define STRONG "__________________________________\n"
#define NORMAL ""
#endif

#if CDIO_IOCTL_FINISHED
struct cdrom_mcn           mcn;
struct cdrom_multisession  ms;
struct cdrom_subchnl       sub;
#endif

char *source_name = NULL;
char *program_name;

const char *argp_program_version     = PROGRAM_NAME " " VERSION;
const char *argp_program_bug_address = "rocky@panix.com";

typedef enum
{
  IMAGE_AUTO,
  IMAGE_DEVICE,
  IMAGE_BIN,
  IMAGE_CUE,
  IMAGE_NRG,
  IMAGE_UNKNOWN
} source_image_t;

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  int            no_tracks;
  int            no_ioctl;
  int            no_analysis;
#ifdef HAVE_CDDB
  int            no_cddb;     /* If set the below are meaningless. */
  char          *cddb_email;  /* email to report to CDDB server. */
  char          *cddb_server; /* CDDB server to contact */
  int            cddb_port;   /* port number to contact CDDB server. */
  int            cddb_http;   /* 1 if use http proxy */
  int            cddb_timeout;
  bool           cddb_disable_cache; /* If set the below is meaningless. */
  char          *cddb_cachedir;
#endif
#ifdef HAVE_VCDINFO
  int            no_vcd;
#endif
  uint32_t       debug_level;
  int            silent;
  int            version_only;
  int            no_header;
  source_image_t source_image;
} opts;
     
/* Program documentation. */
const char doc[] = 
  PROGRAM_NAME " -- Get information about a Compact Disk or CD image.";

/* A description of the arguments we accept. */
const char args_doc[] = "[DEVICE or DISK-IMAGE]";


/* Configuration option codes */
enum {
  
  OP_SOURCE_UNDEF,
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_NRG ,
  OP_SOURCE_DEVICE,
  
  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

char *temp_str;

struct poptOption optionsTable[] = {
  {"debug",       'd', POPT_ARG_INT, &opts.debug_level, 0,
   "Set debugging to LEVEL"},
  
  {"quiet",       'q', POPT_ARG_NONE, &opts.silent, 0,
   "Don't produce warning output" },
  
  {"no-tracks",   'T', POPT_ARG_NONE, &opts.no_tracks, 0,
   "Don't show track information"},
  
  {"no-analyze",  'A', POPT_ARG_NONE, &opts.no_analysis, 0,
   "Don't filesystem analysis"},
  
#ifdef HAVE_CDDB
  {"no-cddb",     'a', POPT_ARG_NONE, &opts.no_cddb, 0,
   "Don't look up audio CDDB information or print that"},

  {"cddb-port",   'P', POPT_ARG_INT, &opts.cddb_port, 0,
   "CDDB port number to use (default 8880)"},

  {"cddb-http",   'H', POPT_ARG_NONE, &opts.cddb_http, 0,
   "Lookup CDDB via HTTP proxy (default no proxy)"},

  {"cddb-server", '\0', POPT_ARG_STRING, &opts.cddb_server, 0,
   "CDDB server to contact for information (default: freedb.freedb.org)"},

  {"cddb-cache",  '\0', POPT_ARG_STRING, &opts.cddb_cachedir, 0,
   "Location of CDDB cache directory (default ~/.cddbclient)"},

  {"cddb-email",  '\0', POPT_ARG_STRING, &opts.cddb_email, 0,
   "Email address to give CDDB server (default me@home"},

  {"no-cddb-cache", '\0', POPT_ARG_NONE, &opts.cddb_disable_cache, 0,
   "Lookup CDDB via HTTP proxy (default no proxy)"},

  {"cddb-timeout",  '\0', POPT_ARG_INT, &opts.cddb_timeout, 0,
   "CDDB timeout value in seconds (default 10 seconds)"},

#endif
  
#ifdef HAVE_VCDINFO
  {"no-vcd",   'v', POPT_ARG_NONE, &opts.no_vcd, 0,
   "Don't look up Video CD information"},
#endif
  {"no-ioctl",  'I', POPT_ARG_NONE,  &opts.no_ioctl, 0,
   "Don't show ioctl() information"},
  
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

  {"no-header", '\0', POPT_ARG_NONE, &opts.no_header, 
   0, "Don't display header and copyright (for regression testing)"},
  
  {"nrg-file", 'N', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
   OP_SOURCE_NRG, "set Nero CD-ROM disk image file as source", "FILE"},
  
  {"quiet", 'q', POPT_ARG_NONE, &opts.silent, 0, 
   "show only critical messages"},
  
  {"version", 'V', POPT_ARG_NONE, &opts.version_only, 0,
   "display version and copyright information and exit"},
  POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
};

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


/* Parse a single option. */
static bool
parse_options (poptContext optCon)
{
  int opt;

  while ((opt = poptGetNextOpt (optCon)) != -1) {
    switch (opt) {
      
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
      
    default:
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
  
  return true;
}

static void
print_version (bool version_only)
{
  
  driver_id_t driver_id;

  if (!opts.no_header)
    printf( _("CD Info %s (c) 2003 Gerd Knorr, Heiko Eiﬂfeldt & R. Bernstein\n"),
	    VERSION);
  printf( _("This is free software; see the source for copying conditions.\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
"));

  if (version_only) {
    char *default_device;
    for (driver_id=DRIVER_UNKNOWN+1; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
      if (cdio_have_driver(driver_id)) {
	printf("Have driver: %s\n", cdio_driver_describe(driver_id));
      }
    }
    default_device=cdio_get_default_device(NULL);
    if (default_device)
      printf("Default CD-ROM device: %s\n", default_device);
    else
      printf("No CD-ROM device found.\n");
    exit(100);
  }
  
}

static void 
myexit(CdIo *cdio, int rc) 
{
  if (NULL != cdio) 
    cdio_destroy(cdio);
  exit(rc);
}


/* ------------------------------------------------------------------------ */
/* CDDB                                                                     */

/* 
   Returns the sum of the decimal digits in a number. Eg. 1955 = 20
*/
static int
cddb_dec_digit_sum(int n)
{
  int ret=0;
  
  for (;;) {
    ret += n%10;
    n    = n/10;
    if (!n)
      return ret;
  }
}

/* Return the number of seconds (discarding frame portion) of an MSF */
static inline unsigned int
msf_seconds(msf_t *msf) 
{
  return from_bcd8(msf->m)*60 + from_bcd8(msf->s);
}

/* 
   Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
   consisting of the concatenation of 3 things:
      the sum of the decimal digits of sizes of all tracks, 
      the total length of the disk, and 
      the number of tracks.
*/
static unsigned long
cddb_discid(CdIo *cdio, int num_tracks)
{
  int i,t,n=0;
  msf_t start_msf;
  msf_t msf;
  
  for (i = 1; i <= num_tracks; i++) {
    cdio_get_track_msf(cdio, i, &msf);
    n += cddb_dec_digit_sum(msf_seconds(&msf));
  }

  cdio_get_track_msf(cdio, 1, &start_msf);
  cdio_get_track_msf(cdio, CDIO_CDROM_LEADOUT_TRACK, &msf);
  
  t = msf_seconds(&msf) - msf_seconds(&start_msf);
  
  return ((n % 0xff) << 24 | t << 8 | num_tracks);
}


/* CDIO logging routines */

static cdio_log_handler_t gl_default_cdio_log_handler = NULL;
#ifdef HAVE_CDDB
static cddb_log_handler_t gl_default_cddb_log_handler = NULL;
#endif
#ifdef HAVE_VCDINFO
static vcd_log_handler_t gl_default_vcd_log_handler = NULL;
#endif

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


#ifdef HAVE_CDDB
static void 
print_cddb_info(CdIo *cdio, track_t num_tracks, track_t first_track_num) {
  int i, matches;
  
  cddb_conn_t *conn =  cddb_new();
  cddb_disc_t *disc = NULL;

  if (!conn) {
    fprintf(stderr, "%s: unable to initialize libcddb\n", program_name);
    goto cddb_destroy;
  }

  if (NULL == opts.cddb_email) 
    cddb_set_email_address(conn, "me@home");
  else 
    cddb_set_email_address(conn, opts.cddb_email);

  if (NULL == opts.cddb_server) 
    cddb_set_server_name(conn, "freedb.freedb.org");
  else 
    cddb_set_server_name(conn, opts.cddb_server);

  if (opts.cddb_timeout >= 0) 
    cddb_set_timeout(conn, opts.cddb_timeout);

  cddb_set_server_port(conn, opts.cddb_port);

  if (opts.cddb_http) 
    cddb_http_enable(conn);
  else 
    cddb_http_disable(conn);

  if (NULL != opts.cddb_cachedir) 
    cddb_cache_set_dir(conn, opts.cddb_cachedir);
    
  if (opts.cddb_disable_cache) 
    cddb_cache_disable(conn);
    
  disc = cddb_disc_new();
  if (!disc) {
    fprintf(stderr, "%s: unable to create CDDB disc structure", program_name);
    goto cddb_destroy;
  }
  for(i = 0; i < num_tracks; i++) {
    cddb_track_t *t = cddb_track_new(); 
    t->frame_offset = cdio_get_track_lba(cdio, i+first_track_num);
    cddb_disc_add_track(disc, t);
  }
    
  disc->length = 
    cdio_get_track_lba(cdio, CDIO_CDROM_LEADOUT_TRACK) 
    / CDIO_CD_FRAMES_PER_SEC;

  if (!cddb_disc_calc_discid(disc)) {
    fprintf(stderr, "%s: libcddb calc discid failed.\n", 
	    program_name);
    goto cddb_destroy;
  }

  matches = cddb_query(conn, disc);

  if (-1 == matches) 
    printf("%s: %s\n", program_name, cddb_error_str(errno));
  else {
    printf("%s: Found %d matches in CDDB\n", program_name, matches);
    for (i=1; i<=matches; i++) {
      cddb_read(conn, disc);
      cddb_disc_print(disc);
      cddb_query_next(conn, disc);
    }
  }
  
  cddb_disc_destroy(disc);
  cddb_destroy:
  cddb_destroy(conn);
}
#endif

#ifdef HAVE_VCDINFO
static void 
print_vcd_info(void) {
  vcdinfo_open_return_t open_rc;
  vcdinfo_obj_t *obj;
  open_rc = vcdinfo_open(&obj, &source_name, DRIVER_UNKNOWN, NULL);
  switch (open_rc) {
  case VCDINFO_OPEN_VCD: 
    if (vcdinfo_get_format_version (obj) == VCD_TYPE_INVALID) {
      fprintf(stderr, "VCD format detection failed");
      vcdinfo_close(obj);
      return;
    }
    fprintf (stdout, "format: %s\n", vcdinfo_get_format_version_str(obj));
    fprintf (stdout, "album id: `%.16s'\n",  vcdinfo_get_album_id(obj));
    fprintf (stdout, "volume count: %d\n",   vcdinfo_get_volume_count(obj));
    fprintf (stdout, "volume number: %d\n",  vcdinfo_get_volume_num(obj));
    fprintf (stdout, "system id: `%s'\n",    vcdinfo_get_system_id(obj));
    fprintf (stdout, "volume id: `%s'\n",    vcdinfo_get_volume_id(obj));
    fprintf (stdout, "volumeset id: `%s'\n", vcdinfo_get_volumeset_id(obj));
    fprintf (stdout, "publisher id: `%s'\n", vcdinfo_get_publisher_id(obj));
    fprintf (stdout, "preparer id: `%s'\n",  vcdinfo_get_preparer_id(obj));
    fprintf (stdout, "application id: `%s'\n", 
	     vcdinfo_get_application_id(obj));

    break;
  case VCDINFO_OPEN_ERROR:
    fprintf (stderr, "Error in Video CD opening of %s\n", source_name);
    return;
    break;
  case VCDINFO_OPEN_OTHER:
    fprintf (stderr, "Even though we thought this was a Video CD, "
	     " further inspection says it is not.\n");
    break;
  }
  vcdinfo_close(obj);
    
}
#endif 

static void
print_analysis(int ms_offset, cdio_analysis_t cdio_analysis, 
	       cdio_fs_anal_t fs, int first_data, unsigned int num_audio, 
	       track_t num_tracks, track_t first_track_num, CdIo *cdio)
{
  int need_lf;
  
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_NO_DATA:
    if (num_audio > 0) {
      printf("Audio CD, CDDB disc ID is %08lx\n", 
	     cddb_discid(cdio, num_tracks));
#ifdef HAVE_CDDB
      if (!opts.no_cddb) print_cddb_info(cdio, num_tracks, first_track_num);
#endif      
    }
    break;
  case CDIO_FS_ISO_9660:
    printf("CD-ROM with ISO 9660 filesystem");
    if (fs & CDIO_FS_ANAL_JOLIET) {
      printf(" and joliet extension level %d", cdio_analysis.joliet_level);
    }
    if (fs & CDIO_FS_ANAL_ROCKRIDGE)
      printf(" and rockridge extensions");
    printf("\n");
    break;
  case CDIO_FS_ISO_9660_INTERACTIVE:
    printf("CD-ROM with CD-RTOS and ISO 9660 filesystem\n");
    break;
  case CDIO_FS_HIGH_SIERRA:
    printf("CD-ROM with High Sierra filesystem\n");
    break;
  case CDIO_FS_INTERACTIVE:
    printf("CD-Interactive%s\n", num_audio > 0 ? "/Ready" : "");
    break;
  case CDIO_FS_HFS:
    printf("CD-ROM with Macintosh HFS\n");
    break;
  case CDIO_FS_ISO_HFS:
    printf("CD-ROM with both Macintosh HFS and ISO 9660 filesystem\n");
    break;
  case CDIO_FS_UFS:
    printf("CD-ROM with Unix UFS\n");
    break;
  case CDIO_FS_EXT2:
    printf("CD-ROM with Linux second extended filesystem\n");
	  break;
  case CDIO_FS_3DO:
    printf("CD-ROM with Panasonic 3DO filesystem\n");
    break;
  case CDIO_FS_UNKNOWN:
    printf("CD-ROM with unknown filesystem\n");
    break;
  }
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_ISO_9660:
  case CDIO_FS_ISO_9660_INTERACTIVE:
  case CDIO_FS_ISO_HFS:
    printf("ISO 9660: %i blocks, label `%.32s'\n",
	   cdio_analysis.isofs_size, cdio_analysis.iso_label);
    break;
  }
  need_lf = 0;
  if (first_data == 1 && num_audio > 0)
    need_lf += printf("mixed mode CD   ");
  if (fs & CDIO_FS_ANAL_XA)
    need_lf += printf("XA sectors   ");
  if (fs & CDIO_FS_ANAL_MULTISESSION)
    need_lf += printf("Multisession, offset = %i   ", ms_offset);
  if (fs & CDIO_FS_ANAL_HIDDEN_TRACK)
    need_lf += printf("Hidden Track   ");
  if (fs & CDIO_FS_ANAL_PHOTO_CD)
    need_lf += printf("%sPhoto CD   ", 
		      num_audio > 0 ? " Portfolio " : "");
  if (fs & CDIO_FS_ANAL_CDTV)
    need_lf += printf("Commodore CDTV   ");
  if (first_data > 1)
    need_lf += printf("CD-Plus/Extra   ");
  if (fs & CDIO_FS_ANAL_BOOTABLE)
    need_lf += printf("bootable CD   ");
  if (fs & CDIO_FS_ANAL_VIDEOCDI && num_audio == 0) {
    need_lf += printf("Video CD   ");
#ifdef HAVE_VCDINFO
    if (!opts.no_vcd) {
      printf("\n");
      print_vcd_info();
    }
#endif    
  }
  if (fs & CDIO_FS_ANAL_SVCD)
    need_lf += printf("Super Video CD (SVCD) or Chaoji Video CD (CVD)");
  if (fs & CDIO_FS_ANAL_CVD)
    need_lf += printf("Chaoji Video CD (CVD)");
  if (need_lf) printf("\n");
}

/* ------------------------------------------------------------------------ */

int
main(int argc, const char *argv[])
{

  CdIo          *cdio=NULL;
  cdio_fs_anal_t fs=0;
  int i;
  lsn_t          start_track;          /* first sector of track */
  lsn_t          data_start =0;        /* start of data area */
  int            ms_offset = 0;
  track_t        num_tracks=0;
  track_t        first_track_num  =  0;
  unsigned int   num_audio   =  0;  /* # of audio tracks */
  unsigned int   num_data    =  0;  /* # of data tracks */
  int            first_data  = -1;  /* # of first data track */
  int            first_audio = -1;  /* # of first audio track */
  cdio_analysis_t cdio_analysis; 
      
  memset(&cdio_analysis, 0, sizeof(cdio_analysis));

  poptContext optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);

  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);
#ifdef HAVE_CDDB
  gl_default_cddb_log_handler = cddb_log_set_handler (_log_handler);
#endif

#ifdef HAVE_VCDINFO
  gl_default_vcd_log_handler = vcd_log_set_handler (_log_handler);
#endif

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? program_name+1 : strdup(argv[0]);

  /* Default option values. */
  opts.silent        = false;
  opts.no_header     = false;
  opts.debug_level   = 0;
  opts.no_tracks     = 0;
#ifdef HAVE_CDDB
  opts.no_cddb       = 0;
  opts.cddb_port     = 8880;
  opts.cddb_http     = 0;
  opts.cddb_cachedir = NULL;
  opts.cddb_server   = NULL;
  opts.cddb_timeout = -1;
  opts.cddb_disable_cache = false;
  
#endif
  opts.no_ioctl      = 0;
  opts.no_analysis   = 0;
  opts.source_image  = IMAGE_UNKNOWN;
     

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(optCon);
     
  print_version(opts.version_only);

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

  if (cdio==NULL) {
    err_exit("%s: Error in opening device driver\n", program_name);
  } 

  if (opts.debug_level > 0) {
    printf("CD driver name: %s\n", cdio_get_driver_name(cdio));
  }
  
  if (source_name==NULL) {
    source_name=strdup(cdio_get_arg(cdio, "source"));
    if (NULL == source_name) {
      err_exit("%s: No input device given/found\n", program_name);
    }
  } 

  first_track_num = cdio_get_first_track_num(cdio);
  num_tracks      = cdio_get_num_tracks(cdio);

  if (!opts.no_tracks) {
    printf(STRONG "CD-ROM Track List (%i - %i)\n" NORMAL, 
	   first_track_num, num_tracks);

    printf("  #: MSF       LSN     Type\n");
  }
  
  /* Read and possibly print track information. */
  for (i = first_track_num; i <= CDIO_CDROM_LEADOUT_TRACK; i++) {
    msf_t msf;
    
    if (!cdio_get_track_msf(cdio, i, &msf)) {
      err_exit("cdio_track_msf for track %i failed, I give up.\n", i);
    }

    if (i == CDIO_CDROM_LEADOUT_TRACK) {
      if (!opts.no_tracks)
	printf("%3d: %2.2x:%2.2x:%2.2x  %06d  leadout\n",
	       (int) i, 
	       msf.m, msf.s, msf.f, 
	       cdio_msf_to_lsn(&msf));
      break;
    } else if (!opts.no_tracks) {
      printf("%3d: %2.2x:%2.2x:%2.2x  %06d  %s\n",
	     (int) i, 
	     msf.m, msf.s, msf.f, 
	     cdio_msf_to_lsn(&msf),
	     track_format2str[cdio_get_track_format(cdio, i)]);

    }
    
    if (TRACK_FORMAT_AUDIO == cdio_get_track_format(cdio, i)) {
      num_audio++;
      if (-1 == first_audio) first_audio = i;
    } else {
      num_data++;
      if (-1 == first_data)  first_data = i;
    }
    /* skip to leadout? */
    if (i == num_tracks) i = CDIO_CDROM_LEADOUT_TRACK-1;
  }

#if CDIO_IOCTL_FINISHED
  if (!opts.no_ioctl) {
    printf(STRONG "What ioctl's report...\n" NORMAL);
  
#ifdef CDROM_GET_MCN
    /* get mcn */
    printf("Get MCN     : "); fflush(stdout);
    if (ioctl(filehandle,CDROM_GET_MCN, &mcn))
      printf("FAILED\n");
    else
      printf("%s\n",mcn.medium_catalog_number);
#endif
    
#ifdef CDROM_DISC_STATUS
    /* get disk status */
    printf("disc status : "); fflush(stdout);
    switch (ioctl(filehandle,CDROM_DISC_STATUS,0)) {
    case CDS_NO_INFO: printf("no info\n"); break;
    case CDS_NO_DISC: printf("no disc\n"); break;
    case CDS_AUDIO:   printf("audio\n"); break;
    case CDS_DATA_1:  printf("data mode 1\n"); break;
    case CDS_DATA_2:  printf("data mode 2\n"); break;
    case CDS_XA_2_1:  printf("XA mode 1\n"); break;
    case CDS_XA_2_2:  printf("XA mode 2\n"); break;
    default:          printf("unknown (failed?)\n");
    }
#endif 
    
#ifdef CDROMMULTISESSION
    /* get multisession */
    printf("multisession: "); fflush(stdout);
    ms.addr_format = CDROM_LBA;
    if (ioctl(filehandle,CDROMMULTISESSION,&ms))
      printf("FAILED\n");
    else
      printf("%d%s\n",ms.addr.lba,ms.xa_flag?" XA":"");
#endif 
    
#ifdef CDROMSUBCHNL
    /* get audio status from subchnl */
    printf("audio status: "); fflush(stdout);
    sub.cdsc_format = CDROM_MSF;
    if (ioctl(filehandle,CDROMSUBCHNL,&sub))
      printf("FAILED\n");
    else {
      switch (sub.cdsc_audiostatus) {
      case CDROM_AUDIO_INVALID:    printf("invalid\n");   break;
      case CDROM_AUDIO_PLAY:       printf("playing");     break;
      case CDROM_AUDIO_PAUSED:     printf("paused");      break;
      case CDROM_AUDIO_COMPLETED:  printf("completed\n"); break;
      case CDROM_AUDIO_ERROR:      printf("error\n");     break;
      case CDROM_AUDIO_NO_STATUS:  printf("no status\n"); break;
      default:                     printf("Oops: unknown\n");
      }
      if (sub.cdsc_audiostatus == CDROM_AUDIO_PLAY ||
	  sub.cdsc_audiostatus == CDROM_AUDIO_PAUSED) {
	printf(" at: %02d:%02d abs / %02d:%02d track %d\n",
	       sub.cdsc_absaddr.msf.minute,
	       sub.cdsc_absaddr.msf.second,
	       sub.cdsc_reladdr.msf.minute,
	       sub.cdsc_reladdr.msf.second,
	       sub.cdsc_trk);
      }
    }
#endif  /* CDROMSUBCHNL */
  }
#endif /*CDIO_IOCTL_FINISHED*/
  
  if (!opts.no_analysis) {
    printf(STRONG "CD Analysis Report\n" NORMAL);
    
    /* try to find out what sort of CD we have */
    if (0 == num_data) {
      /* no data track, may be a "real" audio CD or hidden track CD */
      
      msf_t msf;
      cdio_get_track_msf(cdio, 1, &msf);
      start_track = cdio_msf_to_lsn(&msf);
      
      /* CD-I/Ready says start_track <= 30*75 then CDDA */
      if (start_track > 100 /* 100 is just a guess */) {
	fs = cdio_guess_cd_type(cdio, 0, 1, &cdio_analysis);
	if ((CDIO_FSTYPE(fs)) != CDIO_FS_UNKNOWN)
	  fs |= CDIO_FS_ANAL_HIDDEN_TRACK;
	else {
	  fs &= ~CDIO_FS_MASK; /* del filesystem info */
	  printf("Oops: %i unused sectors at start, "
		 "but hidden track check failed.\n",
		 start_track);
	}
      }
      print_analysis(ms_offset, cdio_analysis, fs, first_data, num_audio,
		     num_tracks, first_track_num, cdio);
    } else {
      /* we have data track(s) */
      int j;

      for (j = 2, i = first_data; i <= num_tracks; i++) {
	msf_t msf;
	track_format_t track_format = cdio_get_track_format(cdio, i);
      
	cdio_get_track_msf(cdio, i, &msf);

	switch ( track_format ) {
	case TRACK_FORMAT_AUDIO:
	case TRACK_FORMAT_ERROR:
	  break;
	case TRACK_FORMAT_CDI:
	case TRACK_FORMAT_XA:
	case TRACK_FORMAT_DATA: 
	case TRACK_FORMAT_PSX: 
	  ;
	}
	
	start_track = (i == 1) ? 0 : cdio_msf_to_lsn(&msf);
	
	/* save the start of the data area */
	if (i == first_data) 
	  data_start = start_track;
	
	/* skip tracks which belong to the current walked session */
	if (start_track < data_start + cdio_analysis.isofs_size)
	  continue;
	
	fs = cdio_guess_cd_type(cdio, start_track, i, &cdio_analysis);

	if (i > 1) {
	  /* track is beyond last session -> new session found */
	  ms_offset = start_track;
	  printf("session #%d starts at track %2i, LSN: %6i,"
		 " ISO 9660 blocks: %6i\n",
		 j++, i, start_track, cdio_analysis.isofs_size);
	  printf("ISO 9660: %i blocks, label `%.32s'\n",
		 cdio_analysis.isofs_size, cdio_analysis.iso_label);
	  fs |= CDIO_FS_ANAL_MULTISESSION;
	} else {
	  print_analysis(ms_offset, cdio_analysis, fs, first_data, num_audio,
			 num_tracks, first_track_num, cdio);
	}

	if ( !(CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660 ||
	       CDIO_FSTYPE(fs) == CDIO_FS_ISO_HFS  ||
	       /* CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660_INTERACTIVE) 
		  && (fs & XA))) */
	       CDIO_FSTYPE(fs) == CDIO_FS_ISO_9660_INTERACTIVE) )
	  /* no method for non-ISO9660 multisessions */
	  break;
      }
    }
  }

  myexit(cdio, EXIT_SUCCESS);
  /* Not reached:*/
  return(EXIT_SUCCESS);
}
