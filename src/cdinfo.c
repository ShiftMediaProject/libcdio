/*
    $Id: cdinfo.c,v 1.5 2003/04/06 06:46:06 rocky Exp $

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
 
  usage: cdinfo  [options] [ dev ]
 
*/
#define PROGRAM_NAME "CD Info"
#define CDINFO_VERSION "2.0"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <popt.h>
/* Accomodate to older popt that doesn't support the "optional" flag */
#ifndef POPT_ARGFLAG_OPTIONAL
#define POPT_ARGFLAG_OPTIONAL 0
#endif

#include <fcntl.h>
#include <sys/ioctl.h>
#ifdef __linux__
# include <linux/version.h>
# include <linux/cdrom.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,50)
#  include <linux/ucdrom.h>
# endif
#endif
 
#include <sys/ioctl.h>
#include <errno.h>

#include "config.h"
#include "cdio.h"
#include "logging.h"
#include "util.h"

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
  myexit(EXIT_FAILURE)		     
  
/*
Subject:   -65- How can I read an IRIX (EFS) CD-ROM on a machine which
                doesn't use EFS?
Date: 18 Jun 1995 00:00:01 EST

  You want 'efslook', at
  ftp://viz.tamu.edu/pub/sgi/software/efslook.tar.gz.

and
! Robert E. Seastrom <rs@access4.digex.net>'s software (with source
! code) for using an SGI CD-ROM on a Macintosh is at
! ftp://bifrost.seastrom.com/pub/mac/CDROM-Jumpstart.sit151.hqx.

*/

#define FS_NO_DATA              0   /* audio only */
#define FS_HIGH_SIERRA		1
#define FS_ISO_9660		2
#define FS_INTERACTIVE		3
#define FS_HFS			4
#define FS_UFS			5
#define FS_EXT2			6
#define FS_ISO_HFS              7  /* both hfs & isofs filesystem */
#define FS_ISO_9660_INTERACTIVE 8  /* both CD-RTOS and isofs filesystem */
#define FS_3DO			9
#define FS_UNKNOWN	       15
#define FS_MASK		       15

#define XA		       16
#define MULTISESSION	       32
#define PHOTO_CD	       64
#define HIDDEN_TRACK          128
#define CDTV		      256
#define BOOTABLE       	      512
#define VIDEOCDI       	     1024
#define ROCKRIDGE            2048
#define JOLIET               4096

char buffer[6][CD_FRAMESIZE_RAW];  /* for CD-Data */

/* Some interesting sector numbers stored in the above buffer. */
#define ISO_SUPERBLOCK_SECTOR  16  /* buffer[0] */
#define UFS_SUPERBLOCK_SECTOR   4  /* buffer[2] */
#define BOOT_SECTOR            17  /* buffer[3] */
#define VCD_INFO_SECTOR       150  /* buffer[4] */


typedef struct signature
{
  unsigned int buf_num;
  unsigned int offset;
  const char *sig_str;
  const char *description;
} signature_t;

static signature_t sigs[] =
  {
/*buffer[x] off look for     description */
    {0,     1, "CD001",      "ISO 9660"}, 
    {0,     1, "CD-I",       "CD-I"}, 
    {0,     8, "CDTV",       "CDTV"}, 
    {0,     8, "CD-RTOS",    "CD-RTOS"}, 
    {0,     9, "CDROM",      "HIGH SIERRA"}, 
    {0,    16, "CD-BRIDGE",  "BRIDGE"}, 
    {0,  1024, "CD-XA001",   "XA"}, 
    {1,    64, "PPPPHHHHOOOOTTTTOOOO____CCCCDDDD",  "PHOTO CD"}, 
    {1, 0x438, "\x53\xef",   "EXT2 FS"}, 
    {2,  1372, "\x54\x19\x01\x0", "UFS"}, 
    {3,     7, "EL TORITO",  "BOOTABLE"}, 
    {4,     0, "VIDEO_CD",   "VIDEO CD"}, 
    { 0 }
  };


#if 0
#define STRONG "\033[1m"
#define NORMAL "\033[0m"
#else
#define STRONG "__________________________________\n"
#define NORMAL ""
#endif

#define IS_ISOFS     0
#define IS_CD_I      1
#define IS_CDTV      2
#define IS_CD_RTOS   3
#define IS_HS        4
#define IS_BRIDGE    5
#define IS_XA        6
#define IS_PHOTO_CD  7
#define IS_EXT2      8
#define IS_UFS       9
#define IS_BOOTABLE 10
#define IS_VIDEO_CD 11

int rc;                                                      /* return code */
int i,j;                                                           /* index */
int isofs_size = 0;                                      /* size of session */
int start_track;                                   /* first sector of track */
int ms_offset;                /* multisession offset found by track-walking */
int data_start;                                       /* start of data area */
int joliet_level = 0;

CdIo *img; 
track_t num_tracks;
track_t first_track_num;

#if CDIO_IOCTL_FINISHED
struct cdrom_mcn           mcn;
struct cdrom_multisession  ms;
struct cdrom_subchnl       sub;
#endif

int                        first_data = -1;        /* # of first data track */
int                        num_data = 0;                /* # of data tracks */
int                        first_audio = -1;      /* # of first audio track */
int                        num_audio = 0;              /* # of audio tracks */


char *source_name = NULL;
char *program_name;

const char *argp_program_version     = PROGRAM_NAME CDINFO_VERSION;
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
  bool no_tracks;
  bool no_ioctl;
  bool no_analysis;
  int debug_level;
  bool silent;
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
  OP_VERSION,       OP_NOIOCTL,   OP_NOTRACKS,   OP_QUIET,
  OP_NOANALYZE
  
};

char *temp_str;

struct poptOption optionsTable[] = {
  {"debug",    'd', POPT_ARG_INT, &opts.debug_level, 0,
   "Set debugging to LEVEL"},
  
  {"quiet",    'q', POPT_ARG_NONE, &opts.silent, 0,
   "Don't produce warning output" },
  
  {"notracks", 'T', POPT_ARG_NONE, &opts.no_tracks, 0,
   "Don't show track information"},
  
  {"noanalyze",'A', POPT_ARG_NONE, &opts.no_analysis, 0,
   "Don't filesystem analysis"},
  
  {"noioctl",  'I', POPT_ARG_NONE,  &opts.no_ioctl,   0,
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

  {"nrg-file", 'N', POPT_ARG_STRING|POPT_ARGFLAG_OPTIONAL, &source_name, 
   OP_SOURCE_NRG, "set Nero CD-ROM disk image file as source", "FILE"},
  
  {"quiet", 'q', POPT_ARG_NONE, &opts.silent, 0, 
   "show only critical messages"},
  
  {"version", 'V', POPT_ARG_NONE, NULL, OP_VERSION,
   "display version and copyright information and exit"},
  POPT_AUTOHELP {NULL, 0, 0, NULL, 0}
};

#define DEV_PREFIX "/dev/"
static char *
fillout_device_name(const char *device_name) 
{
  unsigned int prefix_len=strlen(DEV_PREFIX);
  if (0 == strncmp(device_name, DEV_PREFIX, prefix_len))
    return strdup(device_name);
  else {
    char *full_device_name=malloc(strlen(device_name)+prefix_len);
    sprintf(full_device_name, DEV_PREFIX "%s", device_name);
    return full_device_name;
  }
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
print_version (void)
{
  printf( _("CD Info %s | (c) 2003 Gerd Knorr, Heiko Eiﬂfeldt & R. Bernstein\n\
This is free software; see the source for copying conditions.\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
"),
	 CDINFO_VERSION);
}

/* ------------------------------------------------------------------------ */
/* some ISO 9660 fiddling                                                   */

static int 
read_block(int superblock, uint32_t offset, uint8_t bufnum, track_t track_num)
{
  unsigned int track_sec_count = cdio_get_track_sec_count(img, track_num);
  memset(buffer[bufnum],0,M2F1_SECTOR_SIZE);

  if ( track_sec_count < superblock) {
    dbg_print(1, "reading block %u skipped track %d has only %u sectors\n", 
	      superblock, track_num, track_sec_count);
    return -1;
  }
  
  dbg_print(2, "about to read sector %u\n", offset+superblock);

  if (0 > cdio_lseek(img, FORM1_DATA_SIZE*(offset+superblock),SEEK_SET))
    return -1;
  

  if (0 > cdio_read(img, buffer[bufnum], FORM1_DATA_SIZE))
    return -1;

  return 0;
}

static bool 
is_it(int num) 
{
  signature_t *sigp;
  int len;

  /* TODO: check that num < largest sig. */
  sigp = &sigs[num];

  len = strlen(sigp->sig_str);
  return 0 == memcmp(&buffer[sigp->buf_num][sigp->offset], 
		     sigp->sig_str, len);
}

static int 
is_hfs(void)
{
  return (0 == memcmp(&buffer[1][512],"PM",2)) ||
    (0 == memcmp(&buffer[1][512],"TS",2)) ||
    (0 == memcmp(&buffer[1][1024], "BD",2));
}

static int 
is_3do(void)
{
  return (0 == memcmp(&buffer[1][0],"\x01\x5a\x5a\x5a\x5a\x5a\x01", 7)) &&
    (0 == memcmp(&buffer[1][40], "CD-ROM", 6));
}

static int is_joliet(void)
{
  return 2 == buffer[3][0] && buffer[3][88] == 0x25 && buffer[3][89] == 0x2f;
}

/* ISO 9660 volume space in M2F1_SECTOR_SIZE byte units */
static int 
get_size(void)
{
  return ((buffer[0][80] & 0xff) |
	  ((buffer[0][81] & 0xff) << 8) |
	  ((buffer[0][82] & 0xff) << 16) |
	  ((buffer[0][83] & 0xff) << 24));
}

static int 
get_joliet_level( void )
{
  switch (buffer[3][90]) {
  case 0x40: return 1;
  case 0x43: return 2;
  case 0x45: return 3;
  }
  return 0;
}

#define is_it_dbg(sig) \
    if (is_it(sig)) printf("%s, ", sigs[sig].description)

static int 
guess_filesystem(int start_session, track_t track_num)
{
  int ret = 0;
  
  if (read_block(ISO_SUPERBLOCK_SECTOR, start_session, 0, track_num) < 0)
    return FS_UNKNOWN;
  
  if (opts.debug_level > 0) {
    /* buffer is defined */
    is_it_dbg(IS_CD_I);
    is_it_dbg(IS_CD_RTOS);
    is_it_dbg(IS_ISOFS);
    is_it_dbg(IS_HS);
    is_it_dbg(IS_BRIDGE);
    is_it_dbg(IS_XA);
    is_it_dbg(IS_CDTV);
    puts("");
  }
  
  /* filesystem */
  if (is_it(IS_CD_I) && is_it(IS_CD_RTOS) 
      && !is_it(IS_BRIDGE) && !is_it(IS_XA)) {
    return FS_INTERACTIVE;
  } else {	/* read sector 0 ONLY, when NO greenbook CD-I !!!! */

    if (read_block(0, start_session, 1, track_num) < 0)
      return ret;
    
    if (opts.debug_level > 0) {
      /* buffer[1] is defined */
      is_it_dbg(IS_PHOTO_CD);
      if (is_hfs()) printf("HFS, ");
      is_it_dbg(IS_EXT2);
      if (is_3do()) printf("3DO, ");
      puts("");
    }
    
    if (is_it(IS_HS))
      ret |= FS_HIGH_SIERRA;
    else if (is_it(IS_ISOFS)) {
      if (is_it(IS_CD_RTOS) && is_it(IS_BRIDGE))
	ret = FS_ISO_9660_INTERACTIVE;
      else if (is_hfs())
	ret = FS_ISO_HFS;
      else
	ret = FS_ISO_9660;
      isofs_size = get_size();
      
#if 0
      if (is_rockridge())
	ret |= ROCKRIDGE;
#endif

      if (read_block(BOOT_SECTOR, start_session, 3, track_num) < 0)
	return ret;
      
      if (opts.debug_level > 0) {
	
	/* buffer[3] is defined */
	if (is_joliet()) printf("JOLIET, ");
	puts("");
	is_it_dbg(IS_BOOTABLE);
	puts("");
      }
      
      if (is_joliet()) {
	joliet_level = get_joliet_level();
	ret |= JOLIET;
      }
      if (is_it(IS_BOOTABLE))
	ret |= BOOTABLE;
      
      if (is_it(IS_BRIDGE) && is_it(IS_XA) && is_it(IS_ISOFS) 
	  && is_it(IS_CD_RTOS) &&
	  !is_it(IS_PHOTO_CD)) {

        if (read_block(VCD_INFO_SECTOR, start_session, 4, track_num) < 0)
	  return ret;
	
	if (opts.debug_level > 0) {
	  /* buffer[4] is defined */
	  is_it_dbg(IS_VIDEO_CD);
	  puts("");
	}
	
	if (is_it(IS_VIDEO_CD)) ret |= VIDEOCDI;
      }
    } 
    else if (is_hfs())       ret |= FS_HFS;
    else if (is_it(IS_EXT2)) ret |= FS_EXT2;
    else if (is_3do())       ret |= FS_3DO;
    else {
      if (read_block(UFS_SUPERBLOCK_SECTOR, start_session, 2, track_num) < 0)
	return ret;
      
      if (opts.debug_level > 0) {
	/* buffer[2] is defined */
	is_it_dbg(IS_UFS);
	puts("");
      }
      
      if (is_it(IS_UFS)) 
	ret |= FS_UFS;
      else
	ret |= FS_UNKNOWN;
    }
  }
  
  /* other checks */
  if (is_it(IS_XA))       ret |= XA;
  if (is_it(IS_PHOTO_CD)) ret |= PHOTO_CD;
  if (is_it(IS_CDTV))     ret |= CDTV;
  return ret;
}

static void 
myexit(int rc) 
{
  cdio_destroy(img);
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
cddb_discid()
{
  int i,t,n=0;
  msf_t start_msf;
  msf_t msf;
  
  for (i = 1; i <= num_tracks; i++) {
    cdio_get_track_msf(img, i, &msf);
    n += cddb_dec_digit_sum(msf_seconds(&msf));
  }

  cdio_get_track_msf(img, 1, &start_msf);
  cdio_get_track_msf(img, CDROM_LEADOUT, &msf);
  
  t = msf_seconds(&msf) - msf_seconds(&start_msf);
  
  return ((n % 0xff) << 24 | t << 8 | num_tracks);
}


/* CDIO logging routines */

static cdio_log_handler_t gl_default_log_handler = NULL;

static void 
_log_handler (cdio_log_level_t level, const char message[])
{
  if (level == CDIO_LOG_DEBUG && opts.debug_level < 2)
    return;

  if (level == CDIO_LOG_INFO  && opts.debug_level < 1)
    return;
  
  if (level == CDIO_LOG_WARN  && opts.silent)
    return;
  
  gl_default_log_handler (level, message);
}

static void
print_analysis(int fs, int num_audio)
{
  int need_lf;
  
  switch(fs & FS_MASK) {
  case FS_NO_DATA:
    if (num_audio > 0)
      printf("Audio CD, CDDB disc ID is %08lx\n", cddb_discid());
    break;
  case FS_ISO_9660:
    printf("CD-ROM with ISO 9660 filesystem");
    if (fs & JOLIET)
      printf(" and joliet extension level %d", joliet_level);
    if (fs & ROCKRIDGE)
      printf(" and rockridge extensions");
    printf("\n");
    break;
  case FS_ISO_9660_INTERACTIVE:
    printf("CD-ROM with CD-RTOS and ISO 9660 filesystem\n");
    break;
  case FS_HIGH_SIERRA:
    printf("CD-ROM with High Sierra filesystem\n");
    break;
  case FS_INTERACTIVE:
    printf("CD-Interactive%s\n", num_audio > 0 ? "/Ready" : "");
    break;
  case FS_HFS:
    printf("CD-ROM with Macintosh HFS\n");
    break;
  case FS_ISO_HFS:
    printf("CD-ROM with both Macintosh HFS and ISO 9660 filesystem\n");
    break;
  case FS_UFS:
    printf("CD-ROM with Unix UFS\n");
    break;
  case FS_EXT2:
    printf("CD-ROM with Linux second extended filesystem\n");
	  break;
  case FS_3DO:
    printf("CD-ROM with Panasonic 3DO filesystem\n");
    break;
  case FS_UNKNOWN:
    printf("CD-ROM with unknown filesystem\n");
    break;
  }
  switch(fs & FS_MASK) {
  case FS_ISO_9660:
  case FS_ISO_9660_INTERACTIVE:
  case FS_ISO_HFS:
    printf("ISO 9660: %i blocks, label `%.32s'\n",
	   isofs_size, buffer[0]+40);
    break;
  }
  need_lf = 0;
  if (first_data == 1 && num_audio > 0)
    need_lf += printf("mixed mode CD   ");
  if (fs & XA)
    need_lf += printf("XA sectors   ");
  if (fs & MULTISESSION)
    need_lf += printf("Multisession, offset = %i   ",ms_offset);
  if (fs & HIDDEN_TRACK)
    need_lf += printf("Hidden Track   ");
  if (fs & PHOTO_CD)
    need_lf += printf("%sPhoto CD   ", num_audio > 0 ? " Portfolio " : "");
  if (fs & CDTV)
    need_lf += printf("Commodore CDTV   ");
  if (first_data > 1)
    need_lf += printf("CD-Plus/Extra   ");
  if (fs & BOOTABLE)
    need_lf += printf("bootable CD   ");
  if (fs & VIDEOCDI && num_audio == 0)
    need_lf += printf("Video CD   ");
  if (need_lf) puts("");
}


/* ------------------------------------------------------------------------ */

int
main(int argc, const char *argv[])
{

  int fs=0;
  poptContext optCon = poptGetContext (NULL, argc, argv, optionsTable, 0);

  gl_default_log_handler = cdio_log_set_handler (_log_handler);

  program_name = strrchr(argv[0],'/');
  program_name = program_name ? program_name+1 : strdup(argv[0]);

  /* Default option values. */
  opts.silent         = false;
  opts.debug_level    = 0;
  opts.no_tracks      = false;
  opts.no_ioctl       = false;
  opts.no_analysis    = false;
  opts.source_image   = IMAGE_UNKNOWN;
     

  /* Parse our arguments; every option seen by `parse_opt' will
     be reflected in `arguments'. */
  parse_options(optCon);
     
  print_version();

  switch (opts.source_image) {
  case IMAGE_UNKNOWN:
  case IMAGE_AUTO:
    img = cdio_open (source_name, DRIVER_UNKNOWN);
    break;
  case IMAGE_DEVICE:
    img = cdio_open (source_name, DRIVER_DEVICE);
    break;
  case IMAGE_BIN:
    img = cdio_open (source_name, DRIVER_BINCUE);
    break;
  case IMAGE_CUE:
    img = cdio_open_cue(source_name);
    break;
  case IMAGE_NRG:
    img = cdio_open (source_name, DRIVER_NRG);
    break;
  }

  if (source_name==NULL) {
    source_name=strdup(cdio_get_arg(img, "source"));
  } 

  first_track_num = cdio_get_first_track_num(img);
  num_tracks      = cdio_get_num_tracks(img);

  if (!opts.no_tracks) {
    printf(STRONG "CD-ROM Track List (%i - %i)\n" NORMAL, 
	   first_track_num, num_tracks);

    printf("  #: MSF       LSN     Type\n");
  }
  
  /* Read and possibly print track information. */
  for (i = first_track_num; i <= CDROM_LEADOUT; i++) {
    msf_t msf;
    
    if (!cdio_get_track_msf(img, i, &msf)) {
      err_exit("cdio_track_msf for track %i failed, I give up.\n", i);
    }

    if (i == CDROM_LEADOUT) {
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
	     track_format2str[cdio_get_track_format(img, i)]);

    }
    
    if (TRACK_FORMAT_AUDIO == cdio_get_track_format(img, i)) {
      num_audio++;
      if (-1 == first_audio)
	first_audio = i;
    } else {
      num_data++;
      if (-1 == first_data)
	first_data = i;
    }
    /* skip to leadout? */
    if (i == num_tracks) i = CDROM_LEADOUT-1;
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
    printf(STRONG "try to find out what sort of CD this is\n" NORMAL);
    
    /* try to find out what sort of CD we have */
    if (0 == num_data) {
      /* no data track, may be a "real" audio CD or hidden track CD */
      
      msf_t msf;
      cdio_get_track_msf(img, 1, &msf);
      start_track = cdio_msf_to_lsn(&msf);
      
      /* CD-I/Ready says start_track <= 30*75 then CDDA */
      if (start_track > 100 /* 100 is just a guess */) {
	fs = guess_filesystem(0, 1);
	if ((fs & FS_MASK) != FS_UNKNOWN)
	  fs |= HIDDEN_TRACK;
	else {
	  fs &= ~FS_MASK; /* del filesystem info */
	  printf("Oops: %i unused sectors at start, "
		 "but hidden track check failed.\n",
		 start_track);
	}
      }
      print_analysis(fs, num_audio);
    } else {
      /* we have data track(s) */
      
      for (j = 2, i = first_data; i <= num_tracks; i++) {
	msf_t msf;
	track_format_t track_format = cdio_get_track_format(img, i);
      
	cdio_get_track_msf(img, i, &msf);

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
	if (start_track < data_start + isofs_size)
	  continue;
	
	fs = guess_filesystem(start_track, i);

	if (i > 1) {
	  /* track is beyond last session -> new session found */
	  ms_offset = start_track;
	  printf("session #%d starts at track %2i, LSN: %6i,"
		 " ISO 9660 blocks: %6i\n",
		 j++, i, start_track, isofs_size);
	  printf("ISO 9660: %i blocks, label `%.32s'\n",
		 isofs_size, buffer[0]+40);
	  fs |= MULTISESSION;
	} else {
	  print_analysis(fs, num_audio);
	}
	
	if (!(((fs & FS_MASK) == FS_ISO_9660 ||
	       (fs & FS_MASK) == FS_ISO_HFS ||
	       /* (fs & FS_MASK) == FS_ISO_9660_INTERACTIVE) && (fs & XA))) */
	       (fs & FS_MASK) == FS_ISO_9660_INTERACTIVE)))
	  break;	/* no method for non-iso9660 multisessions */
      }
    }
  }
  
  
  myexit(EXIT_SUCCESS);
  /* Not reached:*/
  return(EXIT_SUCCESS);
}
