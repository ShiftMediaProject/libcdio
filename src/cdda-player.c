/*
    $Id: cdda-player.c,v 1.17 2005/03/17 08:19:19 rocky Exp $

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

#ifdef HAVE_CURSES_H
#include <curses.h>
#else 
#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#  error "You need <curses.h> or <ncurses.h to build cdda-player"
#endif
#endif

#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <cdio/util.h>
#include <cdio/cd_types.h>
#include <cdio/logging.h>

#include "cddb.h"

static bool play_track(track_t t1, track_t t2);
static void display_cdinfo(CdIo_t *p_cdio, track_t i_tracks, 
			   track_t i_first_track);
static void get_cddb_track_info(track_t i_track);
static void get_cdtext_track_info(track_t i_track);
static void get_track_info(track_t i_track);

CdIo_t             *p_cdio;               /* libcdio handle */
driver_id_t        driver_id = DRIVER_DEVICE;
int b_sig = false;                                /* set on some signals */

/* cdrom data */
track_t            i_first_track;
track_t            i_last_track;
track_t            i_first_audio_track;
track_t            i_last_audio_track;
track_t            i_last_display_track = CDIO_INVALID_TRACK;
track_t            i_tracks;
msf_t              toc[CDIO_CDROM_LEADOUT_TRACK+1];
cdio_subchannel_t  sub;      /* subchannel last time read */
int                i_data;     /* # of data tracks present ? */
int                start_track = 0;
int                stop_track = 0;
int                one_track = 0;
int                i_vol_port   = -1; /* If -1 get retrieve volume port.
					 Otherwise the port number 0..3
					 of a working volume port.
				       */

bool               b_cd         = false;
bool               auto_mode    = false;
bool               b_verbose    = false;
bool               debug        = false;
bool               interactive  = true;
bool               b_prefer_cdtext = true; 
bool               b_cddb   = false; /* CDDB database present */
bool               b_db     = false; /* we have a database at all */
bool               b_record = false; /* we have a record for
					the inserted CD */

char *psz_device=NULL;
char *psz_program;

/* Info about songs and titles. The 0 entry will contain the disc info.
 */
typedef struct 
{
  char title[80];
  char artist[80];
  char length[8];
  char ext_data[80];
  bool b_cdtext;  /* true if from CD-Text, false if from CDDB */
} cd_track_info_rec_t;

cd_track_info_rec_t cd_info[CDIO_CD_MAX_TRACKS+2];

char title[80];
char artist[80];
char genre[40];
char category[40];
char year[5];

bool b_cdtext_title;     /* true if from CD-Text, false if from CDDB */
bool b_cdtext_artist;    /* true if from CD-Text, false if from CDDB */
bool b_cdtext_genre;     /* true if from CD-Text, false if from CDDB */
bool b_cdtext_category;  /* true if from CD-Text, false if from CDDB */
bool b_cdtext_year;  /* true if from CD-Text, false if from CDDB */

#ifdef HAVE_CDDB
cddb_conn_t *p_conn = NULL;
cddb_disc_t *p_cddb_disc = NULL;
int i_cddb_matches = 0;
#endif

#define MAX_KEY_STR 50
const char key_bindings[][MAX_KEY_STR] = {
  "    right     play / next track",
  "    left      previous track",
  "    up/down   10 sec forward / back",
  "    1-9       jump to track 1-9",
  "    0         jump to track 10",
  "    F1-F20    jump to track 11-30",
  " ",
  "    k, h, ?   show this key help",
  "    l,        list tracks",
  "    e         eject",
  "    c         close tray",
  "    p, space  pause / resume",
  "    s         stop",
  "    q, ^C     quit",
  "    x         quit and continue playing",
  "    a         toggle auto-mode",
};

const unsigned int i_key_bindings = sizeof(key_bindings) / MAX_KEY_STR;

/* ---------------------------------------------------------------------- */
/* tty stuff                                                              */

typedef enum {
  LINE_STATUS       =  0,
  LINE_CDINFO       =  1,

  LINE_ARTIST       =  3,
  LINE_CDNAME       =  4,
  LINE_GENRE        =  5,
  LINE_YEAR         =  6,

  LINE_TRACK_PREV   =  8,
  LINE_TRACK_TITLE  =  9,
  LINE_TRACK_ARTIST = 10,
  LINE_TRACK_NEXT   = 11,

  LINE_ACTION       = 24,
  LINE_LAST         = 25
} track_line_t;

/*! Curses window initialization. */
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

/*! Curses window finalization. */
static void
tty_restore()
{
  if (!interactive) return;
  endwin();
}

/* Signal handler - Ctrl-C and others. */
static void
ctrlc(int signal)
{
  b_sig = true;
}

/* Timed wait on an event. */
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
action(const char *psz_action)
{
  if (!interactive) {
    if (b_verbose)
      fprintf(stderr,"action: %s\n", psz_action);
    return;
  }
  
  if (psz_action && strlen(psz_action))
    mvprintw(LINE_ACTION, 0, (char *) "action : %s\n", psz_action);
  else
    mvprintw(LINE_ACTION, 0, (char *) "");
  clrtoeol();
  refresh();
}

static void inline
xperror(const char *psz_msg)
{
  char line[80];
  
  if (!interactive) {
    if (b_verbose) {
      fprintf(stderr, "error: ");
      perror(psz_msg);
    }
    return;
  }
  
  if (b_verbose) {
    sprintf(line,"%s: %s", psz_msg, strerror(errno));
    mvprintw(LINE_ACTION, 0, (char *) "error  : %s", line);
    clrtoeol();
    refresh();
    select_wait(3);
    action(NULL);
  }
}

static void 
oops(const char *psz_msg, int rc)
{
  if (interactive) {
    mvprintw(LINE_LAST, 0, (char *) "%s, exiting...\n", psz_msg);
    clrtoeol();
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

/*! Stop playing audio CD */
static bool
cd_stop(CdIo_t *p_cdio)
{
  bool b_ok = true;
  if (b_cd && p_cdio) {
    action("stop...");
    i_last_audio_track = CDIO_INVALID_TRACK;
    b_ok = DRIVER_OP_SUCCESS == cdio_audio_stop(p_cdio);
    if ( !b_ok )
      xperror("stop");
  }
  return b_ok;
}

/*! Eject CD */
static bool
cd_eject(void)
{
  bool b_ok = true;
  if (p_cdio) {
    cd_stop(p_cdio);
    action("eject...");
    b_ok = DRIVER_OP_SUCCESS == cdio_eject_media(&p_cdio);
    if (!b_ok)
      xperror("eject");
    b_cd = 0;
    p_cdio = NULL;
  }
  return b_ok;
}

/*! Close CD tray */
static bool
cd_close(const char *psz_device)
{
  bool b_ok = true;
  if (!b_cd) {
    action("close...");
    b_ok = DRIVER_OP_SUCCESS == cdio_close_tray(psz_device, &driver_id);
    if (!b_ok)
      xperror("close");
  }
  return b_ok;
}

/*! Pause playing audio CD */
static bool
cd_pause(CdIo_t *p_cdio)
{
  bool b_ok = true;
  if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY) {
    b_ok = DRIVER_OP_SUCCESS == cdio_audio_pause(p_cdio);
    if (!b_ok)
      xperror("pause");
  }
  return b_ok;
}

/*! Get status/track/position info of an audio CD */
static bool
read_subchannel(CdIo_t *p_cdio)
{
  bool b_ok = true;
  if (!b_cd) return false;

  b_ok = DRIVER_OP_SUCCESS == cdio_audio_read_subchannel(p_cdio, &sub);
  if (!b_ok) {
    xperror("read subchannel");
    b_cd = 0;
  }
  if (auto_mode && sub.audio_status == CDIO_MMC_READ_SUB_ST_COMPLETED)
    cd_eject();
  return b_ok;
}

#define add_cddb_disc_info(format_str, field)  \
  if (p_cddb_disc->field && !strlen(field))  { \
    snprintf(field, sizeof(field), format_str, p_cddb_disc->field); \
    b_cdtext_ ## field = false;                                     \
  }

static void 
get_cddb_disc_info(CdIo_t *p_cdio) 
{
#ifdef HAVE_CDDB
  b_db = init_cddb(p_cdio, &p_conn, &p_cddb_disc, xperror, i_first_track, 
		   i_tracks, &i_cddb_matches);
  if (b_db) {
    add_cddb_disc_info("%s",  artist);
    add_cddb_disc_info("%s",  title);
    add_cddb_disc_info("%s",  genre);
    add_cddb_disc_info("%4d", year);
  }
#endif
  return;
}

#define add_cdtext_disc_info(format_str, info_field, FIELD) \
  if (p_cdtext->field[FIELD] && !strlen(info_field)) {      \
    snprintf(info_field, sizeof(info_field), format_str,    \
	     p_cdtext->field[FIELD]);                       \
    b_cdtext_ ## info_field = true;                         \
  }

static void 
get_cdtext_disc_info(CdIo_t *p_cdio) 
{
  cdtext_t *p_cdtext = cdio_get_cdtext(p_cdio, 0);

  if (p_cdtext) {
    add_cdtext_disc_info("%s", title, CDTEXT_TITLE);
    add_cdtext_disc_info("%s", artist, CDTEXT_PERFORMER);
    add_cdtext_disc_info("%s", genre, CDTEXT_GENRE);
    cdtext_destroy(p_cdtext);
  }
}

static void 
get_disc_info(CdIo_t *p_cdio) 
{
  b_db = false;
  if (b_prefer_cdtext) {
    get_cdtext_disc_info(p_cdio);
    get_cddb_disc_info(p_cdio);
  } else {
    get_cddb_disc_info(p_cdio);
    get_cdtext_disc_info(p_cdio);
  }
}

/*! Read CD TOC  and set CD information. */
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


  if ( DRIVER_OP_SUCCESS == cdio_audio_get_volume(p_cdio, NULL) ) {
    for (i_vol_port=0; i_vol_port<4; i_vol_port++) {
      if (i_vol_port > 0) break;
    }
    if (4 == i_vol_port) 
      /* Didn't find a non-zero volume level, maybe we've got everthing muted.
	 Set port to 0 to distinguis from -1 (driver can't get volume).
      */
      i_vol_port = 0;
  }
  
  if ( CDIO_INVALID_TRACK == i_first_track ||
       CDIO_INVALID_TRACK == i_last_track ) {
    xperror("read toc header");
    b_cd = false;
    b_record = false;
    i_last_display_track = CDIO_INVALID_TRACK;
  } else {
    b_cd = true;
    i_data = 0;
    get_disc_info(p_cdio);
    for (i = i_first_track; i <= i_last_track+1; i++) {
      int s;
      if ( !cdio_get_track_msf(p_cdio, i, &(toc[i])) )
      {
	xperror("read toc entry");
	b_cd = false;
	return;
      }
      if ( TRACK_FORMAT_AUDIO == cdio_get_track_format(p_cdio, i) ) {
	
	if (i != i_first_track) 
	  {
	    s = cdio_audio_get_msf_seconds(&toc[i]) 
	      - cdio_audio_get_msf_seconds(&toc[i-1]);
	    snprintf(cd_info[i-1].length, sizeof(cd_info[0].length), 
		     "%02d:%02d",
		     s / CDIO_CD_SECS_PER_MIN,  s % CDIO_CD_SECS_PER_MIN);
	  }
      } else {
	if ((i != i_last_track+1) ) {
	  i_data++;
	  if (i == i_first_track) {
	    if (i == i_last_track)
	      i_first_audio_track = CDIO_CDROM_LEADOUT_TRACK;
	    else
	      i_first_audio_track++;
	  }
	}
      }
      get_track_info(i);
    }
    b_record = true;
    read_subchannel(p_cdio);
    if (auto_mode && sub.audio_status != CDIO_MMC_READ_SUB_ST_PLAY)
      play_track(1, CDIO_CDROM_LEADOUT_TRACK);
  }
  action(NULL);
  display_cdinfo(p_cdio, i_tracks, i_first_track);
}

/*! Play an audio track. */
static bool
play_track(track_t i_start_track, track_t i_end_track)
{
  bool b_ok = true;
  char line[80];
  
  if (!b_cd) {
    cd_close(psz_device);
    read_toc(p_cdio);
  }
  
  read_subchannel(p_cdio);
  if (!b_cd || i_first_track == CDIO_CDROM_LEADOUT_TRACK)
    return false;
  
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
  b_ok = (DRIVER_OP_SUCCESS == cdio_audio_play_msf(p_cdio, 
						   &(toc[i_start_track]),
						   &(toc[i_end_track])) );
  if (!b_ok) xperror("play");
  return b_ok;
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

static bool
toggle_pause()
{
  bool b_ok = true;
  if (!b_cd) return false;
  
  if (CDIO_MMC_READ_SUB_ST_PAUSED == sub.audio_status) {
    b_ok = DRIVER_OP_SUCCESS != cdio_audio_resume(p_cdio);
    if (!b_ok)
      xperror("resume");
  } else {
    b_ok = DRIVER_OP_SUCCESS != cdio_audio_pause(p_cdio);
    if (!b_ok)
      xperror("pause");
  }
  return b_ok;
}

/*! Update windows with status and possibly track info. This used in 
  interactive playing not batch mode.
 */
static void
display_status(bool b_status_only)
{
  char line[80];

  if (!interactive) return;

  if (!b_cd) {
    sprintf(line,"no CD in drive (%s)", psz_device);

  } else if (i_first_track == CDIO_CDROM_LEADOUT_TRACK) {
    sprintf(line,"CD has only data tracks");
    
  } else if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
	     sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY) {
    cdio_audio_volume_t audio_volume;
    if (i_vol_port > 0 && 
	DRIVER_OP_SUCCESS == cdio_audio_get_volume(p_cdio, &audio_volume) ) 
      {
	uint8_t i_level = audio_volume.level[i_vol_port];
	sprintf(line,
		"track %2d - %02d:%02d of %s (%02d:%02d abs) %s volume: %d",
		sub.track,
		sub.rel_addr.msf.m,
		sub.rel_addr.msf.s,
		cd_info[sub.track].length,
		sub.abs_addr.msf.m,
		sub.abs_addr.msf.s,
		mmc_audio_state2str(sub.audio_status),
		(i_level*100+128) / 256 );
      
      } else 
	sprintf(line,"track %2d - %02d:%02d of %s (%02d:%02d abs) %s",
		sub.track,
		sub.rel_addr.msf.m,
		sub.rel_addr.msf.s,
		cd_info[sub.track].length,
		sub.abs_addr.msf.m,
		sub.abs_addr.msf.s,
	      mmc_audio_state2str(sub.audio_status));
  } else {
    sprintf(line,"%s", mmc_audio_state2str(sub.audio_status));
    
  }
  mvprintw(LINE_STATUS, 0, (char *) "status%s: %s",auto_mode ? "*" : " ", line);
  clrtoeol();
  
  if ( !b_status_only && b_db && i_last_display_track != sub.track && 
       (sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
	sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)  &&
	b_cd ) {
    i_last_display_track = sub.track;
    const cd_track_info_rec_t *p_cd_info = &cd_info[sub.track];
    if (i_first_audio_track != sub.track && 
	strlen(cd_info[sub.track-1].title)) {
      const cd_track_info_rec_t *p_cd_info = &cd_info[sub.track-1];
      mvprintw(LINE_TRACK_PREV, 0, (char *) "track %2d title  : %s [%s]",
	       sub.track-1, p_cd_info->title, 
	       p_cd_info->b_cdtext ? "CD-Text" : "CDDB");
      clrtoeol();
    } else {
      mvprintw(LINE_TRACK_PREV, 0, (char *) "%s","");
      clrtoeol();
    }
    if (strlen(p_cd_info->title)) {
      mvprintw(LINE_TRACK_TITLE, 0, (char *) "track %2d title  : %s [%s]", 
	       sub.track, p_cd_info->title, 
	       (char *) p_cd_info->b_cdtext ? "CD-Text" : "CDDB");
      clrtoeol();
    }
    if (strlen(p_cd_info->artist)) {
      mvprintw(LINE_TRACK_ARTIST, 0, (char *) "track %2d artist : %s [%s]",
	       sub.track, p_cd_info->artist,
	       p_cd_info->b_cdtext ? "CD-Text" : "CDDB");
      clrtoeol();
    }
    if (i_last_audio_track != sub.track && 
	strlen(cd_info[sub.track+1].title)) {
      const cd_track_info_rec_t *p_cd_info = &cd_info[sub.track+1];
      mvprintw(LINE_TRACK_NEXT, 0, (char *) "track %2d title  : %s [%s]", 
	       sub.track+1, p_cd_info->title,
	       p_cd_info->b_cdtext ? "CD-Text" : "CDDB");
      clrtoeol();
    } else {
      mvprintw(LINE_TRACK_NEXT, 0, (char *) "%s","");
      clrtoeol();
    }
    clrtobot();
  }

  refresh();

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

#define add_cdtext_track_info(format_str, info_field, FIELD) \
  if (p_cdtext->field[FIELD]) {                              \
    snprintf(cd_info[i_track].info_field,                    \
	     sizeof(cd_info[i_track].info_field),            \
	     format_str, p_cdtext->field[FIELD]);            \
    cd_info[i_track].b_cdtext = true; \
  }


static void
get_cdtext_track_info(track_t i_track)
{

  cdtext_t *p_cdtext = cdio_get_cdtext(p_cdio, i_track);

  if (NULL != p_cdtext) {
    add_cdtext_track_info("%s", title, CDTEXT_TITLE);
    add_cdtext_track_info("%s", title, CDTEXT_PERFORMER);
    cdtext_destroy(p_cdtext);
  }
}

static void
get_track_info(track_t i_track)
{

  if (b_prefer_cdtext) {
    get_cdtext_track_info(i_track);
    get_cddb_track_info(i_track);
  } else {
    get_cddb_track_info(i_track);
    get_cdtext_track_info(i_track);
  }
}

#define display_line(LINE_NO, COL_NO, format_str, field)   \
  if (field && field[0])  {                                \
    mvprintw(LINE_NO, COL_NO, (char *) format_str " [%s]", \
	     field,                                        \
	     b_cdtext_ ## field ? "CD-Text": "CDDB");      \
    clrtoeol();                                            \
  }
    
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
    if (i_data && i_first_track != CDIO_CDROM_LEADOUT_TRACK)
      sprintf(line+len,", audio=%u-%u", (unsigned int) i_first_audio_track,
	      (unsigned int) i_last_audio_track);
    
    display_line(LINE_ARTIST, 0, "CD Artist       : %s", artist);
    display_line(LINE_CDNAME, 0, "CD Title        : %s", title);
    display_line(LINE_GENRE,  0, "CD Genre        : %s", genre);
    display_line(LINE_YEAR,   0, "CD Year         : %s", year);
  }
  
  mvprintw(LINE_CDINFO, 0, (char *) "CD info: %0s", line);
  clrtoeol();
  refresh();
}
    
/* ---------------------------------------------------------------------- */

static void
usage(char *prog)
{
    fprintf(stderr,
	    "%s is a simple curses CD player.  It can pick up artist,\n"
	    "CD name and song title from CD-Text info on the CD or\n"
	    "via CDDB.\n"
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
	    "for non-interactive use (only one) of these:\n"
	    "  -l      list tracks\n"
	    "  -c      print cover (PostScript to stdout)\n"
	    "  -C      close CD-ROM tray. If you use this option,\n"
	    "          a CD-ROM device name must be specified.\n"
	    "  -p      play the whole CD\n"
	    "  -t n    play track >n<\n"
	    "  -t a-b  play all tracks between a and b (inclusive)\n"
	    "  -L      set volume level\n"
	    "  -s      stop playing\n"
	    "  -S      list audio subchannel information\n"
	    "  -e      eject cdrom\n"
            "\n"
	    "That's all. Oh, maybe a few words more about the auto-mode. This\n"
	    "is the 'dont-touch-any-key' feature. You load a CD, player starts\n"
	    "to play it, and when it is done it ejects the CD. Start it that\n"
	    "way on a spare console and forget about it...\n"
	    "\n"
	    "(c) 1997,98 Gerd Knorr <kraxel@goldbach.in-berlin.de>\n"
	    "(c) 2005 Rocky Bernstein <rocky@panix.com>\n"
	    , prog, prog);
}

static void
print_keys()
{
  unsigned int i;
  for (i=0; i < i_key_bindings; i++)
    fprintf(stderr, "%s\n", key_bindings[i]);
}

static void 
keypress_wait(CdIo_t *p_cdio)
  {
    int key;
    action("press any key to continue");
    while (1 != select_wait(b_cd ? 1 : 5)) {
      read_subchannel(p_cdio);
      display_status(true);
    }
    key = getch();
    clrtobot();
    action(NULL);
    display_cdinfo(p_cdio, i_tracks, i_first_track);
    i_last_display_track = CDIO_INVALID_TRACK;
  }

static void
list_keys()
{
  unsigned int i;
  for (i=0; i < i_key_bindings; i++) {
    mvprintw(LINE_TRACK_PREV+i, 0, (char *) "%s", key_bindings[i]);
    clrtoeol();
  }
  keypress_wait(p_cdio);
}

static void
list_tracks(void)
{
  track_t i;
  int i_line=0;
  int s;

  if (b_record) {
    i_line=LINE_TRACK_PREV - 1;
    for (i = i_first_track; i <= i_last_track; i++) {
      char line[80];
      s = cdio_audio_get_msf_seconds(&toc[i+1]) 
	- cdio_audio_get_msf_seconds(&toc[i]);
      sprintf(line, "%2d  %02d:%02d  %s  ", i, 
	      s / CDIO_CD_SECS_PER_MIN,  s % CDIO_CD_SECS_PER_MIN,
	      (sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY &&
	       sub.track == i) ? "*" : "|");
      if (b_record) {
	if ( strlen(cd_info[i].title) )
	  strcat(line, cd_info[i].title);
	if ( strlen(cd_info[i].artist) > 0 ) {
	  if (strlen(cd_info[i].title))
	    strcat(line, " / ");
	  strcat(line, cd_info[i].artist);
	}
      }
      mvprintw(i_line++, 0, line);
      clrtoeol();
    }
    keypress_wait(p_cdio);
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
ps_list_tracks(void)
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

typedef enum {
  PLAY_CD=1,
  PLAY_TRACK=2,
  STOP_PLAYING=3,
  EJECT_CD=4,
  CLOSE_CD=5,
  SET_VOLUME=6,
  LIST_SUBCHANNEL=7,
  LIST_KEYS=8,
  LIST_TRACKS=9,
  PS_LIST_TRACKS=10
} cd_operation_t;

int
main(int argc, char *argv[])
{    
  int  c, nostop=0;
  char *h;
  int  i_rc = 0;
  int  i_volume_level;
  cd_operation_t todo; /* operation to do in non-interactive mode */
  
  psz_program = strrchr(argv[0],'/');
  psz_program = psz_program ? psz_program+1 : argv[0];

  memset(&cddb_opts, 0, sizeof(cddb_opts));
  
  cdio_loglevel_default = CDIO_LOG_WARN;
  /* parse options */
  while ( 1 ) {
    if (-1 == (c = getopt(argc, argv, "acCdehkplL:sSt:vx")))
      break;
    switch (c) {
    case 'v':
      b_verbose = true;
      if (cdio_loglevel_default > CDIO_LOG_INFO) 
	cdio_loglevel_default = CDIO_LOG_INFO;
      break;
    case 'd':
      debug = 1;
      if (cdio_loglevel_default > CDIO_LOG_DEBUG) 
      cdio_loglevel_default = CDIO_LOG_DEBUG;
      break;
    case 'a':
      auto_mode = 1;
      break;
    case 'L':
      if (NULL != (h = strchr(optarg,'-'))) {
	i_volume_level = atoi(optarg);
	todo = SET_VOLUME;
      }
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
    case 'S':
      todo = LIST_SUBCHANNEL;
      break;
    case 'e':
      interactive = 0;
      todo = EJECT_CD;
      break;
    case 'k':
      print_keys();
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

  if (p_cdio && b_verbose)
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
      i_rc = cd_eject() ? 0 : 1;
    } else {
      read_toc(p_cdio);
      if (!b_cd) {
	cd_close(psz_device);
	read_toc(p_cdio);
      }
      if (b_cd)
	switch (todo) {
	case STOP_PLAYING:
	  i_rc = cd_stop(p_cdio) ? 0 : 1;
	  break;
	case EJECT_CD:
	  /* Should have been handled above. */
	  cd_eject();
	  break;
	case PS_LIST_TRACKS:
	  ps_list_tracks();
	  break;
	case PLAY_TRACK:
	  /* play just this one track */
	  if (b_record) {
	    printf("%s / %s\n", artist, title);
	    if (one_track)
	      printf("%s\n", cd_info[start_track].title);
	  }
	  i_rc = play_track(start_track, stop_track) ? 0 : 1;
	  break;
	case PLAY_CD:
	  if (b_record)
	    printf("%s / %s\n", artist, title);
	  play_track(1,CDIO_CDROM_LEADOUT_TRACK);
	  break;
	case CLOSE_CD:
	  i_rc = cdio_close_tray(psz_device, NULL) ? 0 : 1;
	  break;
	case SET_VOLUME:
	  {
	    cdio_audio_volume_t volume;
	    volume.level[0] = i_volume_level;
	    i_rc = (DRIVER_OP_SUCCESS == cdio_audio_set_volume(p_cdio, 
							       &volume))
	      ? 0 : 1;
	    break;
	  }
	case LIST_SUBCHANNEL: 
	  if (read_subchannel(p_cdio)) {
	    if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PAUSED ||
		sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY) {
	      {
		printf("track %2d - %02d:%02d (%02d:%02d abs) ",
		       sub.track,
		       sub.rel_addr.msf.m,
		       sub.rel_addr.msf.s,
		       sub.abs_addr.msf.m,
		       sub.abs_addr.msf.s);
	      }
	    }
	    printf("drive state: %s\n", 
		   mmc_audio_state2str(sub.audio_status));
	  } else {
	    i_rc = 1;
	  }
	  break;
	case LIST_KEYS:
	case LIST_TRACKS:
	  break;
	}
      else {
	fprintf(stderr,"no CD in drive (%s)\n", psz_device);
      }
    }
  }
  
  while ( !b_sig ) {
    int key;
    if (!b_cd) read_toc(p_cdio);
    read_subchannel(p_cdio);
    display_status(false);
    
    if (1 == select_wait(b_cd ? 1 : 5)) {
      switch (key = getch()) {
      case 'A':
      case 'a':
	auto_mode = !auto_mode;
	break;
      case 'X':
      case 'x':
	nostop=1;
	/* fall through */
      case 'Q':
      case 'q':
	b_sig = true;
	break;
      case 'E':
      case 'e':
	cd_eject();
	break;
      case 's':
	cd_stop(p_cdio);
	break;
      case 'C':
      case 'c':
	cd_close(psz_device);
	break;
      case 'L':
      case 'l':
	list_tracks();
	break;
      case 'K':
      case 'k':
      case 'h':
      case 'H':
      case '?':
	list_keys();
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
	play_track(key - '0', CDIO_CDROM_LEADOUT_TRACK);
	break;
      case '0':
	play_track(10, CDIO_CDROM_LEADOUT_TRACK);
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
	play_track(key - KEY_F(1) + 11, CDIO_CDROM_LEADOUT_TRACK);
	break;
      }
    }
  }
  if (!nostop) cd_stop(p_cdio);
  tty_restore();
  oops("bye", i_rc);
  
  return 0; /* keep compiler happy */
}
