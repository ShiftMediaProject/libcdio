/*
    $Id: win32.h,v 1.6 2004/06/21 03:22:58 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>

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

#include "cdio_private.h"

#pragma pack()

typedef struct {
  lsn_t          start_lsn;
  UCHAR          Control : 4;
  UCHAR          Format;
} track_info_t;

typedef enum {
  _AM_NONE,
  _AM_IOCTL,
  _AM_ASPI,
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  access_mode_t access_mode;

  bool b_ioctl_init;
  bool b_aspi_init;

  HANDLE h_device_handle; /* device descriptor */
  long  hASPI;
  short i_sid;
  long  (*lpSendCommand)( void* );

  /* Track information */
  bool toc_init;                 /* if true, info below is valid. */
  track_info_t  tocent[100];     /* entry info for each track */
  track_t       total_tracks;    /* number of tracks in image */
  track_t       i_first_track;   /* track number of first track */

} _img_private_t;

/*!
   Reads an audio device using the DeviceIoControl method into data
   starting from lsn.  Returns 0 if no error.
*/
int read_audio_sectors_win32ioctl (_img_private_t *obj, void *data, lsn_t lsn, 
				   unsigned int nblocks);
/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int read_mode2_sector_win32ioctl (const _img_private_t *env, void *data, 
				  lsn_t lsn, bool b_form2);

/*!
   Reads a single mode1 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int read_mode1_sector_win32ioctl (const _img_private_t *env, void *data, 
				  lsn_t lsn, bool b_form2);

const char *is_cdrom_win32ioctl (const char drive_letter);

/*!
  Initialize internal structures for CD device.
 */
bool init_win32ioctl (_img_private_t *env);

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool read_toc_win32ioctl (_img_private_t *env);

char *get_mcn_win32ioctl (const _img_private_t *env);

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
cdio_drive_cap_t get_drive_cap_aspi (const _img_private_t *env);

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
cdio_drive_cap_t get_drive_cap_win32ioctl (const _img_private_t *env);

/*!  
  Get the format (XA, DATA, AUDIO) of a track. 
*/
track_format_t get_track_format_win32ioctl(const _img_private_t *env, 
					   track_t i_track); 
