/*
    $Id: cdda-player.c,v 1.3 2005/03/09 02:19:54 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

    Adapted from Gerd Knorr's player.c program  <kraxel@bytesex.org>
    Copyright (C) 1997, 1998 

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <sys/time.h>

#ifdef HAVE_CDDB
#include <cddb/cddb.h>
#endif

#include <signal.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <curses.h>

#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <cdio/util.h>
#include <cdio/cd_types.h>

#include "cddb.h"

static void play_track(track_t t1, track_t t2);
static void display_cdinfo(CdIo_t *p_cdio, track_t i_tracks, 
			   track_t i_first_track);
static bool init_cddb(CdIo_t *p_cdio, track_t i_tracks);
static void get_cddb_track_info(track_t i_track);


CdIo_t             *p_cdio;               /* libcdio handle */
driver_id_t        driver_id = DRIVER_DEVICE;
int b_sig = false;                                /* set on some signals */

/* cdrom data */
track_t            i_first_track;
track_t            i_last_track;
track_t            i_first_audio_track;
track_t            i_last_audio_track;
track_t            i_tracks;
msf_t              toc[CDIO_CDROM_LEADOUT_TRACK+1];
cdio_subchannel_t  sub;      /* subchannel last time read */
int                data;     /* data tracks present ? */
bool               b_cd = false;
bool               auto_mode = false;
int                start_track = 0;
int                stop_track = 0;
int                one_track = 0;
bool               b_verbose = false;
bool               debug = false;
bool               interactive = true;
int                todo; /* non-interactive */
bool               have_wdb = false;    /* wdb database present */
bool               b_cddb = false;   /* cddb datebase present */
bool               b_cddb_l = false; /* local cddb */
bool               b_cddb_r = false; /* remote cddb */
bool               b_db     = false; /* we have a database at all */
bool               b_record = false; /* we have a record for
					the inserted CD */

typedef enum {
  PLAY_CD=1,
  PLAY_TRACK=2,
  STOP_PLAYING=3,
  EJECT_CD=4,
  CLOSE_CD=5,
  LIST_TRACKS=6,
  PS_LIST_TRACKS=7
} cd_operation_t;

char *psz_device=NULL;
char *psz_program;

struct opts_s
{
  char          *cddb_email;  /* email to report to CDDB server. */
  char          *cddb_server; /* CDDB server to contact */
  int            cddb_port;   /* port number to contact CDDB server. */
  int            cddb_http;   /* 1 if use http proxy */
  int            cddb_timeout;
  bool           cddb_disable_cache; /* If set the below is meaningless. */
  char          *cddb_cachedir;
} opts;

/* Info about songs and titles. The 0 entry will contain the disc info.
 */
typedef struct 
{
  char title[80];
  char artist[80];
  char ext_data[80];
} cd_track_info_rec_t;

cd_track_info_rec_t cd_info[CDIO_CD_MAX_TRACKS+2];

char title[80];
char artist[80];
char genre[40];
char category[40];
char year[5];

#ifdef HAVE_CDDB
cddb_conn_t *p_conn = NULL;
cddb_disc_t *p_cddb_disc = NULL;
int i_cddb_matches = 0;
#endif

const char key_bindings[] =
"    right     play / next track\n"
"    left      previous track\n"
"    up        10 sec forward\n"
"    down      10 sec back\n"
"    1-9       jump to track 1-9\n"
"    0         jump to track 10\n"
"    F1-F20    jump to track 11-30\n"
"\n"
"    e         eject\n"
"    c         close tray\n"
"    p, space  pause / resume\n"
"    s         stop\n"
"    q, ^C     quit\n"
"    x         quit and continue playing\n"
"    a         toggle auto-mode\n";


/* ---------------------------------------------------------------------- */
/* tty stuff                                                              */

#define LINE_STATUS    0
#define LINE_CDINFO    1

#define LINE_ARTIST    3
#define LINE_CDNAME    4
#define LINE_GENRE     5
#define LINE_YEAR      6
#define LINE_TRACK_A   7
#define LINE_TRACK_T   8

#define LINE_ACTION   10
#define LINE_LAST     11

static void
tty_raw()
{
  if (!interactive) return;
  
  initscr();
  cbreak();
  noecho();
  keypad(stdscr,1);
  refresh();
}

static void
tty_restore()
{
  if (!interactive) return;
  
  endwin();
}

static void
ctrlc(int signal)
{
  b_sig = true;
}

static int
select_wait(int sec)
{
  struct timeval  tv;
  fd_set          se;
  
  FD_ZERO(&se);
  FD_SET(0,&se);
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  return select(1,&se,NULL,NULL,&tv);
}

/* ---------------------------------------------------------------------- */

static void 
action(const char *txt)
{
  if (!interactive) {
    if (b_verbose)
      fprintf(stderr,"action: %s\n",txt);
    return;
  }
  
  if (txt && strlen(txt))
    mvprintw(LINE_ACTION,0,"action : %-70s\n",txt);
  else
    mvprintw(LINE_ACTION,0,"%-79s\n","");
  refresh();
}

static void inline
xperror(const char *txt)
{
  char line[80];
  
  if (!interactive) {
    if (b_verbose) {
      fprintf(stderr,"error: ");
      perror(txt);
    }
    return;
  }
  
  if (b_verbose) {
    sprintf(line,"%s: %s",txt,strerror(errno));
    mvprintw(LINE_ACTION, 0, "error  : %-70s", line);
    refresh();
    select_wait(3);
    action(NULL);
  }
}

static void 
oops(const char *txt, int rc)
{
  if(interactive) {
    mvprintw(LINE_LAST, 0, "%s, exiting...\n", txt);
    refresh();
  }
  tty_restore();
#ifdef HAVE_CDDB
  if (p_conn) cddb_destroy(p_conn);
#endif
  cdio_destroy (p_cdio);
  free (psz_device);
  exit (rc);
}

/* ---------------------------------------------------------------------- */

static void
cd_stop(CdIo_t *p_cdio)
{
  if (b_cd && p_cdio) {
    action("stop...");
    if (DRIVER_OP_SUCCESS != cdio_audio_stop(p_cdio))
      xperror("stop");
  }
}

static void
cd_eject(void)
{
  if (p_cdio) {
    cd_stop(p_cdio);
    action("eject...");
    if (DRIVER_OP_SUCCESS != cdio_eject_media(&p_cdio))
      xperror("eject");
    b_cd = 0;
    p_cdio = NULL;
  }
}

static void
cd_close(const char *psz_device)
{
  if (!b_cd) {
    action("close...");
    if (DRIVER_OP_SUCCESS != cdio_close_tray(psz_device, &driver_id))
      xperror("close");
  }
}

static void
cd_pause(CdIo_t *p_cdio)
{
  if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)
    if (DRIVER_OP_SUCCESS != cdio_audio_pause(p_cdio))
      xperror("pause");
}

static void
read_subchannel(CdIo_t *p_cdio)
{
  if (!b_cd) return;

  sub.format = CDIO_CDROM_MSF;
  if (DRIVER_OP_SUCCESS != cdio_audio_read_subchannel(p_cdio, &sub)) {
    xperror("read subchannel");
    b_cd = 0;
  }
  if (auto_mode && sub.audio_status == CDIO_MMC_READ_SUB_ST_COMPLETED)
    cd_eject();
}

static void
read_toc(CdIo_t *p_cdio)
{
  track_t i;

  action("read toc...");

  memset(cd_info, 0, sizeof(cd_info));
  title[0] = artist[0] = genre[0] = category[0] = year[0] = '\0';

  i_first_track       = cdio_get_first_track_num(p_cdio);
  i_last_track        = cdio_get_last_track_num(p_cdio);
  i_tracks            = cdio_get_num_tracks(p_cdio);
  i_first_audio_track = i_first_track;
  i_last_audio_track  = i_last_track;
  
  if ( CDIO_INVALID_TRACK == i_first_track ||
       CDIO_INVALID_TRACK == i_last_track ) {
    xperror("read toc header");
    b_cd = false;
    b_record = false;
  } else {
    b_cd = true;
    data = 0;
    b_db = init_cddb(p_cdio, i_tracks);
    for (i = i_first_track; i <= i_last_track+1; i++) {
      if ( !cdio_get_track_msf(p_cdio, i, &(toc[i])) )
      {
	xperror("read toc entry");
	b_cd = false;
	return;
      }
      if ( (TRACK_FORMAT_AUDIO != cdio_get_track_format(p_cdio, i)) &&
	   (i != i_last_track+1) ) {
	data++;
	if (i == i_first_track) {
	  if (i == i_last_track)
	    i_first_audio_track = CDIO_CDROM_LEADOUT_TRACK;
	  else
	    i_first_audio_track++;
	}
      }
      get_cddb_track_info(i);
    }
    b_record = true;
    read_subchannel(p_cdio);
    if (auto_mode && sub.audio_status != CDIO_MMC_READ_SUB_ST_PLAY)
      play_track(1, CDIO_CDROM_LEADOUT_TRACK);
  }
  display_cdinfo(p_cdio, i_tracks, i_first_track);
}

static void
play_track(track_t i_start_track, track_t i_end_track)
{
  char line[80];
  
  if (!b_cd) {
    cd_close(psz_device);
    read_toc(p_cdio);
  }
  
  read_subchannel(p_cdio);
  if (!b_cd || i_first_track == CDIO_CDROM_LEADOUT_TRACK)
    return;
  
  if (debug)
    fprintf(stderr,"play tracks: %d-%d => ", i_start_track, i_end_track);
  if (i_start_track < i_first_track)       i_start_track = i_first_track;
  if (i_start_track > i_last_audio_track)  i_start_track = i_last_audio_track;
  if (i_end_track < i_first_track)         i_end_track   = i_first_track;
  if (i_end_track > i_last_audio_track)    i_end_track   = i_last_audio_track;
  if (debug)
    fprintf(stderr,"%d-%d\n",i_start_track, i_end_track);
  
  cd_pause(p_cdio);
  sprintf(line,"play track %d...", i_start_track);
  action(line);
  if ( DRIVER_OP_SUCCESS != cdio_audio_play_msf(p_cdio, &(toc[i_start_track]),
						&(toc[i_end_track])) )
    xperror("play");
}

static void
skip(int diff)
{
  msf_t start_msf;
  int   sec;
  
  read_subchannel(p_cdio);
  if (!b_cd ||  i_first_track == CDIO_CDROM_LEADOUT_TRACK)
    return;
  
  sec  = cdio_audio_get_msf_seconds(&sub.abs_addr.msf);
  sec += diff;
  if (sec < 0) sec = 0;
  
  start_msf.m = cdio_to_bcd8(sec / CDIO_CD_SECS_PER_MIN);
  start_msf.s = cdio_to_bcd8(sec % CDIO_CD_SECS_PER_MIN);
  start_msf.f = 0;
  
  cd_pause(p_cdio);
  if ( DRIVER_OP_SUCCESS != cdio_audio_play_msf(p_cdio, &start_msf, 
						&(toc[i_last_audio_track])) )
    xperror("play");
}

static void
toggle_pause()
{
  if (!b_cd) return;
  
  if (CDIO_MMC_READ_SUB_ST_PAUSED == sub.audio_status) {
    if (DRIVER_OP_SUCCESS != cdio_audio_resume(p_cdio))
      xperror("resume");
  } else {
    if (DRIVER_OP_SUCCESS != cdio_audio_pause(p_cdio))
      xperror("pause");
  }
}

static void
display_status()
{
  char line[80];

  if (!interactive) return;

  if (!b_cd) {
    sprintf(line,"no CD in drive (%s)", psz_device);

  } else if (i_first_track == CDIO_CDROM_LEADOUT_TRACK) {
    sprintf(line,"CD has only data tracks");
    
  } else if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
	     sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY) {
    sprintf(line,"%2d - %02d:%02d (%02d:%02d abs)  %-10s",
	    sub.track,
	    sub.rel_addr.msf.m,
	    sub.rel_addr.msf.s,
	    sub.abs_addr.msf.m,
	    sub.abs_addr.msf.s,
	    mmc_audio_state2str(sub.audio_status));
    
  } else {
    sprintf(line,"%s", mmc_audio_state2str(sub.audio_status));
    
  }
  mvprintw(LINE_STATUS,0,"status%s: %-70s",auto_mode ? "*" : " ", line);
  
  if (b_db) {
    if ((sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
	 sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)  &&
	b_cd) {
      if (strlen(cd_info[sub.track].artist))
	mvprintw(LINE_TRACK_A, 0,"track artist : %-70s", 
		 cd_info[sub.track].artist);
      if (strlen(cd_info[sub.track].title))
	mvprintw(LINE_TRACK_T, 0,"track title  : %-70s", 
		 cd_info[sub.track].title);
    }
  }
  refresh();
  action(NULL);
}

#define add_cddb_disc_info(format_str, field) \
  if (p_cddb_disc->field) \
    snprintf(field, sizeof(field)-1, format_str, p_cddb_disc->field);

static bool
init_cddb(CdIo_t *p_cdio, track_t i_tracks)
{
#ifdef HAVE_CDDB
  track_t i;
  
  if (p_conn) cddb_destroy(p_conn);
  p_conn =  cddb_new();
  p_cddb_disc = NULL;
  
  if (!p_conn) {
    xperror("unable to initialize libcddb");
    return false;
  }
  
  if (NULL == opts.cddb_email) 
    cddb_set_email_address(p_conn, "me@home");
  else 
    cddb_set_email_address(p_conn, opts.cddb_email);
  
  if (NULL == opts.cddb_server) 
    cddb_set_server_name(p_conn, "freedb.freedb.org");
  else 
    cddb_set_server_name(p_conn, opts.cddb_server);
  
  if (opts.cddb_timeout >= 0) 
    cddb_set_timeout(p_conn, opts.cddb_timeout);
  
  cddb_set_server_port(p_conn, opts.cddb_port);
  
  if (opts.cddb_http) 
    cddb_http_enable(p_conn);
  else 
    cddb_http_disable(p_conn);
  
  if (NULL != opts.cddb_cachedir) 
    cddb_cache_set_dir(p_conn, opts.cddb_cachedir);
  
  if (opts.cddb_disable_cache) 
    cddb_cache_disable(p_conn);
  
  p_cddb_disc = cddb_disc_new();
  if (!p_cddb_disc) {
    xperror("unable to create CDDB disc structure");
    return false;
  }
  for(i = 0; i < i_tracks; i++) {
    cddb_track_t *t = cddb_track_new(); 
    t->frame_offset = cdio_get_track_lba(p_cdio, i+i_first_track);
    cddb_disc_add_track(p_cddb_disc, t);
  }
  
  p_cddb_disc->length = 
    cdio_get_track_lba(p_cdio, CDIO_CDROM_LEADOUT_TRACK) 
    / CDIO_CD_FRAMES_PER_SEC;
  
  if (!cddb_disc_calc_discid(p_cddb_disc)) {
    xperror("libcddb calc discid failed.");
    return false;
  }
  
  i_cddb_matches = cddb_query(p_conn, p_cddb_disc);
  
  if (-1 == i_cddb_matches) 
    xperror(cddb_error_str(cddb_errno(p_conn)));

  cddb_read(p_conn, p_cddb_disc);

  add_cddb_disc_info("%s",  artist);
  add_cddb_disc_info("%s",  title);
  add_cddb_disc_info("%s",  genre);
  add_cddb_disc_info("%5d", year);
  return true;
#else
    return false;
#endif      
}

#define add_cddb_track_info(format_str, field) \
  if (t->field) \
    snprintf(cd_info[i_track].field, sizeof(cd_info[i_track].field)-1, \
	     format_str, t->field);

static void
get_cddb_track_info(track_t i_track)
{
#ifdef HAVE_CDDB
  cddb_track_t *t = cddb_disc_get_track(p_cddb_disc, 
					i_track - i_first_track);
  if (t) {
    add_cddb_track_info("%s", title);
    add_cddb_track_info("%s", artist);
  }
  
#else
    ;
#endif      
}

#define display_line(LINE_NO, COL_NO, format_str, field) \
  if (field && field[0]) \
    mvprintw(LINE_NO, COL_NO, format_str, field);
    
static void
display_cdinfo(CdIo_t *p_cdio, track_t i_tracks, track_t i_first_track)
{
  int len;
  char line[80];
  
  if (!interactive) return;

  if (!b_cd) sprintf(line, "-");
  else {
    len = sprintf(line, "%2u tracks  (%02x:%02x min)",
		  (unsigned int) i_last_track,
		  toc[i_last_track+1].m, toc[i_last_track+1].s);
    if (data && i_first_track != CDIO_CDROM_LEADOUT_TRACK)
      sprintf(line+len,", audio=%u-%u", (unsigned int) i_first_audio_track,
	      (unsigned int) i_last_audio_track);
    
    display_line(LINE_ARTIST, 0, "Artist      : %-70s", artist);
    display_line(LINE_CDNAME, 0, "CD          : %-70s", title);
    display_line(LINE_GENRE,  0, "Genre       : %-40s", genre);
    display_line(LINE_YEAR,   0, "Year        : %5s",   year);
  }
  
  mvprintw(LINE_CDINFO, 0, "CD info: %-70s", line);
  refresh();
}
    
/* ---------------------------------------------------------------------- */

static void
usage(char *prog)
{
    fprintf(stderr,
	    "%s is a simple curses CD player.  It can pick up artist,\n"
	    "CD name and song title from a workman database file or\n"
	    "via cddb.\n"
	    "\n"
	    "usage: %s [options] [device]\n"
            "\n"
	    "default for to search for a CD-ROM device with a CD-DA loaded\n"
	    "\n"
	    "These command line options available:\n"
	    "  -h      print this help\n"
	    "  -k      print key mapping\n"
	    "  -a      start up in auto-mode\n"
	    "  -v      verbose\n"
	    "\n"
	    "for non-interactive use (only one of them):\n"
	    "  -l      list tracks\n"
	    "  -c      print cover (PostScript to stdout)\n"
	    "  -p      play the whole CD\n"
	    "  -t n    play track >n<\n"
	    "  -t a-b  play all tracks between a and b (inclusive)\n"
	    "  -s      stop playing\n"
	    "  -e      eject cdrom\n"
            "\n"
	    "Thats all. Oh, maybe a few words more about the auto-mode. This\n"
	    "is the 'dont-touch-any-key' feature. You load a CD, player starts\n"
	    "to play it, and when it is done it ejects the CD. Start it that\n"
	    "way on a spare console and forget about it...\n"
	    "\n"
	    "Title database access:\n"
	    "  workman:     checks $HOME/.workmandb\n"
	    "  cddb remote: disabled by default, set CDDB_SERVER to \"server:port\"\n"
	    "               to enable it.\n"
	    "\n"
	    "(c) 1997,98 Gerd Knorr <kraxel@goldbach.in-berlin.de>\n"
	    "(c) 2005 Rocky Bernstein <rocky@panix.com>\n"
	    , prog, prog);
}

static void
keys()
{
  fprintf(stderr, key_bindings);
}

static void
tracklist(void)
{
  track_t i;
  int s;

  if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY) {
    printf("drive plays track %d (%02x:%02x)\n\n",
	   sub.track, sub.rel_addr.msf.m, sub.rel_addr.msf.s);
  }
  printf("%2d  %02x:%02x  |  ", (int) i_last_track,
	 toc[i_last_track+1].m,
	 toc[i_last_track+1].s);
  if (b_record)
    printf("%s / %s", artist, title);
  printf("\n");
  printf("-----------+--------------------------------------------------\n");
  for (i = i_first_track; i <= i_last_track; i++) {
    s = cdio_audio_get_msf_seconds(&toc[i+1]) 
      - cdio_audio_get_msf_seconds(&toc[i]);
    printf("%2d  %02d:%02d  %s  ", i, 
	   s / CDIO_CD_SECS_PER_MIN,  s % CDIO_CD_SECS_PER_MIN,
	   (sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY &&
	    sub.track == i) ? "*" : "|");
    if (b_record) {
      if ( !strlen(cd_info[i].title) && !strlen(cd_info[i].artist) )
	printf("\n");
      else { 
	if ( strlen(cd_info[i].title) )
	  printf("%s\n", cd_info[i].title);
	if ( strlen(cd_info[i].artist) > 0 ) {
	  printf("           %s  %s\n", 
		 (sub.track == i) ? "*" : "|", cd_info[i].artist);
	}
      }
    } else
      printf("\n");
  }
}

/*
 * PostScript 8bit latin1 handling
 * stolen from mpage output -- please don't ask me how this works...
 */
#define ENCODING_TRICKS \
	"/reencsmalldict 12 dict def\n"				\
	"/ReEncodeSmall { reencsmalldict begin\n"		\
	"/newcodesandnames exch def /newfontname exch def\n"	\
	"/basefontname exch def\n"				\
	"/basefontdict basefontname findfont def\n"		\
	"/newfont basefontdict maxlength dict def\n"		\
	"basefontdict { exch dup /FID ne { dup /Encoding eq\n"	\
	"{ exch dup length array copy newfont 3 1 roll put }\n"	\
	"{ exch newfont 3 1 roll put }\n"			\
	"ifelse }\n"						\
	"{ pop pop }\n"						\
	"ifelse } forall\n"					\
	"newfont /FontName newfontname put\n"			\
	"newcodesandnames aload pop newcodesandnames length 2 idiv\n"	\
	"{ newfont /Encoding get 3 1 roll put } repeat\n"	\
	"newfontname newfont definefont pop end } def\n"	\
	"/charvec [\n"		\
	"026 /Scaron\n"		\
	"027 /Ydieresis\n"	\
	"028 /Zcaron\n"		\
	"029 /scaron\n"		\
	"030 /trademark\n"	\
	"031 /zcaron\n"		\
	"032 /space\n"		\
	"033 /exclam\n"		\
	"034 /quotedbl\n"	\
	"035 /numbersign\n"	\
	"036 /dollar\n"		\
	"037 /percent\n"	\
	"038 /ampersand\n"	\
	"039 /quoteright\n"	\
	"040 /parenleft\n"	\
	"041 /parenright\n"	\
	"042 /asterisk\n"	\
	"043 /plus\n"		\
	"044 /comma\n"		\
	"045 /minus\n"		\
	"046 /period\n"		\
	"047 /slash\n"		\
	"048 /zero\n"		\
	"049 /one\n"		\
	"050 /two\n"		\
	"051 /three\n"		\
	"052 /four\n"		\
	"053 /five\n"		\
	"054 /six\n"		\
	"055 /seven\n"		\
	"056 /eight\n"		\
	"057 /nine\n"		\
	"058 /colon\n"		\
	"059 /semicolon\n"	\
	"060 /less\n"		\
	"061 /equal\n"		\
	"062 /greater\n"	\
	"063 /question\n"	\
	"064 /at\n"		\
	"065 /A\n"		\
	"066 /B\n"		\
	"067 /C\n"		\
	"068 /D\n"		\
	"069 /E\n"		\
	"070 /F\n"		\
	"071 /G\n"		\
	"072 /H\n"		\
	"073 /I\n"		\
	"074 /J\n"		\
	"075 /K\n"		\
	"076 /L\n"		\
	"077 /M\n"		\
	"078 /N\n"		\
	"079 /O\n"		\
	"080 /P\n"		\
	"081 /Q\n"		\
	"082 /R\n"		\
	"083 /S\n"		\
	"084 /T\n"		\
	"085 /U\n"		\
	"086 /V\n"		\
	"087 /W\n"		\
	"088 /X\n"		\
	"089 /Y\n"		\
	"090 /Z\n"		\
	"091 /bracketleft\n"	\
	"092 /backslash\n"	\
	"093 /bracketright\n"	\
	"094 /asciicircum\n"	\
	"095 /underscore\n"	\
	"096 /quoteleft\n"	\
	"097 /a\n"		\
	"098 /b\n"		\
	"099 /c\n"		\
	"100 /d\n"		\
	"101 /e\n"		\
	"102 /f\n"		\
	"103 /g\n"		\
	"104 /h\n"		\
	"105 /i\n"		\
	"106 /j\n"		\
	"107 /k\n"		\
	"108 /l\n"		\
	"109 /m\n"		\
	"110 /n\n"		\
	"111 /o\n"		\
	"112 /p\n"		\
	"113 /q\n"		\
	"114 /r\n"		\
	"115 /s\n"		\
	"116 /t\n"		\
	"117 /u\n"		\
	"118 /v\n"		\
	"119 /w\n"		\
	"120 /x\n"		\
	"121 /y\n"		\
	"122 /z\n"		\
	"123 /braceleft\n"	\
	"124 /bar\n"		\
	"125 /braceright\n"	\
	"126 /asciitilde\n"	\
	"127 /.notdef\n"	\
	"128 /fraction\n"	\
	"129 /florin\n"		\
	"130 /quotesingle\n"	\
	"131 /quotedblleft\n"	\
	"132 /guilsinglleft\n"	\
	"133 /guilsinglright\n"	\
	"134 /fi\n"		\
	"135 /fl\n"		\
	"136 /endash\n"		\
	"137 /dagger\n"		\
	"138 /daggerdbl\n"	\
	"139 /bullet\n"		\
	"140 /quotesinglbase\n"	\
	"141 /quotedblbase\n"	\
	"142 /quotedblright\n"	\
	"143 /ellipsis\n"	\
	"144 /dotlessi\n"	\
	"145 /grave\n"		\
	"146 /acute\n"		\
	"147 /circumflex\n"	\
	"148 /tilde\n"		\
	"149 /oe\n"		\
	"150 /breve\n"		\
	"151 /dotaccent\n"	\
	"152 /perthousand\n"	\
	"153 /emdash\n"		\
	"154 /ring\n"		\
	"155 /Lslash\n"		\
	"156 /OE\n"		\
	"157 /hungarumlaut\n"	\
	"158 /ogonek\n"		\
	"159 /caron\n"		\
	"160 /lslash\n"		\
	"161 /exclamdown\n"	\
	"162 /cent\n"		\
	"163 /sterling\n"	\
	"164 /currency\n"	\
	"165 /yen\n"		\
	"166 /brokenbar\n"	\
	"167 /section\n"	\
	"168 /dieresis\n"	\
	"169 /copyright\n"	\
	"170 /ordfeminine\n"	\
	"171 /guillemotleft\n"	\
	"172 /logicalnot\n"	\
	"173 /hyphen\n"		\
	"174 /registered\n"	\
	"175 /macron\n"		\
	"176 /degree\n"		\
	"177 /plusminus\n"	\
	"178 /twosuperior\n"	\
	"179 /threesuperior\n"	\
	"180 /acute\n"		\
	"181 /mu\n"		\
	"182 /paragraph\n"	\
	"183 /periodcentered\n"	\
	"184 /cedilla\n"	\
	"185 /onesuperior\n"	\
	"186 /ordmasculine\n"	\
	"187 /guillemotright\n"	\
	"188 /onequarter\n"	\
	"189 /onehalf\n"	\
	"190 /threequarters\n"	\
	"191 /questiondown\n"	\
	"192 /Agrave\n"		\
	"193 /Aacute\n"		\
	"194 /Acircumflex\n"	\
	"195 /Atilde\n"		\
	"196 /Adieresis\n"	\
	"197 /Aring\n"		\
	"198 /AE\n"		\
	"199 /Ccedilla\n"	\
	"200 /Egrave\n"		\
	"201 /Eacute\n"		\
	"202 /Ecircumflex\n"	\
	"203 /Edieresis\n"	\
	"204 /Igrave\n"		\
	"205 /Iacute\n"		\
	"206 /Icircumflex\n"	\
	"207 /Idieresis\n"	\
	"208 /Eth\n"		\
	"209 /Ntilde\n"		\
	"210 /Ograve\n"		\
	"211 /Oacute\n"		\
	"212 /Ocircumflex\n"	\
	"213 /Otilde\n"		\
	"214 /Odieresis\n"	\
	"215 /multiply\n"	\
	"216 /Oslash\n"		\
	"217 /Ugrave\n"		\
	"218 /Uacute\n"		\
	"219 /Ucircumflex\n"	\
	"220 /Udieresis\n"	\
	"221 /Yacute\n"		\
	"222 /Thorn\n"		\
	"223 /germandbls\n"	\
	"224 /agrave\n"		\
	"225 /aacute\n"		\
	"226 /acircumflex\n"	\
	"227 /atilde\n"		\
	"228 /adieresis\n"	\
	"229 /aring\n"		\
	"230 /ae\n"		\
	"231 /ccedilla\n"	\
	"232 /egrave\n"		\
	"233 /eacute\n"		\
	"234 /ecircumflex\n"	\
	"235 /edieresis\n"	\
	"236 /igrave\n"		\
	"237 /iacute\n"		\
	"238 /icircumflex\n"	\
	"239 /idieresis\n"	\
	"240 /eth\n"		\
	"241 /ntilde\n"		\
	"242 /ograve\n"		\
	"243 /oacute\n"		\
	"244 /ocircumflex\n"	\
	"245 /otilde\n"		\
	"246 /odieresis\n"	\
	"247 /divide\n"		\
	"248 /oslash\n"		\
	"249 /ugrave\n"		\
	"250 /uacute\n"		\
	"251 /ucircumflex\n"	\
	"252 /udieresis\n"	\
	"253 /yacute\n"		\
	"254 /thorn\n"		\
	"255 /ydieresis\n"	\
	"] def"


static void
ps_tracklist(void)
{
  int i,s,y,sy;
  
  if (!b_record) return;

  printf("%%!PS-Adobe-2.0\n");
  
  /* encoding tricks */
  puts(ENCODING_TRICKS);
  printf("/Times /TimesLatin1 charvec ReEncodeSmall\n");
  printf("/Helvetica /HelveticaLatin1 charvec ReEncodeSmall\n");
  
  /* Spaces */
  printf("0 setlinewidth\n");
  printf(" 100 100 moveto\n");
  printf(" 390   0 rlineto\n");
  printf("   0 330 rlineto\n");
  printf("-390   0 rlineto\n");
  printf("closepath stroke\n");
  
  printf(" 100 100 moveto\n");
  printf("-16    0 rlineto\n");
  printf("  0  330 rlineto\n");
  printf("422    0 rlineto\n");
  printf("  0 -330 rlineto\n");
  printf("closepath stroke\n");
  
  /* Title */
  printf("/TimesLatin1 findfont 24 scalefont setfont\n");
  printf("120 400 moveto (%s) show\n", title);
  printf("/TimesLatin1 findfont 18 scalefont setfont\n");
  printf("120 375 moveto (%s) show\n", artist);
  
  /* List */
  sy = 250 / i_last_track;
  if (sy > 14) sy = 14;
  printf("/labelfont /TimesLatin1 findfont %d scalefont def\n",sy-2);
  printf("/timefont /Courier findfont %d scalefont def\n",sy-2);
  for (i = i_first_track, y = 350; i <= i_last_track; i++, y -= sy) {
    s = cdio_audio_get_msf_seconds(&toc[i+1]) 
      - cdio_audio_get_msf_seconds(&toc[i]);
    
    printf("labelfont setfont\n");
    printf("120 %d moveto (%d) show\n", y, i);
    printf("150 %d moveto (%s) show\n", y, cd_info[i].title);
    
    printf("timefont setfont\n");
    printf("420 %d moveto (%2d:%02d) show\n", y, 
	   s / CDIO_CD_SECS_PER_MIN, s % CDIO_CD_SECS_PER_MIN);
  }
  
  /* Seitenbanner */
  printf("/HelveticaLatin1 findfont 12 scalefont setfont\n");
  printf(" 97 105 moveto (%s: %s) 90 rotate show -90 rotate\n",
	 artist, title);
  printf("493 425 moveto (%s: %s) -90 rotate show 90 rotate\n",
	 artist, title);
  printf("showpage\n");
}

int
main(int argc, char *argv[])
{    
  int             c,key,nostop=0;
  char            *h;
  
  psz_program = strrchr(argv[0],'/');
  psz_program = psz_program ? psz_program+1 : argv[0];

  memset(&opts, 0, sizeof(opts));
  
  /* parse options */
  while ( 1 ) {
    if (-1 == (c = getopt(argc, argv, "xdhkvcCpset:la")))
      break;
    switch (c) {
    case 'v':
      b_verbose = true;
      break;
    case 'd':
      debug = 1;
      break;
    case 'a':
      auto_mode = 1;
      break;
    case 't':
      if (NULL != (h = strchr(optarg,'-'))) {
	*h = 0;
	start_track = atoi(optarg);
	stop_track = atoi(h+1)+1;
	if (0 == start_track) start_track = 1;
	if (1 == stop_track)  stop_track  = CDIO_CDROM_LEADOUT_TRACK;
      } else {
	start_track = atoi(optarg);
	stop_track = start_track+1;
	one_track = 1;
      }
      interactive = 0;
      todo = PLAY_TRACK;
      break;
    case 'p':
      interactive = 0;
      todo = PLAY_CD;
      break;
    case 'l':
      interactive = 0;
      todo = LIST_TRACKS;
      break;
    case 'C':
      interactive = 0;
      todo = CLOSE_CD;
      break;
    case 'c':
      interactive = 0;
      todo = PS_LIST_TRACKS;
      break;
    case 's':
      interactive = 0;
      todo = STOP_PLAYING;
      break;
    case 'e':
      interactive = 0;
      todo = EJECT_CD;
      break;
    case 'k':
      keys();
      exit(1);
    case 'h':
      usage(psz_program);
      exit(1);
    default:
      usage(psz_program);
      exit(1);
    }
  }
  
  if (argc > optind) {
    psz_device = strdup(argv[optind]);
  } else {
    char **ppsz_cdda_drives=NULL;
    char **ppsz_all_cd_drives = cdio_get_devices_ret(&driver_id);

    if (!ppsz_all_cd_drives) {
      fprintf(stderr, "Can't find a CD-ROM drive\n");
      exit(2);
    }
    ppsz_cdda_drives = cdio_get_devices_with_cap(ppsz_all_cd_drives, 
						 CDIO_FS_AUDIO, false);
    if (!ppsz_cdda_drives || !ppsz_cdda_drives[0]) {
      fprintf(stderr, "Can't find a CD-ROM drive with a CD-DA in it\n");
      exit(3);
    }
    psz_device = strdup(ppsz_cdda_drives[0]);
    cdio_free_device_list(ppsz_all_cd_drives);
    cdio_free_device_list(ppsz_cdda_drives);
  }
  
  if (!interactive) {
    b_sig = true;
    nostop=1;
    if (!b_cd && todo != EJECT_CD) {
      cd_close(psz_device);
    }
  }

  /* open device */
  if (b_verbose)
    fprintf(stderr,"open %s... ", psz_device);

  p_cdio = cdio_open (psz_device, driver_id);

  if (!p_cdio) {
    if (b_verbose)
      fprintf(stderr, "error: %s\n", strerror(errno));
    else
      fprintf(stderr, "open %s: %s\n", psz_device, strerror(errno));
    exit(1);
  } else
    if (b_verbose)
      fprintf(stderr,"ok\n");
  
  tty_raw();
  signal(SIGINT,ctrlc);
  signal(SIGQUIT,ctrlc);
  signal(SIGTERM,ctrlc);
  signal(SIGHUP,ctrlc);
  
  if (!interactive) {
    b_sig = true;
    nostop=1;
    if (EJECT_CD == todo) {
	cd_eject();
    } else {
      read_toc(p_cdio);
      if (!b_cd) {
	cd_close(psz_device);
	read_toc(p_cdio);
      }
      if (b_cd)
	switch (todo) {
	case STOP_PLAYING:
	  cd_stop(p_cdio);
	  break;
	case EJECT_CD:
	  /* Should have been handled above. */
	  cd_eject();
	  break;
	case LIST_TRACKS:
	  tracklist();
	  break;
	case PS_LIST_TRACKS:
	  ps_tracklist();
	  break;
	case PLAY_TRACK:
	  /* play just this one track */
	  if (b_record) {
	    printf("%s / %s\n", artist, title);
	    if (one_track)
	      printf("%s\n", cd_info[start_track].title);
	  }
	  play_track(start_track, stop_track);
	  break;
	case PLAY_CD:
	  if (b_record)
	    printf("%s / %s\n", artist, title);
	  play_track(1,CDIO_CDROM_LEADOUT_TRACK);
	  break;
	}
      else {
	fprintf(stderr,"no CD in drive (%s)\n", psz_device);
      }
    }
  }
  
  while ( !b_sig ) {
    if (!b_cd) read_toc(p_cdio);
    read_subchannel(p_cdio);
    display_status();
    
    if (1 == select_wait(b_cd ? 1 : 5)) {
      switch (key = getch()) {
      case 'A':
      case 'a':
	auto_mode = !auto_mode;
	break;
      case 'X':
      case 'x':
	nostop=1;
	/* fall */
      case 'Q':
      case 'q':
	b_sig = true;
	break;
      case 'E':
      case 'e':
	cd_eject();
	break;
      case 'S':
      case 's':
	cd_stop(p_cdio);
	break;
      case 'C':
      case 'c':
	cd_close(psz_device);
	break;
      case ' ':
      case 'P':
      case 'p':
	toggle_pause();
	break;
      case KEY_RIGHT:
	if (b_cd &&
	    (sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
	     sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)) 
	  play_track(sub.track+1, CDIO_CDROM_LEADOUT_TRACK);
	else
	  play_track(1,CDIO_CDROM_LEADOUT_TRACK);
	break;
      case KEY_LEFT:
	if (b_cd &&
	    (sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
	     sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY))
	  play_track(sub.track-1,CDIO_CDROM_LEADOUT_TRACK);
	break;
      case KEY_UP:
	if (b_cd && sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)
	  skip(10);
	break;
      case KEY_DOWN:
	if (b_cd && sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)
	  skip(-10);
	break;
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
	play_track(key - '0',CDIO_CDROM_LEADOUT_TRACK);
	break;
      case '0':
	play_track(10,CDIO_CDROM_LEADOUT_TRACK);
	break;
      case KEY_F(1):
      case KEY_F(2):
      case KEY_F(3):
      case KEY_F(4):
      case KEY_F(5):
      case KEY_F(6):
      case KEY_F(7):
      case KEY_F(8):
      case KEY_F(9):
      case KEY_F(10):
      case KEY_F(11):
      case KEY_F(12):
      case KEY_F(13):
      case KEY_F(14):
      case KEY_F(15):
      case KEY_F(16):
      case KEY_F(17):
      case KEY_F(18):
      case KEY_F(19):
      case KEY_F(20):
	play_track(key - KEY_F(1) + 11,CDIO_CDROM_LEADOUT_TRACK);
	break;
      }
    }
  }
  if (!nostop) cd_stop(p_cdio);
  tty_restore();
  oops("bye", 0);
  
  return 0; /* keep compiler happy */
}
