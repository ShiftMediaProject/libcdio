/*
    $Id: cd-info.c,v 1.71 2004/07/16 21:29:25 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 1996, 1997, 1998  Gerd Knorr <kraxel@bytesex.org>
         and Heiko Eiﬂfeldt <heiko@hexco.de>

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

#include "util.h"

#ifdef HAVE_CDDB
#include <cddb/cddb.h>
#endif

#ifdef HAVE_VCDINFO
#include <libvcd/logging.h>
#include <libvcd/files.h>
#include <libvcd/info.h>
#endif

#include <cdio/util.h>
#include <cdio/cd_types.h>
#include <cdio/cdtext.h>
#include <cdio/iso9660.h>

#include "cdio_assert.h"
#include "bytesex.h"
#include "ds.h"
#include "iso9660_private.h"

#include <fcntl.h>
#ifdef __linux__
# include <linux/version.h>
# include <linux/cdrom.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,50)
#  include <linux/ucdrom.h>
# endif
#endif
 
#include <errno.h>

#if 0
#define STRONG "\033[1m"
#define NORMAL "\033[0m"
#else
#define STRONG "__________________________________\n"
#define NORMAL ""
#endif

#if CDIO_IOCTL_FINISHED
struct cdrom_multisession  ms;
struct cdrom_subchnl       sub;
#endif

/* Used by `main' to communicate with `parse_opt'. And global options
 */
struct arguments
{
  int            no_tracks;
  int            no_ioctl;
  int            no_analysis;
  char          *access_mode; /* Access method driver should use for control */
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
  int            no_vcd;
  uint32_t       debug_level;
  int            version_only;
  int            silent;
  int            no_header;
  int            print_iso9660;
  int            list_drives;
  source_image_t source_image;
} opts;
     
/* Configuration option codes */
enum {
  
  OP_SOURCE_UNDEF,
  OP_SOURCE_AUTO,
  OP_SOURCE_BIN,
  OP_SOURCE_CUE,
  OP_SOURCE_CDRDAO,
  OP_SOURCE_NRG ,
  OP_SOURCE_DEVICE,
  
  /* These are the remaining configuration options */
  OP_VERSION,  
  
};

char *temp_str;


/* Parse a all options. */
static bool
parse_options (int argc, const char *argv[])
{
  int opt;
  char *psz_my_source;
  
  struct poptOption optionsTable[] = {
    {"access-mode",       'a', POPT_ARG_STRING, &opts.access_mode, 0,
     "Set CD access methed"},
    
    {"debug",       'd', POPT_ARG_INT, &opts.debug_level, 0,
     "Set debugging to LEVEL"},
    
    {"no-tracks",   'T', POPT_ARG_NONE, &opts.no_tracks, 0,
     "Don't show track information"},
    
    {"no-analyze",  'A', POPT_ARG_NONE, &opts.no_analysis, 0,
     "Don't filesystem analysis"},
    
#ifdef HAVE_CDDB
    {"no-cddb",     '\0', POPT_ARG_NONE, &opts.no_cddb, 0,
     "Don't look up audio CDDB information or print that"},
    
    {"cddb-port",   'P', POPT_ARG_INT, &opts.cddb_port, 8880,
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
#else 
    {"no-vcd",   'v', POPT_ARG_NONE, &opts.no_vcd, 1,
     "Don't look up Video CD information - for this build, this is always set"},
#endif
    {"no-ioctl",  'I', POPT_ARG_NONE,  &opts.no_ioctl, 0,
     "Don't show ioctl() information"},
    
    {"bin-file", 'b', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_BIN, "set \"bin\" CD-ROM disk image file as source", "FILE"},
    
    {"cue-file", 'c', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_CUE, "set \"cue\" CD-ROM disk image file as source", "FILE"},
    
    {"input", 'i', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_AUTO,
     "set source and determine if \"bin\" image or device", "FILE"},
    
    {"iso9660",  '\0', POPT_ARG_NONE, &opts.print_iso9660, 0,
     "print directory contents of any ISO-9660 filesystems"},
    
    {"cdrom-device", 'C', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
     OP_SOURCE_DEVICE,
     "set CD-ROM device as source", "DEVICE"},
    
    {"list-drives",   'l', POPT_ARG_NONE, &opts.list_drives, 0,
     "Give a list of CD-drives" },
    
    {"no-header", '\0', POPT_ARG_NONE, &opts.no_header, 
     0, "Don't display header and copyright (for regression testing)"},
    
    {"nrg-file", 'N', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_NRG, "set Nero CD-ROM disk image file as source", "FILE"},
    
    {"toc-file", 't', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &psz_my_source, 
     OP_SOURCE_CDRDAO, "set cdrdao CD-ROM disk image file as source", "FILE"},
    
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
      
    case OP_SOURCE_AUTO:
    case OP_SOURCE_BIN: 
    case OP_SOURCE_CUE: 
    case OP_SOURCE_CDRDAO: 
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

      /* For all input sources which are not a DEVICE, we need to make
	 a copy of the string; for a DEVICE the fill-out routine makes
	 the copy.
       */
      if (OP_SOURCE_DEVICE != opt) 
	source_name = strdup(psz_my_source);
      
      switch (opt) {
      case OP_SOURCE_BIN: 
	opts.source_image  = IMAGE_BIN;
	break;
      case OP_SOURCE_CUE: 
	opts.source_image  = IMAGE_CUE;
	break;
      case OP_SOURCE_CDRDAO: 
	opts.source_image  = IMAGE_CDRDAO;
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
      poptFreeContext(optCon);
      return false;
    }
  }
  {
    const char *remaining_arg = poptGetArg(optCon);
    if ( remaining_arg != NULL) {
      if (opts.source_image != IMAGE_UNKNOWN) {
	fprintf (stderr, 
		 "%s: Source '%s' given as an argument of an option and as "
		 "unnamed option '%s'\n", 
		 program_name, psz_my_source, remaining_arg);
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
		 program_name, psz_my_source, remaining_arg);
	poptFreeContext(optCon);
	free(program_name);
	exit (EXIT_FAILURE);
	
      }
    }
  }
  
  poptFreeContext(optCon);
  return true;
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

static void 
print_cdtext_info(CdIo *cdio) {
    const cdtext_t *cdtext = cdio_get_cdtext(cdio);

    if (NULL != cdtext) {
      cdtext_field_t i;
      
      printf("\nCD-TEXT info:\n");

      for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
	if (cdtext->field[i]) {
	  printf("\t%s: %s\n", cdtext_field2str(i), cdtext->field[i]);
	}
      }
    } else {
      printf("Didn't get CD-TEXT info.\n");
    }
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
    printf("%s: %s\n", program_name, cddb_error_str(cddb_errno(conn)));
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
print_vcd_info(driver_id_t driver) {
  vcdinfo_open_return_t open_rc;
  vcdinfo_obj_t *obj;
  open_rc = vcdinfo_open(&obj, &source_name, driver, NULL);
  switch (open_rc) {
  case VCDINFO_OPEN_VCD: 
    if (vcdinfo_get_format_version (obj) == VCD_TYPE_INVALID) {
      fprintf(stderr, "VCD format detection failed");
      vcdinfo_close(obj);
      return;
    }
    fprintf (stdout, "Format      : %s\n", 
	     vcdinfo_get_format_version_str(obj));
    fprintf (stdout, "Album       : `%.16s'\n",  vcdinfo_get_album_id(obj));
    fprintf (stdout, "Volume count: %d\n",   vcdinfo_get_volume_count(obj));
    fprintf (stdout, "volume number: %d\n",  vcdinfo_get_volume_num(obj));

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
print_iso9660_recurse (const CdIo *cdio, const char pathname[], 
		       cdio_fs_anal_t fs, 
		       bool b_mode2)
{
  CdioList *entlist;
  CdioList *dirlist =  _cdio_list_new ();
  CdioListNode *entnode;

  entlist = iso9660_fs_readdir (cdio, pathname, b_mode2);
    
  printf ("%s:\n", pathname);

  if (NULL == entlist) {
    fprintf (stderr, "Error getting above directory information\n");
    return;
  }

  /* Iterate over files in this directory */
  
  _CDIO_LIST_FOREACH (entnode, entlist)
    {
      iso9660_stat_t *statbuf = _cdio_list_node_data (entnode);
      char *iso_name = statbuf->filename;
      char _fullname[4096] = { 0, };
      char translated_name[MAX_ISONAME+1];
#define DATESTR_SIZE 30
      char date_str[DATESTR_SIZE];

      iso9660_name_translate(iso_name, translated_name);
      
      snprintf (_fullname, sizeof (_fullname), "%s%s", pathname, 
		iso_name);
  
      strncat (_fullname, "/", sizeof (_fullname));

      if (statbuf->type == _STAT_DIR
          && strcmp (iso_name, ".") 
          && strcmp (iso_name, ".."))
        _cdio_list_append (dirlist, strdup (_fullname));

      if (fs & CDIO_FS_ANAL_XA) {
	printf ( "  %c %s %d %d [fn %.2d] [LSN %6lu] ",
		 (statbuf->type == _STAT_DIR) ? 'd' : '-',
		 iso9660_get_xa_attr_str (statbuf->xa.attributes),
		 uint16_from_be (statbuf->xa.user_id),
		 uint16_from_be (statbuf->xa.group_id),
		 statbuf->xa.filenum,
		 (long unsigned int) statbuf->lsn);
	
	if (uint16_from_be(statbuf->xa.attributes) & XA_ATTR_MODE2FORM2) {
	  printf ("%9u (%9u)",
		  (unsigned int) statbuf->secsize * M2F2_SECTOR_SIZE,
		  (unsigned int) statbuf->size);
	} else {
	  printf ("%9u", (unsigned int) statbuf->size);
	}
      }
      strftime(date_str, DATESTR_SIZE, "%b %d %Y %H:%M ", &statbuf->tm);
      printf (" %s %s\n", date_str, translated_name);
    }

  _cdio_list_free (entlist, true);

  printf ("\n");

  /* Now recurse over the directories. */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);

      print_iso9660_recurse (cdio, _fullname, fs, b_mode2);
    }

  _cdio_list_free (dirlist, true);
}

static bool
read_iso9660_pvd(const CdIo *p_cdio, track_format_t track_format, /*out*/
		 iso9660_pvd_t *p_pvd) 		 {
  
  switch (track_format) {
  case TRACK_FORMAT_CDI:
  case TRACK_FORMAT_XA:
    if (0 != cdio_read_mode2_sector (p_cdio, p_pvd, ISO_PVD_SECTOR, false))
      return false;
    break;
  case TRACK_FORMAT_DATA:
    if (0 != cdio_read_mode1_sector (p_cdio, p_pvd, ISO_PVD_SECTOR, false))
      return false;
    break;
  case TRACK_FORMAT_AUDIO: 
  case TRACK_FORMAT_PSX: 
  case TRACK_FORMAT_ERROR: 
  default:
    return false;
  }
  return true;
}
		 

static void
print_iso9660_fs (const CdIo *p_cdio, cdio_fs_anal_t fs, 
		  track_format_t track_format)
{
  iso9660_pvd_t pvd;
  bool b_mode2 = false;

  if (fs & CDIO_FS_ANAL_XA) track_format = TRACK_FORMAT_XA;

  if ( !read_iso9660_pvd(p_cdio, track_format, &pvd) ) 
    return;
  
  b_mode2 = ( TRACK_FORMAT_CDI == track_format 
	       || TRACK_FORMAT_XA == track_format );
  
  {
    const lsn_t extent = iso9660_get_root_lsn(&pvd);
    
    printf ("ISO9660 filesystem\n");
    printf (" root dir in PVD set to lsn %lu\n\n", (long unsigned) extent);
    
    print_iso9660_recurse (p_cdio, "/", fs, b_mode2);
  }
}

static void
print_analysis(int ms_offset, cdio_iso_analysis_t cdio_iso_analysis, 
	       cdio_fs_anal_t fs, int first_data, unsigned int num_audio, 
	       track_t num_tracks, track_t first_track_num, 
	       track_format_t track_format, CdIo *p_cdio)
{
  int need_lf;
  
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_AUDIO:
    if (num_audio > 0) {
      printf("Audio CD, CDDB disc ID is %08lx\n", 
	     cddb_discid(p_cdio, num_tracks));
#ifdef HAVE_CDDB
      if (!opts.no_cddb) print_cddb_info(p_cdio, num_tracks, first_track_num);
#endif      
      print_cdtext_info(p_cdio);
    }
    break;
  case CDIO_FS_ISO_9660:
    printf("CD-ROM with ISO 9660 filesystem");
    if (fs & CDIO_FS_ANAL_JOLIET) {
      printf(" and joliet extension level %d", cdio_iso_analysis.joliet_level);
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
    printf("CD-ROM with GNU/Linux EXT2 (native) filesystem\n");
	  break;
  case CDIO_FS_3DO:
    printf("CD-ROM with Panasonic 3DO filesystem\n");
    break;
  case CDIO_FS_UDFX:
    printf("CD-ROM with UDFX filesystem\n");
    break;
  case CDIO_FS_UNKNOWN:
    printf("CD-ROM with unknown filesystem\n");
    break;
  case CDIO_FS_XISO:
    printf("CD-ROM with Microsoft X-BOX XISO filesystem\n");
    break;
  }
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_ISO_9660:
  case CDIO_FS_ISO_9660_INTERACTIVE:
  case CDIO_FS_ISO_HFS:
  case CDIO_FS_ISO_UDF:
    printf("ISO 9660: %i blocks, label `%.32s'\n",
	   cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);

    {
      iso9660_pvd_t pvd;

      if ( read_iso9660_pvd(p_cdio, track_format, &pvd) ) {
	fprintf(stdout, "Application: %s\n", 
		iso9660_get_application_id(&pvd));
	fprintf(stdout, "Preparer   : %s\n", iso9660_get_preparer_id(&pvd));
	fprintf(stdout, "Publisher  : %s\n", iso9660_get_publisher_id(&pvd));
	fprintf(stdout, "System     : %s\n", iso9660_get_system_id(&pvd));
	fprintf(stdout, "Volume     : %s\n", iso9660_get_volume_id(&pvd));
	fprintf(stdout, "Volume Set : %s\n", iso9660_get_volumeset_id(&pvd));
      }
      
    }
    
    if (opts.print_iso9660) 
      print_iso9660_fs(p_cdio, fs, track_format);
    
    break;
  }
  
  switch(CDIO_FSTYPE(fs)) {
  case CDIO_FS_UDF:
  case CDIO_FS_ISO_UDF:
    fprintf(stdout, "UDF: version %x.%2.2x\n",
	    cdio_iso_analysis.UDFVerMajor, cdio_iso_analysis.UDFVerMinor);
    break;
  default: ;
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
  if (fs & CDIO_FS_ANAL_VIDEOCD && num_audio == 0) {
    need_lf += printf("Video CD   ");
  }
  if (fs & CDIO_FS_ANAL_SVCD)
    need_lf += printf("Super Video CD (SVCD) or Chaoji Video CD (CVD)");
  if (fs & CDIO_FS_ANAL_CVD)
    need_lf += printf("Chaoji Video CD (CVD)");
  if (need_lf) printf("\n");
#ifdef HAVE_VCDINFO
  if (fs & (CDIO_FS_ANAL_VIDEOCD|CDIO_FS_ANAL_CVD|CDIO_FS_ANAL_SVCD)) 
    if (!opts.no_vcd) {
      printf("\n");
      print_vcd_info(cdio_get_driver_id(p_cdio));
    }
#endif    

}

/* Initialize global variables. */
static void 
init(void) 
{
  gl_default_cdio_log_handler = cdio_log_set_handler (_log_handler);
#ifdef HAVE_CDDB
  gl_default_cddb_log_handler = cddb_log_set_handler (_log_handler);
#endif

#ifdef HAVE_VCDINFO
  gl_default_vcd_log_handler = vcd_log_set_handler (_log_handler);
#endif

  /* Default option values. */
  opts.silent        = false;
  opts.list_drives   = false;
  opts.no_header     = false;
  opts.debug_level   = 0;
  opts.no_tracks     = 0;
  opts.print_iso9660 = 0;
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
  opts.access_mode   = NULL;
}

/* ------------------------------------------------------------------------ */

int
main(int argc, const char *argv[])
{

  CdIo               *cdio=NULL;
  cdio_fs_anal_t      fs=CDIO_FS_AUDIO;
  int i;
  lsn_t               start_track_lsn;      /* lsn of first track */
  lsn_t               data_start =0;        /* start of data area */
  int                 ms_offset = 0;
  track_t             num_tracks=0;
  track_t             first_track_num  =  0;
  unsigned int        num_audio   =  0;  /* # of audio tracks */
  unsigned int        num_data    =  0;  /* # of data tracks */
  int                 first_data  = -1;  /* # of first data track */
  int                 first_audio = -1;  /* # of first audio track */
  cdio_iso_analysis_t cdio_iso_analysis; 
  char                *media_catalog_number;  
      
  memset(&cdio_iso_analysis, 0, sizeof(cdio_iso_analysis));
  init();

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(argc, argv);
     
  print_version(program_name, VERSION, opts.no_header, opts.version_only);

  if (opts.debug_level == 3) {
    cdio_loglevel_default = CDIO_LOG_INFO;
#ifdef HAVE_VCDINFO
    vcd_loglevel_default = VCD_LOG_INFO;
#endif
  } else if (opts.debug_level >= 4) {
    cdio_loglevel_default = CDIO_LOG_DEBUG;
#ifdef HAVE_VCDINFO
    vcd_loglevel_default = VCD_LOG_DEBUG;
#endif
  }

  switch (opts.source_image) {
  case IMAGE_UNKNOWN:
  case IMAGE_AUTO:
    cdio = cdio_open_am (source_name, DRIVER_UNKNOWN, opts.access_mode);
    if (cdio==NULL) {
      if (source_name) {
	err_exit("%s: Error in automatically selecting driver for input %s.\n", 
		 program_name, source_name);
      } else {
	err_exit("%s: Error in automatically selecting driver.\n", 
		 program_name);
      }
    } 
    break;
  case IMAGE_DEVICE:
    cdio = cdio_open_am (source_name, DRIVER_DEVICE, opts.access_mode);
    if (cdio==NULL) {
      if (source_name) {
	err_exit("%s: Error in automatically selecting CD-image driver for input %s\n", 
	       program_name, source_name);
      } else {
	err_exit("%s: Error in automatically selecting CD-image driver.\n", 
	       program_name);
      }
    } 
    break;
  case IMAGE_BIN:
    cdio = cdio_open_am (source_name, DRIVER_BINCUE, opts.access_mode);
    if (cdio==NULL) {
      if (source_name) {
	err_exit("%s: Error in opening bin/cue for input %s\n", 
		 program_name, source_name);
      } else {
	err_exit("%s: Error in opening bin/cue for unspecified input.\n", 
		 program_name);
      }
    } 
    break;
  case IMAGE_CUE:
    cdio = cdio_open_cue(source_name);
    if (cdio==NULL) {
      if (source_name) {
	err_exit("%s: Error in opening cue/bin with input %s\n", 
		 program_name, source_name);
      } else {
	err_exit("%s: Error in opening cue/bin for unspeicified input.\n", 
		 program_name);
      }
    } 
    break;
  case IMAGE_NRG:
    cdio = cdio_open_am (source_name, DRIVER_NRG, opts.access_mode);
    if (cdio==NULL) {
      if (source_name) {
	err_exit("%s: Error in opening NRG with input %s\n", 
		 program_name, source_name);
      } else {
	err_exit("%s: Error in opening NRG for unspecified input.\n", 
		 program_name);
      }
    } 
    break;

  case IMAGE_CDRDAO:
    cdio = cdio_open_am (source_name, DRIVER_CDRDAO, opts.access_mode);
    if (cdio==NULL) {
      if (source_name) {
	err_exit("%s: Error in opening TOC with input %s.\n", 
		 program_name, source_name);
      } else {
	err_exit("%s: Error in opening TOC for unspecified input.\n", 
		 program_name);
      }
    }
    break;
  }

  if (cdio==NULL) {
    if (source_name) {
      err_exit("%s: Error in opening device driver for input %s.\n", 
	       program_name, source_name);
    } else {
      err_exit("%s: Error in opening device driver for unspecified input.\n", 
	       program_name);
    }
    
  } 

  if (source_name==NULL) {
    source_name=strdup(cdio_get_arg(cdio, "source"));
    if (NULL == source_name) {
      err_exit("%s: No input device given/found\n", program_name);
    }
  } 

  if (opts.silent == 0) {
    printf("CD location   : %s\n",   source_name);
    printf("CD driver name: %s\n",   cdio_get_driver_name(cdio));
    printf("   access mode: %s\n\n", cdio_get_arg(cdio, "access-mode"));
  } 

  {
    cdio_drive_cap_t i_drive_cap =  cdio_get_drive_cap(cdio);
    print_drive_capabilities(i_drive_cap);
  }

  if (opts.list_drives) {
    char ** device_list = cdio_get_devices(DRIVER_DEVICE);
    char ** d = device_list;

    printf("list of devices found:\n");
    if (NULL != d) {
      for ( ; *d != NULL ; d++ ) {
	printf("%s\n", *d);
      }
    }
    cdio_free_device_list(device_list);
    if (device_list) free(device_list);
  }

  first_track_num = cdio_get_first_track_num(cdio);
  num_tracks      = cdio_get_num_tracks(cdio);

  if (!opts.no_tracks) {
    printf(STRONG "CD-ROM Track List (%i - %i)\n" NORMAL, 
	   first_track_num, num_tracks);

    printf("  #: MSF       LSN     Type  Green?\n");
  }
  
  /* Read and possibly print track information. */
  for (i = first_track_num; i <= CDIO_CDROM_LEADOUT_TRACK; i++) {
    msf_t msf;
    char *psz_msf;

    if (!cdio_get_track_msf(cdio, i, &msf)) {
      err_exit("cdio_track_msf for track %i failed, I give up.\n", i);
    }

    psz_msf = cdio_msf_to_str(&msf);
    if (i == CDIO_CDROM_LEADOUT_TRACK) {
      if (!opts.no_tracks)
	printf("%3d: %8s  %06lu  leadout\n", (int) i, psz_msf,
	       (long unsigned int) cdio_msf_to_lsn(&msf));
      break;
    } else if (!opts.no_tracks) {
      printf("%3d: %8s  %06lu  %-5s %s\n", (int) i, psz_msf,
	     (long unsigned int) cdio_msf_to_lsn(&msf),
	     track_format2str[cdio_get_track_format(cdio, i)],
	     cdio_get_track_green(cdio, i)? "true" : "false");
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

  /* get MCN */
  media_catalog_number = cdio_get_mcn(cdio);
  printf("Media Catalog Number (MCN): "); fflush(stdout);
  if (NULL == media_catalog_number)
    printf("not available\n");
  else {
    printf("%s\n", media_catalog_number);
    free(media_catalog_number);
  }
  
    
#if CDIO_IOCTL_FINISHED
  if (!opts.no_ioctl) {
    printf(STRONG "What ioctl's report...\n" NORMAL);
  
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
      start_track_lsn = cdio_msf_to_lsn(&msf);
      
      /* CD-I/Ready says start_track_lsn <= 30*75 then CDDA */
      if (start_track_lsn > 100 /* 100 is just a guess */) {
	fs = cdio_guess_cd_type(cdio, 0, 1, &cdio_iso_analysis);
	if ((CDIO_FSTYPE(fs)) != CDIO_FS_UNKNOWN)
	  fs |= CDIO_FS_ANAL_HIDDEN_TRACK;
	else {
	  fs &= ~CDIO_FS_MASK; /* del filesystem info */
	  printf("Oops: %lu unused sectors at start, "
		 "but hidden track check failed.\n",
		 (long unsigned int) start_track_lsn);
	}
      }
      print_analysis(ms_offset, cdio_iso_analysis, fs, first_data, num_audio,
		     num_tracks, first_track_num, 
		     cdio_get_track_format(cdio, 1), cdio);
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
	
	start_track_lsn = (i == 1) ? 0 : cdio_msf_to_lsn(&msf);
	
	/* save the start of the data area */
	if (i == first_data) 
	  data_start = start_track_lsn;
	
	/* skip tracks which belong to the current walked session */
	if (start_track_lsn < data_start + cdio_iso_analysis.isofs_size)
	  continue;
	
	fs = cdio_guess_cd_type(cdio, start_track_lsn, i, &cdio_iso_analysis);

	if (i > 1) {
	  /* track is beyond last session -> new session found */
	  ms_offset = start_track_lsn;
	  printf("session #%d starts at track %2i, LSN: %lu,"
		 " ISO 9660 blocks: %6i\n",
		 j++, i, (unsigned long int) start_track_lsn, 
		 cdio_iso_analysis.isofs_size);
	  printf("ISO 9660: %i blocks, label `%.32s'\n",
		 cdio_iso_analysis.isofs_size, cdio_iso_analysis.iso_label);
	  fs |= CDIO_FS_ANAL_MULTISESSION;
	} else {
	  print_analysis(ms_offset, cdio_iso_analysis, fs, first_data, 
			 num_audio, num_tracks, first_track_num, 
			 track_format, cdio);
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
