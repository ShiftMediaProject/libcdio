/*
    $Id: _cdio_win32.h,v 1.3 2004/02/05 03:02:16 rocky Exp $

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

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  HANDLE h_device_handle; /* device descriptor */
  long  hASPI;
  short i_sid;
  long  (*lpSendCommand)( void* );

  /* Track information */
  bool toc_init;                 /* if true, info below is valid. */
  track_info_t  tocent[100];     /* entry info for each track */
  track_t       total_tracks;    /* number of tracks in image */
  track_t       first_track_num; /* track number of first track */

} _img_private_t;

/*!
   Reads an audio device using the DeviceIoControl method into data
   starting from lsn.  Returns 0 if no error.
 */
int win32ioctl_read_audio_sectors (_img_private_t *obj, void *data, lsn_t lsn, 
				   unsigned int nblocks);
/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
win32ioctl_read_mode2_sector (_img_private_t *env, void *data, lsn_t lsn, 
			      bool mode2_form2);

const char *win32ioctl_is_cdrom(const char drive_letter);

/*!
  Initialize internal structures for CD device.
 */
bool win32ioctl_init_win32 (_img_private_t *env);

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool win32ioctl_read_toc (_img_private_t *env);

char *win32ioctl_get_mcn (_img_private_t *env);

/*!  
  Get the format (XA, DATA, AUDIO) of a track. 
*/
track_format_t win32ioctl_get_track_format(_img_private_t *env, 
					   track_t track_num); 
