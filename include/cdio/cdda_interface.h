/*
  $Id: cdda_interface.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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
/** \file cdda_paranoia.h
 *  \brief The top-level interface header; applications include this.
 *
 ******************************************************************/

#ifndef _CDDA_INTERFACE_H_
#define _CDDA_INTERFACE_H_

#include <cdio/paranoia.h>

#define CD_FRAMESAMPLES (CDIO_CD_FRAMESIZE_RAW / 4)

#include <sys/types.h>
#include <signal.h>

/** We keep MAXTRK since this header is exposed publicly and other
   programs may have used this.
*/
#define MAXTRK (CDIO_CD_MAX_TRACKS+1)

typedef struct TOC {	/* structure of table of contents */
  unsigned char bFlags;
  unsigned char bTrack;
  int32_t       dwStartSector;
} TOC;

/** interface types */
#define GENERIC_SCSI	0
#define COOKED_IOCTL	1
#define TEST_INTERFACE	2

#define CDDA_MESSAGE_FORGETIT 0
#define CDDA_MESSAGE_PRINTIT 1
#define CDDA_MESSAGE_LOGIT 2

/** cdrom access function pointer */

struct cdrom_drive_s {

  int opened; /* This struct may just represent a candidate for opening */

  char *cdda_device_name;
  char *ioctl_device_name;

  int cdda_fd;
  int ioctl_fd;

  char *drive_model;
  int drive_type;
  int interface;
  int bigendianp;
  int nsectors;

  int cd_extra;
  int tracks;
  TOC disc_toc[MAXTRK];
  long audio_first_sector;
  long audio_last_sector;

  int errordest;
  int messagedest;
  char *errorbuf;
  char *messagebuf;

  /* functions specific to particular drives/interrfaces */

  int  (*enable_cdda)  (cdrom_drive_t *d, int onoff);
  int  (*read_toc)     (cdrom_drive_t *d);
  long (*read_audio)   (cdrom_drive_t *d, void *p, long begin, 
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

#define IS_AUDIO(d,i) (!(d->disc_toc[i].bFlags & 0x04))

/** autosense functions */

extern cdrom_drive_t *cdda_find_a_cdrom(int messagedest, char **message);
extern cdrom_drive_t *cdda_identify(const char *device, int messagedest,
				  char **message);
extern cdrom_drive_t *cdda_identify_cooked(const char *device,int messagedest,
					 char **message);
extern cdrom_drive_t *cdda_identify_scsi(const char *generic_device, 
				       const char *ioctl_device,
				       int messagedest, char **message);
#ifdef CDDA_TEST
extern cdrom_drive_t *cdda_identify_test(const char *filename,
				       int messagedest, char **message);
#endif

/**  oriented functions */

extern int   cdda_speed_set(cdrom_drive_t *d, int speed);
extern void  cdda_verbose_set(cdrom_drive_t *d,int err_action, int mes_action);
extern char *cdda_messages(cdrom_drive_t *d);
extern char *cdda_errors(cdrom_drive_t *d);

extern int   cdda_close(cdrom_drive_t *d);
extern int   cdda_open(cdrom_drive_t *d);
extern long  cdda_read(cdrom_drive_t *d, void *buffer,
		       long int beginsector, long int sectors);

extern long cdda_track_firstsector(cdrom_drive_t *d,int track);
extern long cdda_track_lastsector(cdrom_drive_t *d,int track);
extern long cdda_tracks(cdrom_drive_t *d);
extern int  cdda_sector_gettrack(cdrom_drive_t *d,long sector);
extern int  cdda_track_channels(cdrom_drive_t *d,int track);
extern int  cdda_track_audiop(cdrom_drive_t *d,int track);
extern int  cdda_track_copyp(cdrom_drive_t *d,int track);
extern int  cdda_track_preemp(cdrom_drive_t *d,int track);
extern long cdda_disc_firstsector(cdrom_drive_t *d);
extern long cdda_disc_lastsector(cdrom_drive_t *d);

/** transport errors: */

#define TR_OK            0
#define TR_EWRITE        1  /* Error writing packet command (transport) */
#define TR_EREAD         2  /* Error reading packet data (transport) */
#define TR_UNDERRUN      3  /* Read underrun */
#define TR_OVERRUN       4  /* Read overrun */
#define TR_ILLEGAL       5  /* Illegal/rejected request */
#define TR_MEDIUM        6  /* Medium error */
#define TR_BUSY          7  /* Device busy */
#define TR_NOTREADY      8  /* Device not ready */
#define TR_FAULT         9  /* Devive failure */
#define TR_UNKNOWN      10  /* Unspecified error */
#define TR_STREAMING    11  /* loss of streaming */

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

