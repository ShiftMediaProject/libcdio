/*
    $Id: audio.c,v 1.1 2005/03/15 04:18:05 rocky Exp $

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

#include <signal.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <cdio/util.h>
#include <cdio/cd_types.h>

static void play_track(track_t t1, track_t t2);

typedef enum {
  PLAY_CD=1,
  PLAY_TRACK=2,
  STOP_PLAYING=3,
  EJECT_CD=4,
  CLOSE_CD=5,
} cd_operation_t;

CdIo_t             *p_cdio;               /* libcdio handle */
driver_id_t        driver_id = DRIVER_DEVICE;

/* cdrom data */
track_t            i_first_track;
track_t            i_last_track;
track_t            i_first_audio_track;
track_t            i_last_audio_track;
track_t            i_tracks;
msf_t              toc[CDIO_CDROM_LEADOUT_TRACK+1];
cd_operation_t     todo; /* operation to do in non-interactive mode */
cdio_subchannel_t  sub;      /* subchannel last time read */
int                i_data;     /* # of data tracks present ? */
int                start_track = 0;
int                stop_track = 0;
int                one_track = 0;

bool               b_cd         = false;
bool               auto_mode    = false;
bool               b_verbose    = false;
bool               debug        = false;
bool               b_record = false; /* we have a record for
					the inserted CD */

char *psz_device=NULL;
char *psz_program;

static void inline
xperror(const char *psz_msg)
{
  if (b_verbose) {
    fprintf(stderr, "error: ");
    perror(psz_msg);
  }
  return;
}


static void 
oops(const char *psz_msg, int rc)
{
  cdio_destroy (p_cdio);
  free (psz_device);
  exit (rc);
}

/* ---------------------------------------------------------------------- */

/*! Stop playing audio CD */
static void
cd_stop(CdIo_t *p_cdio)
{
  if (b_cd && p_cdio) {
    i_last_audio_track = CDIO_INVALID_TRACK;
    if (DRIVER_OP_SUCCESS != cdio_audio_stop(p_cdio))
      xperror("stop");
  }
}

/*! Eject CD */
static void
cd_eject(void)
{
  if (p_cdio) {
    cd_stop(p_cdio);
    if (DRIVER_OP_SUCCESS != cdio_eject_media(&p_cdio))
      xperror("eject");
    b_cd = 0;
    p_cdio = NULL;
  }
}

/*! Close CD tray */
static void
cd_close(const char *psz_device)
{
  if (!b_cd) {
    if (DRIVER_OP_SUCCESS != cdio_close_tray(psz_device, &driver_id))
      xperror("close");
  }
}

/*! Pause playing audio CD */
static void
cd_pause(CdIo_t *p_cdio)
{
  if (sub.audio_status == CDIO_MMC_READ_SUB_ST_PLAY)
    if (DRIVER_OP_SUCCESS != cdio_audio_pause(p_cdio))
      xperror("pause");
}

/*! Get status/track/position info of an audio CD */
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

/*! Read CD TOC  and set CD information. */
static void
read_toc(CdIo_t *p_cdio)
{
  track_t i;

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
    i_data = 0;
    for (i = i_first_track; i <= i_last_track+1; i++) {
      if ( !cdio_get_track_msf(p_cdio, i, &(toc[i])) )
      {
	xperror("read toc entry");
	b_cd = false;
	return;
      }
      if ( TRACK_FORMAT_AUDIO != cdio_get_track_format(p_cdio, i) ) {
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
    }
    b_record = true;
    read_subchannel(p_cdio);
    if (auto_mode && sub.audio_status != CDIO_MMC_READ_SUB_ST_PLAY)
      play_track(1, CDIO_CDROM_LEADOUT_TRACK);
  }
}

/*! Play an audio track. */
static void
play_track(track_t i_start_track, track_t i_end_track)
{
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
  if ( DRIVER_OP_SUCCESS != cdio_audio_play_msf(p_cdio, &(toc[i_start_track]),
						&(toc[i_end_track])) )
    xperror("play");
}

static void
usage(char *prog)
{
    fprintf(stderr,
	    "%s is a simple interface to issuing CD audio comamnds\n"
	    "\n"
	    "usage: %s [options] [device]\n"
            "\n"
	    "default for to search for a CD-ROM device with a CD-DA loaded\n"
	    "\n"
	    "These command line options available:\n"
	    "  -h      print this help\n"
	    "  -a      start up in auto-mode\n"
	    "  -v      verbose\n"
	    "\n"
	    " Use only one of these:\n"
	    "  -C      close CD-ROM tray. If you use this option,\n"
	    "          a CD-ROM device name must be specified.\n"
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
	    "(c) 1997,98 Gerd Knorr <kraxel@goldbach.in-berlin.de>\n"
	    "(c) 2005 Rocky Bernstein <rocky@panix.com>\n"
	    , prog, prog);
}

int
main(int argc, char *argv[])
{
  int             c, nostop=0;
  char            *h;
  
  psz_program = strrchr(argv[0],'/');
  psz_program = psz_program ? psz_program+1 : argv[0];

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
      todo = PLAY_TRACK;
      break;
    case 'p':
      todo = PLAY_CD;
      break;
    case 'C':
      todo = CLOSE_CD;
      break;
      break;
    case 's':
      todo = STOP_PLAYING;
      break;
    case 'e':
      todo = EJECT_CD;
      break;
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
  
  if (!b_cd && todo != EJECT_CD) {
    cd_close(psz_device);
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
  
  {
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
	case PLAY_TRACK:
	  /* play just this one track */
	  play_track(start_track, stop_track);
	  break;
	case PLAY_CD:
	  play_track(1,CDIO_CDROM_LEADOUT_TRACK);
	  break;
	case CLOSE_CD:
	  cdio_close_tray(psz_device, NULL);
	  break;
	}
      else {
	fprintf(stderr,"no CD in drive (%s)\n", psz_device);
      }
    }
  }
  
  if (!nostop) cd_stop(p_cdio);
  oops("bye", 0);
  
  return 0; /* keep compiler happy */
}
