/*
  $Id: cdda.h,v 1.2 2005/01/06 23:34:13 rocky Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 2001 Xiph.org
  and Heiko Eissfeldt heiko@escape.colossus.de

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
/** \file cdda.h
 *  \brief The top-level interface header for cd-paranoia; applications 
 *  include this.
 *
 ******************************************************************/

#ifndef _CDDA_INTERFACE_H_
#define _CDDA_INTERFACE_H_

#include <cdio/cdio.h>
#include <cdio/paranoia.h>

#define CD_FRAMESAMPLES (CDIO_CD_FRAMESIZE_RAW / 4)

#include <sys/types.h>
#include <signal.h>

/** We keep MAXTRK since this header is exposed publicly and other
   programs may have used this.
*/
#define MAXTRK (CDIO_CD_MAX_TRACKS+1)

typedef struct TOC {	/* structure of table of contents */
  unsigned char bTrack;
  int32_t       dwStartSector;
} TOC_t;

/** For compatibility. TOC is depricated, use TOC_t instead. */
#define TOC TOC_t

/** interface types */
#define GENERIC_SCSI	0
#define COOKED_IOCTL	1
#define TEST_INTERFACE	2

#define CDDA_MESSAGE_FORGETIT 0
#define CDDA_MESSAGE_PRINTIT 1
#define CDDA_MESSAGE_LOGIT 2

/** cdrom access function pointer */

struct cdrom_drive_s {

  CdIo_t *p_cdio;
  int opened; /* This struct may just represent a candidate for opening */

  char *cdda_device_name;

  char *drive_model;
  int interface;
  int bigendianp;
  int nsectors;

  int cd_extra;
  track_t tracks;
  TOC_t disc_toc[MAXTRK];
  lsn_t audio_first_sector;
  lsn_t audio_last_sector;

  int errordest;
  int messagedest;
  char *errorbuf;
  char *messagebuf;

  /* functions specific to particular drives/interrfaces */

  int  (*enable_cdda)  (cdrom_drive_t *d, int onoff);
  int  (*read_toc)     (cdrom_drive_t *d);
  long (*read_audio)   (cdrom_drive_t *d, void *p, lsn_t begin, 
		       long sectors);
  int  (*set_speed)    (cdrom_drive_t *d, int speed);
  int error_retry;
  int report_all;

  int is_atapi;
  int is_mmc;

  /* SCSI command buffer and offset pointers */
  unsigned char *sg;
  unsigned char *sg_buffer;
  unsigned char inqbytes[4];

  /* Scsi parameters and state */
  unsigned char density;
  unsigned char orgdens;
  unsigned int orgsize;
  long bigbuff;
  int adjust_ssize;

  int fua;
  int lun;

  sigset_t sigset;

};

/** autosense functions */

extern cdrom_drive_t *cdda_find_a_cdrom(int messagedest, char **message);
extern cdrom_drive_t *cdda_identify(const char *device, int messagedest,
				  char **message);
extern cdrom_drive_t *cdda_identify_cooked(const char *device,int messagedest,
					 char **message);
#ifdef CDDA_TEST
extern cdrom_drive_t *cdda_identify_test(const char *filename,
				       int messagedest, char **message);
#endif

/**  oriented functions */

extern int     cdda_speed_set(cdrom_drive_t *d, int speed);
extern void    cdda_verbose_set(cdrom_drive_t *d, int err_action, 
				int mes_action);
extern char   *cdda_messages(cdrom_drive_t *d);
extern char   *cdda_errors(cdrom_drive_t *d);

extern int     cdda_close(cdrom_drive_t *d);
extern int     cdda_open(cdrom_drive_t *d);
extern long    cdda_read(cdrom_drive_t *d, void *p_buffer,
			 lsn_t beginsector, long sectors);

/*! Return the lsn for the start of track i_track */
extern lsn_t   cdda_track_firstsector(cdrom_drive_t *d, track_t i_track);

/*! Get last lsn of the track. This generally one less than the start
  of the next track. -1 is returned on error. */
extern lsn_t   cdda_track_lastsector(cdrom_drive_t *d, track_t i_track);

/*! Return the number of tracks on the CD. */
extern track_t cdda_tracks(cdrom_drive_t *d);

/*! Return the track containing the given LSN. If the LSN is before
    the first track (in the pregap), 0 is returned. If there was an
    error or the LSN after the LEADOUT (beyond the end of the CD), then
    CDIO_INVALID_TRACK is returned.
 */
extern int     cdda_sector_gettrack(cdrom_drive_t *d, lsn_t lsn);

/*! Return the number of channels in track: 2 or 4; -2 if not
  implemented or -1 for error.
  Not meaningful if track is not an audio track.
*/
extern int     cdda_track_channels(cdrom_drive_t *d, track_t i_track);

/*! Return 1 is track is an audio track, 0 otherwise. */
extern int     cdda_track_audiop(cdrom_drive_t *d, track_t i_track);

/*! Return 1 is track has copy permit set, 0 otherwise. */
extern int     cdda_track_copyp(cdrom_drive_t *d, track_t i_track);

/*! Return 1 is audio track has linear preemphasis set, 0 otherwise. 
    Only makes sense for audio tracks.
 */
extern int     cdda_track_preemp(cdrom_drive_t *d, track_t i_track);

/*! Get first lsn of the first audio track. -1 is returned on error. */
extern lsn_t   cdda_disc_firstsector(cdrom_drive_t *d);

/*! Get last lsn of the last audio track. The last lssn generally one
  less than the start of the next track after the audio track. -1 is
  returned on error. */
extern lsn_t   cdda_disc_lastsector(cdrom_drive_t *d);

/** transport errors: */

#define TR_OK            0
#define TR_EWRITE        1  /**< Error writing packet command (transport) */
#define TR_EREAD         2  /**< Error reading packet data (transport) */
#define TR_UNDERRUN      3  /**< Read underrun */
#define TR_OVERRUN       4  /**< Read overrun */
#define TR_ILLEGAL       5  /**< Illegal/rejected request */
#define TR_MEDIUM        6  /**< Medium error */
#define TR_BUSY          7  /**< Device busy */
#define TR_NOTREADY      8  /**< Device not ready */
#define TR_FAULT         9  /**< Device failure */
#define TR_UNKNOWN      10  /**< Unspecified error */
#define TR_STREAMING    11  /**< loss of streaming */

#ifdef NEED_STRERROR_TR
const char *strerror_tr[]={
  "Success",
  "Error writing packet command to device",
  "Error reading command from device",
  "SCSI packet data underrun (too little data)",
  "SCSI packet data overrun (too much data)",
  "Illegal SCSI request (rejected by target)",
  "Medium reading data from medium",
  "Device busy",
  "Device not ready",
  "Target hardware fault",
  "Unspecified error",
  "Drive lost streaming"
};
#endif /*NEED_STERROR_TR*/

/** Errors returned by lib: 

001: Unable to set CDROM to read audio mode
002: Unable to read table of contents lead-out
003: CDROM reporting illegal number of tracks
004: Unable to read table of contents header
005: Unable to read table of contents entry
006: Could not read any data from drive
007: Unknown, unrecoverable error reading data
008: Unable to identify CDROM model
009: CDROM reporting illegal table of contents
010: Unaddressable sector 

100: Interface not supported
101: Drive is neither a CDROM nor a WORM device
102: Permision denied on cdrom (ioctl) device
103: Permision denied on cdrom (data) device

300: Kernel memory error

400: Device not open
401: Invalid track number
402: Track not audio data
403: No audio tracks on disc

*/
#endif /*_CDDA_INTERFACE_H_*/

