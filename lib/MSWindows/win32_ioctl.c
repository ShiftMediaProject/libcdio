/*
    $Id: win32_ioctl.c,v 1.1 2004/04/30 08:23:23 rocky Exp $

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

static const char _rcsid[] = "$Id: win32_ioctl.c,v 1.1 2004/04/30 08:23:23 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include "cdio_assert.h"

#ifdef HAVE_WIN32_CDROM

#include <windows.h>
#include <ddk/ntddstor.h>
#include <ddk/ntddscsi.h>
#include <ddk/scsi.h>

#include <stdio.h>
#include <stddef.h>  /* offsetof() macro */
#include <sys/stat.h>
#include <sys/types.h>

#include "scsi_mmc.h"

/* Win32 DeviceIoControl specifics */
/***** FIXME: #include ntddcdrm.h from Wine, but probably need to 
   modify it a little.
*/

#ifndef IOCTL_CDROM_BASE
#    define IOCTL_CDROM_BASE FILE_DEVICE_CD_ROM
#endif
#ifndef IOCTL_CDROM_READ_TOC
#define IOCTL_CDROM_READ_TOC \
  CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
#ifndef IOCTL_CDROM_RAW_READ
#define IOCTL_CDROM_RAW_READ CTL_CODE(IOCTL_CDROM_BASE, 0x000F, \
                                      METHOD_OUT_DIRECT, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_CDROM_READ_Q_CHANNEL
#define IOCTL_CDROM_READ_Q_CHANNEL \
  CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

typedef struct {
   SCSI_PASS_THROUGH Spt;
   ULONG Filler;
   UCHAR SenseBuf[32];
   UCHAR DataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS;

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

#include "win32.h"

/*
  Returns a string that can be used in a CreateFile call if 
  c_drive letter is a character. If not NULL is returned.
 */

const char *
win32ioctl_is_cdrom(const char c_drive_letter) 
{

    UINT uDriveType;
    char sz_win32_drive[4];
    
    sz_win32_drive[0]= c_drive_letter;
    sz_win32_drive[1]=':';
    sz_win32_drive[2]='\\';
    sz_win32_drive[3]='\0';

    uDriveType = GetDriveType(sz_win32_drive);

    switch(uDriveType) {
    case DRIVE_CDROM: {
        char sz_win32_drive_full[] = "\\\\.\\X:";
	sz_win32_drive_full[4] = c_drive_letter;
	return strdup(sz_win32_drive_full);
    }
    default:
        cdio_debug("Drive %c is not a CD-ROM", c_drive_letter);
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
    cdio_info("Error reading audio-mode %lu (%ld)\n", 
	      (long unsigned int) lsn, GetLastError());
    return 1;
  }
  return 0;
}

/*!
   Reads a single raw sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
static int
win32ioctl_read_raw_sector (_img_private_t *env, void *buf, lsn_t lsn) 
{
  SCSI_PASS_THROUGH_DIRECT sptd;
  BOOL success;
  DWORD dwBytesReturned;
  
  sptd.Length=sizeof(sptd);
  sptd.PathId=0;     /* SCSI card ID will be filled in automatically */
  sptd.TargetId=0;   /* SCSI target ID will also be filled in */
  sptd.Lun=0;        /* SCSI lun ID will also be filled in */
  sptd.CdbLength=12; /* CDB size is 12 for ReadCD MMC1 command */
  sptd.SenseInfoLength=0; /* Don't return any sense data */
  sptd.DataIn            = SCSI_IOCTL_DATA_IN; 
  sptd.DataTransferLength= CDIO_CD_FRAMESIZE_RAW; 
  sptd.TimeOutValue=60;  /*SCSI timeout value (60 seconds - 
			   maybe it should be longer) */
  sptd.DataBuffer= (PVOID) buf;
  sptd.SenseInfoOffset=0;

  /* ReadCD CDB12 command.  The values were taken from MMC1 draft paper. */
  sptd.Cdb[0]=CDIO_MMC_GPCMD_READ_CD;
  sptd.Cdb[1]=0;        

  CDIO_MMC_SET_READ_LBA(sptd.Cdb, lsn);

  CDIO_MMC_SET_READ_LENGTH(sptd.Cdb, 1);

  sptd.Cdb[9]=0xF8;  /* Raw read, 2352 bytes per sector */
  sptd.Cdb[10]=0;   
  sptd.Cdb[11]=0;
  sptd.Cdb[12]=0;
  sptd.Cdb[13]=0;
  sptd.Cdb[14]=0;
  sptd.Cdb[15]=0;
  
  /* Send the command to drive */
  success=DeviceIoControl(env->h_device_handle,
			  IOCTL_SCSI_PASS_THROUGH_DIRECT,               
			  (PVOID)&sptd, 
			  (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
			  NULL, 0,                        
			  &dwBytesReturned,
			  NULL);

  if(! success) {
    cdio_info("Error reading %lu (%ld)\n", (long unsigned) lsn, 
	      GetLastError());
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
  int ret = win32ioctl_read_raw_sector (env, buf, lsn);

  if ( 0 != ret) return ret;

  memcpy (data, 
	  buf + CDIO_CD_SYNC_SIZE + CDIO_CD_XA_HEADER,
	  mode2_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
  return 0;

}

/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
win32ioctl_read_mode1_sector (_img_private_t *env, void *data, lsn_t lsn, 
			      bool mode2_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int ret = win32ioctl_read_raw_sector (env, buf, lsn);

  if ( 0 != ret) return ret;

  memcpy (data, 
	  buf + CDIO_CD_SYNC_SIZE+CDIO_CD_HEADER_SIZE, 
	  mode2_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
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
  OSVERSIONINFO ov;
  DWORD dw_access_flags;
  
  cdio_debug("using winNT/2K/XP ioctl layer");
  
  memset(&ov,0,sizeof(OSVERSIONINFO));
  ov.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
  GetVersionEx(&ov);
  
  if((ov.dwPlatformId==VER_PLATFORM_WIN32_NT) &&        
     (ov.dwMajorVersion>4))
    dw_access_flags = GENERIC_READ|GENERIC_WRITE;  /* add gen write on W2k/XP */
  else dw_access_flags = GENERIC_READ;
  
  if (cdio_is_device_win32(env->gen.source_name)) {
    sprintf( psz_win32_drive, "\\\\.\\%c:", env->gen.source_name[len-2] );
    
    env->h_device_handle = CreateFile( psz_win32_drive, 
				       dw_access_flags,
				       FILE_SHARE_READ | FILE_SHARE_WRITE, 
				       NULL, 
				       OPEN_EXISTING,
				       FILE_ATTRIBUTE_NORMAL, 
				       NULL );
    if( env->h_device_handle == INVALID_HANDLE_VALUE )
      {
	/* No good. try toggle write. */
	dw_access_flags ^= GENERIC_WRITE;  
	env->h_device_handle = CreateFile( psz_win32_drive, 
					   dw_access_flags, 
					   FILE_SHARE_READ,  
					   NULL, 
					   OPEN_EXISTING, 
					   FILE_ATTRIBUTE_NORMAL, 
					   NULL );
	return (env->h_device_handle == NULL) ? false : true;
      }
    env->b_ioctl_init = true;
    return true;
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
win32ioctl_get_mcn (const _img_private_t *env) {

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
    cdio_warn( "could not read Q Channel at track %d", 1);
  } else if (mcn.Mcval) 
    return strdup(mcn.MediaCatalog);
  return NULL;
}

/*!  
  Get the format (XA, DATA, AUDIO) of a track. 
*/
track_format_t
win32ioctl_get_track_format(_img_private_t *env, track_t track_num) 
{
  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
  */
  if (env->tocent[track_num-1].Control & 0x04) {
    if (env->tocent[track_num-1].Format == 0x10)
      return TRACK_FORMAT_CDI;
    else if (env->tocent[track_num-1].Format == 0x20) 
      return TRACK_FORMAT_XA;
    else
      return TRACK_FORMAT_DATA;
  } else
    return TRACK_FORMAT_AUDIO;
}


/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
cdio_drive_cap_t
win32ioctl_get_drive_cap (const void *env) 
{
  const _img_private_t *_obj = env;
  int32_t i_drivetype = 0;
  SCSI_PASS_THROUGH_WITH_BUFFERS      sptwb;
  ULONG                               returned = 0;
  ULONG                               length;
  
  /* If device supports SCSI-3, then we can get the CD drive
     capabilities, i.e. ability to read/write to CD-ROM/R/RW
     or/and read/write to DVD-ROM/R/RW.   */
  
  memset(&sptwb,0, sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
  
  sptwb.Spt.Length             = sizeof(SCSI_PASS_THROUGH);
  sptwb.Spt.PathId             = 0;
  sptwb.Spt.TargetId           = 1;
  sptwb.Spt.Lun                = 0;
  sptwb.Spt.CdbLength          = 6; /* CDB6GENERIC_LENGTH; */
  sptwb.Spt.SenseInfoLength    = 24;
  sptwb.Spt.DataIn             = SCSI_IOCTL_DATA_IN;
  sptwb.Spt.DataTransferLength = 192;
  sptwb.Spt.TimeOutValue       = 2;
  sptwb.Spt.DataBufferOffset   =
    offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf);
  sptwb.Spt.SenseInfoOffset    = 
    offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,SenseBuf);
  sptwb.Spt.Cdb[0]             = CDIO_MMC_MODE_SENSE;
  /*sptwb.Spt.Cdb[1]           = 0x08;  /+ doesn't return block descriptors */
  sptwb.Spt.Cdb[1]             = 0x0;
  sptwb.Spt.Cdb[2]             = 0x2a;  /*MODE_PAGE_CAPABILITIES*/;
  sptwb.Spt.Cdb[3]             = 0;     /* Not used */
  sptwb.Spt.Cdb[4]             = 128;
  sptwb.Spt.Cdb[5]             = 0;     /* Not used */
  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf) +
    sptwb.Spt.DataTransferLength;
  
  if ( DeviceIoControl(_obj->h_device_handle,
		       IOCTL_SCSI_PASS_THROUGH,
		       &sptwb,
		       sizeof(SCSI_PASS_THROUGH),
		       &sptwb,
		       length,
		       &returned,
		       FALSE) )
    {
      unsigned int n=sptwb.DataBuf[3]+4;
      /* Reader? */
      if (sptwb.DataBuf[n+5] & 0x01) i_drivetype |= CDIO_DRIVE_CAP_CD_AUDIO;
      if (sptwb.DataBuf[n+2] & 0x02) i_drivetype |= CDIO_DRIVE_CAP_CD_RW;
      if (sptwb.DataBuf[n+2] & 0x08) i_drivetype |= CDIO_DRIVE_CAP_DVD;
      
      /* Writer? */
      if (sptwb.DataBuf[n+3] & 0x01) i_drivetype |= CDIO_DRIVE_CAP_CD_R;
      if (sptwb.DataBuf[n+3] & 0x10) i_drivetype |= CDIO_DRIVE_CAP_DVD_R;
      if (sptwb.DataBuf[n+3] & 0x20) i_drivetype |= CDIO_DRIVE_CAP_DVD_RAM;

      if (sptwb.DataBuf[n+6] & 0x08) i_drivetype |= CDIO_DRIVE_CAP_OPEN_TRAY;
      if (sptwb.DataBuf[n+6] >> 5 != 0) 
	i_drivetype |= CDIO_DRIVE_CAP_CLOSE_TRAY;
      
      return i_drivetype;
    } else {
      i_drivetype = CDIO_DRIVE_CAP_CD_AUDIO | CDIO_DRIVE_CAP_UNKNOWN;
    }
  return CDIO_DRIVE_CAP_ERROR;
}

#endif /*HAVE_WIN32_CDROM*/
