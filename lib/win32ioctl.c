/*
    $Id: win32ioctl.c,v 1.2 2004/02/04 11:08:10 rocky Exp $

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

/* This file contains Win32-specific code using the DeviceIoControl
   access method.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: win32ioctl.c,v 1.2 2004/02/04 11:08:10 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include "cdio_assert.h"

#ifdef HAVE_WIN32_CDROM

#include <windows.h>
#include <winioctl.h>

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Win32 DeviceIoControl specifics */
/***** FIXME: #include ntddcdrm.h from Wine, but probably need to 
   modify it a little.
*/

#ifndef IOCTL_CDROM_BASE
#    define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#endif
#ifndef IOCTL_CDROM_RAW_READ
#define IOCTL_CDROM_RAW_READ CTL_CODE(IOCTL_CDROM_BASE, 0x000F, \
                                      METHOD_OUT_DIRECT, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_CDROM_BASE
#    define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#endif
#ifndef IOCTL_CDROM_READ_TOC
#    define IOCTL_CDROM_READ_TOC CTL_CODE(IOCTL_CDROM_BASE, 0x0000, \
                                          METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
#ifndef IOCTL_CDROM_RAW_READ
#define IOCTL_CDROM_RAW_READ CTL_CODE(IOCTL_CDROM_BASE, 0x000F, \
                                      METHOD_OUT_DIRECT, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_CDROM_READ_Q_CHANNEL
#define IOCTL_CDROM_READ_Q_CHANNEL      CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

typedef struct _TRACK_DATA {
    UCHAR Format;
    UCHAR Control : 4;
    UCHAR Adr : 4;
    UCHAR TrackNumber;
    UCHAR Reserved1;
    UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

typedef struct _CDROM_TOC {
    UCHAR Length[2];
    UCHAR FirstTrack;
    UCHAR LastTrack;
    TRACK_DATA TrackData[CDIO_CD_MAX_TRACKS+1];
} CDROM_TOC, *PCDROM_TOC;

#define IOCTL_CDROM_SUB_Q_CHANNEL    0x00
#define IOCTL_CDROM_CURRENT_POSITION 0x01
#define IOCTL_CDROM_MEDIA_CATALOG    0x02
#define IOCTL_CDROM_TRACK_ISRC       0x03

typedef enum _TRACK_MODE_TYPE {
    YellowMode2,
    XAForm2,
    CDDA
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

typedef struct __RAW_READ_INFO {
    LARGE_INTEGER DiskOffset;
    ULONG SectorCount;
    TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;

typedef struct _CDROM_SUB_Q_DATA_FORMAT {
    UCHAR               Format;
    UCHAR               Track;
} CDROM_SUB_Q_DATA_FORMAT, *PCDROM_SUB_Q_DATA_FORMAT;

typedef struct _SUB_Q_HEADER {
  UCHAR Reserved;
  UCHAR AudioStatus;
  UCHAR DataLength[2];
} SUB_Q_HEADER, *PSUB_Q_HEADER;

typedef struct _SUB_Q_MEDIA_CATALOG_NUMBER {
  SUB_Q_HEADER Header;
  UCHAR FormatCode;
  UCHAR Reserved[3];
  UCHAR Reserved1 : 7;
  UCHAR Mcval :1;
  UCHAR MediaCatalog[15];
} SUB_Q_MEDIA_CATALOG_NUMBER, *PSUB_Q_MEDIA_CATALOG_NUMBER;

#include "_cdio_win32.h"

const char *
win32ioctl_is_cdrom(const char drive_letter) 
{
  static char psz_win32_drive[7];
  static char root_path_name[8];
  _img_private_t env;

  /* Initializations */
  env.h_device_handle = NULL;
  env.i_sid = 0;
  env.hASPI = 0;
  env.lpSendCommand = 0;
  
  sprintf( psz_win32_drive, "\\\\.\\%c:", drive_letter );
  sprintf( root_path_name, "\\\\.\\%c:\\", drive_letter );
  
  env.h_device_handle = CreateFile( psz_win32_drive, GENERIC_READ,
				    FILE_SHARE_READ | FILE_SHARE_WRITE,
				    NULL, OPEN_EXISTING,
				    FILE_FLAG_NO_BUFFERING |
				    FILE_FLAG_RANDOM_ACCESS, NULL );
  if (env.h_device_handle != NULL 
      && (DRIVE_CDROM == GetDriveType(root_path_name))) {
    CloseHandle(env.h_device_handle);
    return strdup(psz_win32_drive);
  } else {
    CloseHandle(env.h_device_handle);
    return NULL;
  }
}
  
/*!
   Reads an audio device using the DeviceIoControl method into data
   starting from lsn.  Returns 0 if no error.
 */
int
win32ioctl_read_audio_sectors (_img_private_t *env, void *data, lsn_t lsn, 
			       unsigned int nblocks) 
{
  DWORD dwBytesReturned;
  RAW_READ_INFO cdrom_raw;
  
  /* Initialize CDROM_RAW_READ structure */
  cdrom_raw.DiskOffset.QuadPart = CDIO_CD_FRAMESIZE_RAW * lsn;
  cdrom_raw.SectorCount = nblocks;
  cdrom_raw.TrackMode = CDDA;
  
  if( DeviceIoControl( env->h_device_handle,
		       IOCTL_CDROM_RAW_READ, &cdrom_raw,
		       sizeof(RAW_READ_INFO), data,
		       CDIO_CD_FRAMESIZE_RAW * nblocks,
		       &dwBytesReturned, NULL ) == 0 ) {
    cdio_info("Error reading audio-mode %lu (%ld)\n", lsn, GetLastError());
    return 1;
  }
  return 0;
}

/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
win32ioctl_read_mode2_sector (_img_private_t *env, void *data, lsn_t lsn, 
			      bool mode2_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  DWORD dwBytesReturned;
  RAW_READ_INFO cdrom_raw;
  
  /* Initialize CDROM_RAW_READ structure */
  cdrom_raw.DiskOffset.QuadPart = CDIO_CD_FRAMESIZE * lsn;
  cdrom_raw.SectorCount = 1;
  cdrom_raw.TrackMode = mode2_form2 ? XAForm2 : YellowMode2;
  
  if( DeviceIoControl( env->h_device_handle,
		       IOCTL_CDROM_RAW_READ, &cdrom_raw,
		       sizeof(RAW_READ_INFO), buf,
		       sizeof(buf), &dwBytesReturned, NULL )
      == 0 ) {
    /* Retry in Yellowmode2 */
    if (mode2_form2) {
      cdio_debug("Retrying mode2 request as mode1");
      cdrom_raw.TrackMode = YellowMode2;
    } else {
      cdio_debug("Retrying mode1 request as mode2");
      cdrom_raw.TrackMode = XAForm2;
    }
    if( DeviceIoControl( env->h_device_handle,
			 IOCTL_CDROM_RAW_READ, &cdrom_raw,
			 sizeof(RAW_READ_INFO), buf,
			 sizeof(buf), &dwBytesReturned, NULL )
	== 0 ) {
      cdio_debug("Last-ditch effort reading as CDDA");
      cdrom_raw.TrackMode = CDDA;
      if( DeviceIoControl( env->h_device_handle,
			   IOCTL_CDROM_RAW_READ, &cdrom_raw,
			   sizeof(RAW_READ_INFO), buf,
			   sizeof(buf), &dwBytesReturned, NULL )
	  == 0 ) {
	cdio_info("Error reading %lu (%ld)\n", lsn, GetLastError());
	return 1;
      }
    }
  }

  if (mode2_form2)
    memcpy (data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
  
  return 0;
}

/*!
  Initialize internal structures for CD device.
 */
bool
win32ioctl_init_win32 (_img_private_t *env)
{
  char psz_win32_drive[7];
  unsigned int len=strlen(env->gen.source_name);
  
  cdio_debug("using winNT/2K/XP ioctl layer");
  
  if (cdio_is_device_win32(env->gen.source_name)) {
    sprintf( psz_win32_drive, "\\\\.\\%c:", env->gen.source_name[len-2] );
    
    env->h_device_handle = CreateFile( psz_win32_drive, GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING,
					FILE_FLAG_NO_BUFFERING |
					FILE_FLAG_RANDOM_ACCESS, NULL );
    return (env->h_device_handle == NULL) ? false : true;
  }
  return false;
}

#define MSF_TO_LBA2(min, sec, frame) \
  ((int) frame + CDIO_CD_FRAMES_PER_SEC * (CDIO_CD_SECS_PER_MIN*min + sec) \
         - CDIO_PREGAP_SECTORS )

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool
win32ioctl_read_toc (_img_private_t *env) 
{

  DWORD dwBytesReturned;
  CDROM_TOC cdrom_toc;
  int i;
  
  if( DeviceIoControl( env->h_device_handle,
		       IOCTL_CDROM_READ_TOC,
		       NULL, 0, &cdrom_toc, sizeof(CDROM_TOC),
		       &dwBytesReturned, NULL ) == 0 ) {
    cdio_warn( "could not read TOCHDR: %ld" , (long int) GetLastError());
    return false;
  }
  
  env->first_track_num = cdrom_toc.FirstTrack;
  env->total_tracks    = cdrom_toc.LastTrack - cdrom_toc.FirstTrack + 1;
  
  
  for( i = 0 ; i <= env->total_tracks ; i++ ) {
    env->tocent[ i ].start_lsn = MSF_TO_LBA2(
					     cdrom_toc.TrackData[i].Address[1],
					     cdrom_toc.TrackData[i].Address[2],
					     cdrom_toc.TrackData[i].Address[3] );
    env->tocent[ i ].Control   = cdrom_toc.TrackData[i].Control;
    env->tocent[ i ].Format    = cdrom_toc.TrackData[i].Format;
    cdio_debug("p_sectors: %i, %lu", i, 
	       (unsigned long int) (env->tocent[i].start_lsn));
  }
  env->gen.toc_init = true;
  return true;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
char *
win32ioctl_get_mcn (_img_private_t *env) {

  DWORD dwBytesReturned;
  SUB_Q_MEDIA_CATALOG_NUMBER mcn;
  CDROM_SUB_Q_DATA_FORMAT q_data_format;
  
  memset( &mcn, 0, sizeof(mcn) );
  
  q_data_format.Format = IOCTL_CDROM_MEDIA_CATALOG;
  q_data_format.Track=1;
  
  if( DeviceIoControl( env->h_device_handle,
		       IOCTL_CDROM_READ_Q_CHANNEL,
		       &q_data_format, sizeof(q_data_format), 
		       &mcn, sizeof(mcn),
		       &dwBytesReturned, NULL ) == 0 ) {
    cdio_warn( "could not read Q Channel at track 1");
  } else if (mcn.Mcval)
    return strdup(mcn.MediaCatalog);
  return NULL;
}

#endif /*HAVE_WIN32_CDROM*/
