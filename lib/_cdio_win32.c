/*
    $Id: _cdio_win32.c,v 1.6 2003/06/07 20:42:49 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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

/* This file contains Win32-specific code and implements low-level 
   control of the CD drive. Culled from vlc's cdrom.h code 
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_win32.c,v 1.6 2003/06/07 20:42:49 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"

/* LBA = msf.frame + 75 * ( msf.second - 2 + 60 * msf.minute ) */
#define MSF_TO_LBA2(min, sec, frame) ((int)frame + 75 * (sec -2 + 60 * min))

#include <string.h>

#ifdef HAVE_WIN32_CDROM

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <windows.h>
#include <winioctl.h>

#include <sys/stat.h>
#include <sys/types.h>

/* Win32 DeviceIoControl specifics */
#ifndef MAXIMUM_NUMBER_TRACKS
#    define MAXIMUM_NUMBER_TRACKS 100
#endif
typedef struct _TRACK_DATA {
    UCHAR Reserved;
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
    TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;
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

/* Win32 aspi specific */
#define WIN_NT               ( GetVersion() < 0x80000000 )
#define ASPI_HAID           0
#define ASPI_TARGET         0
#define DTYPE_CDROM         0x05

#define SENSE_LEN           0x0E
#define SC_GET_DEV_TYPE     0x01
#define SC_EXEC_SCSI_CMD    0x02
#define SC_GET_DISK_INFO    0x06
#define SS_COMP             0x01
#define SS_PENDING          0x00
#define SS_NO_ADAPTERS      0xE8
#define SRB_DIR_IN          0x08
#define SRB_DIR_OUT         0x10
#define SRB_EVENT_NOTIFY    0x40

#define READ_CD 0xbe
#define SECTOR_TYPE_MODE2 0x14
#define READ_CD_USERDATA_MODE2 0x10

#define READ_TOC 0x43
#define READ_TOC_FORMAT_TOC 0x0

#pragma pack(1)

struct SRB_GetDiskInfo
{
    unsigned char   SRB_Cmd;
    unsigned char   SRB_Status;
    unsigned char   SRB_HaId;
    unsigned char   SRB_Flags;
    unsigned long   SRB_Hdr_Rsvd;
    unsigned char   SRB_Target;
    unsigned char   SRB_Lun;
    unsigned char   SRB_DriveFlags;
    unsigned char   SRB_Int13HDriveInfo;
    unsigned char   SRB_Heads;
    unsigned char   SRB_Sectors;
    unsigned char   SRB_Rsvd1[22];
};

struct SRB_GDEVBlock
{
    unsigned char SRB_Cmd;
    unsigned char SRB_Status;
    unsigned char SRB_HaId;
    unsigned char SRB_Flags;
    unsigned long SRB_Hdr_Rsvd;
    unsigned char SRB_Target;
    unsigned char SRB_Lun;
    unsigned char SRB_DeviceType;
    unsigned char SRB_Rsvd1;
};

struct SRB_ExecSCSICmd
{
    unsigned char   SRB_Cmd;
    unsigned char   SRB_Status;
    unsigned char   SRB_HaId;
    unsigned char   SRB_Flags;
    unsigned long   SRB_Hdr_Rsvd;
    unsigned char   SRB_Target;
    unsigned char   SRB_Lun;
    unsigned short  SRB_Rsvd1;
    unsigned long   SRB_BufLen;
    unsigned char   *SRB_BufPointer;
    unsigned char   SRB_SenseLen;
    unsigned char   SRB_CDBLen;
    unsigned char   SRB_HaStat;
    unsigned char   SRB_TargStat;
    unsigned long   *SRB_PostProc;
    unsigned char   SRB_Rsvd2[20];
    unsigned char   CDBByte[16];
    unsigned char   SenseArea[SENSE_LEN+2];
};

#pragma pack()

typedef struct {
  lsn_t          start_lsn;
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

static const char *
cdio_have_cdrom_drive(const char drive_letter) {
  static char psz_win32_drive[7];
  _img_private_t obj;

  /* Initializations */
  obj.h_device_handle = NULL;
  obj.i_sid = 0;
  obj.hASPI = 0;
  obj.lpSendCommand = 0;
  
  if ( WIN_NT ) {
    sprintf( psz_win32_drive, "\\\\.\\%c:", drive_letter );
    
    obj.h_device_handle = CreateFile( psz_win32_drive, GENERIC_READ,
				      FILE_SHARE_READ | FILE_SHARE_WRITE,
				      NULL, OPEN_EXISTING,
				      FILE_FLAG_NO_BUFFERING |
				      FILE_FLAG_RANDOM_ACCESS, NULL );
    if (obj.h_device_handle != NULL) {
      CloseHandle(obj.h_device_handle);
      return strdup(psz_win32_drive);
    } else {
      CloseHandle(obj.h_device_handle);
      return NULL;
    }
  } else {
    HMODULE hASPI = NULL;
    long (*lpGetSupport)( void ) = NULL;
    long (*lpSendCommand)( void* ) = NULL;
    DWORD dwSupportInfo;
    int j, i_hostadapters;
    char c_drive;
    
    hASPI = LoadLibrary( "wnaspi32.dll" );
    if( hASPI != NULL ) {
      (FARPROC) lpGetSupport = GetProcAddress( hASPI,
					       "GetASPI32SupportInfo" );
      (FARPROC) lpSendCommand = GetProcAddress( hASPI,
						"SendASPI32Command" );
    }
    
    if( hASPI == NULL || lpGetSupport == NULL || lpSendCommand == NULL ) {
      cdio_debug("Unable to load ASPI or get ASPI function pointers");
      if( hASPI ) FreeLibrary( hASPI );
      return NULL;
    }
    
    /* ASPI support seems to be there */
    
    dwSupportInfo = lpGetSupport();
    
    if( HIBYTE( LOWORD ( dwSupportInfo ) ) == SS_NO_ADAPTERS ) {
      cdio_debug("no host adapters found (ASPI)");
      FreeLibrary( hASPI );
      return NULL;
    }
    
    if( HIBYTE( LOWORD ( dwSupportInfo ) ) != SS_COMP ) {
      cdio_debug("Unable to initalize ASPI layer");
      FreeLibrary( hASPI );
      return NULL;
    }
    
    i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
    if( i_hostadapters == 0 ) {
      FreeLibrary( hASPI );
      return NULL;
    }
    
    c_drive = toupper(drive_letter) - 'A';
    
    for( j = 0; j < 15; j++ ) {
      struct SRB_GetDiskInfo srbDiskInfo;
      
      srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
      srbDiskInfo.SRB_HaId        = 0;
      srbDiskInfo.SRB_Flags       = 0;
      srbDiskInfo.SRB_Hdr_Rsvd    = 0;
      srbDiskInfo.SRB_Target      = j;
      srbDiskInfo.SRB_Lun         = 0;
      
      lpSendCommand( (void*) &srbDiskInfo );
      
      if( (srbDiskInfo.SRB_Status == SS_COMP) &&
	  (srbDiskInfo.SRB_Int13HDriveInfo == c_drive) ) {
	/* Make sure this is a cdrom device */
	struct SRB_GDEVBlock   srbGDEVBlock;
	
	memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
	srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
	srbGDEVBlock.SRB_HaId   = 0;
	srbGDEVBlock.SRB_Target = j;
	
	lpSendCommand( (void*) &srbGDEVBlock );
	
	if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
	    ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
	  sprintf( psz_win32_drive, "%c:", drive_letter );
	  FreeLibrary( hASPI );
	  return(psz_win32_drive);
	}
      }
    }
    FreeLibrary( hASPI );
  }
  return NULL;

}

/*!
  Initialize CD device.
 */
static bool
_cdio_init_win32 (void *user_data)
{
  _img_private_t *_obj = user_data;
  if (_obj->gen.init) {
    cdio_error ("init called more than once");
    return false;
  }
  
  _obj->gen.init = true;
  _obj->toc_init = false;


  /* Initializations */
  _obj->h_device_handle = NULL;
  _obj->i_sid = 0;
  _obj->hASPI = 0;
  _obj->lpSendCommand = 0;
  
  if ( WIN_NT ) {
    char psz_win32_drive[7];
    
    cdio_debug("using winNT/2K/XP ioctl layer");
    
    sprintf( psz_win32_drive, "\\\\.\\%c:", _obj->gen.source_name[0] );
    
    _obj->h_device_handle = CreateFile( psz_win32_drive, GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL, OPEN_EXISTING,
					FILE_FLAG_NO_BUFFERING |
					FILE_FLAG_RANDOM_ACCESS, NULL );
    return (_obj->h_device_handle == NULL) ? false : true;
  } else {
    HMODULE hASPI = NULL;
    long (*lpGetSupport)( void ) = NULL;
    long (*lpSendCommand)( void* ) = NULL;
    DWORD dwSupportInfo;
    int i, j, i_hostadapters;
    char c_drive = _obj->gen.source_name[0];
    
    hASPI = LoadLibrary( "wnaspi32.dll" );
    if( hASPI != NULL ) {
      (FARPROC) lpGetSupport = GetProcAddress( hASPI,
					       "GetASPI32SupportInfo" );
      (FARPROC) lpSendCommand = GetProcAddress( hASPI,
						"SendASPI32Command" );
    }
    
    if( hASPI == NULL || lpGetSupport == NULL || lpSendCommand == NULL ) {
      cdio_debug("Unable to load ASPI or get ASPI function pointers");
      if( hASPI ) FreeLibrary( hASPI );
      return false;
    }
    
    /* ASPI support seems to be there */
    
    dwSupportInfo = lpGetSupport();
    
    if( HIBYTE( LOWORD ( dwSupportInfo ) ) == SS_NO_ADAPTERS ) {
      cdio_debug("no host adapters found (ASPI)");
      FreeLibrary( hASPI );
      return -1;
    }
    
    if( HIBYTE( LOWORD ( dwSupportInfo ) ) != SS_COMP ) {
      cdio_debug("unable to initalize ASPI layer");
      FreeLibrary( hASPI );
      return -1;
    }
    
    i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
    if( i_hostadapters == 0 ) {
      FreeLibrary( hASPI );
      return -1;
    }
    
    c_drive = toupper(c_drive) - 'A';
    
    for( i = 0; i < i_hostadapters; i++ ) {
      for( j = 0; j < 15; j++ ) {
	struct SRB_GetDiskInfo srbDiskInfo;
	  
	srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
	srbDiskInfo.SRB_HaId        = i;
	srbDiskInfo.SRB_Flags       = 0;
	srbDiskInfo.SRB_Hdr_Rsvd    = 0;
	srbDiskInfo.SRB_Target      = j;
	srbDiskInfo.SRB_Lun         = 0;
	
	lpSendCommand( (void*) &srbDiskInfo );
	
	if( (srbDiskInfo.SRB_Status == SS_COMP) &&
	    (srbDiskInfo.SRB_Int13HDriveInfo == c_drive) ) {
	  /* Make sure this is a cdrom device */
	  struct SRB_GDEVBlock   srbGDEVBlock;
	  
	  memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
	  srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
	  srbGDEVBlock.SRB_HaId   = i;
	  srbGDEVBlock.SRB_Target = j;
	  
	  lpSendCommand( (void*) &srbGDEVBlock );
	  
	  if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
	      ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
	    _obj->i_sid = MAKEWORD( i, j );
	    _obj->hASPI = (long)hASPI;
	    _obj->lpSendCommand = lpSendCommand;
	    cdio_debug("Using ASPI layer");
	    
	    return true;
	  } else {
	    FreeLibrary( hASPI );
	    cdio_debug( "%c: is not a CD-ROM drive",
			_obj->gen.source_name[0] );
	    return false;
	  }
	}
      }
    }
    
    FreeLibrary( hASPI );
    cdio_debug( "Unable to get HaId and target (ASPI)" );
    
  }
  
  return false;
}

/*!
  Release and free resources associated with cd. 
 */
static void
_cdio_win32_free (void *user_data)
{
  _img_private_t *_obj = user_data;

  if (NULL == _obj) return;
  free (_obj->gen.source_name);

  if( _obj->h_device_handle )
    CloseHandle( _obj->h_device_handle );
  if( _obj->hASPI )
    FreeLibrary( (HMODULE)_obj->hASPI );

  free (_obj);
}

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_raw_sector (void *user_data, void *data, lsn_t lsn)
{
  unsigned char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  _img_private_t *_obj = user_data;

  if( _obj->hASPI ) {
    HANDLE hEvent;
    struct SRB_ExecSCSICmd ssc;
    int blocks = 1;
    int sector_type;
    int sync, header_code, user_data, edc_ecc, error_field;
    int sub_channel;
    
    /* Create the transfer completion event */
    hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( hEvent == NULL ) {
      return 1;
    }

    /* Data selection */
    sector_type = 0;		/* all types */
    /*sector_type = 1;*/	/* CD-DA */
    /*sector_type = 2;*/	/* mode1 */
    /*sector_type = 3;*/	/* mode2 */
    /*sector_type = 4;*/	/* mode2/form1 */
    /*sector_type = 5;*/	/* mode2/form2 */
    sync = 0;
    header_code = 2;
    user_data = 1;
    edc_ecc = 0;
    error_field = 0;
    sub_channel = 0;

    memset( &ssc, 0, sizeof( ssc ) );
    
    ssc.SRB_Cmd         = SC_EXEC_SCSI_CMD;
    ssc.SRB_Flags       = SRB_DIR_IN | SRB_EVENT_NOTIFY;
    ssc.SRB_HaId        = LOBYTE( _obj->i_sid );
    ssc.SRB_Target      = HIBYTE( _obj->i_sid );
    ssc.SRB_SenseLen    = SENSE_LEN;
    
    ssc.SRB_PostProc = (LPVOID) hEvent;
    ssc.SRB_CDBLen      = 12;
    
    /* Operation code */
    ssc.CDBByte[ 0 ] = READ_CD;

    /* Start of LSN */
    ssc.CDBByte[ 1 ] = (sector_type) << 2;
    ssc.CDBByte[ 2 ] = (lsn >> 24) & 0xff;
    ssc.CDBByte[ 3 ] = (lsn >> 16) & 0xff;
    ssc.CDBByte[ 4 ] = (lsn >>  8) & 0xff;
    ssc.CDBByte[ 5 ] = (lsn      ) & 0xff;
    
    /* Transfer length */
    ssc.CDBByte[ 6 ] = (blocks >> 16) & 0xff;
    ssc.CDBByte[ 7 ] = (blocks >>  8) & 0xff;
    ssc.CDBByte[ 8 ] = blocks & 0xff;
    
    ssc.CDBByte[ 9 ] = (sync << 7) |
      (header_code << 5) |
      (user_data << 4) |
      (edc_ecc << 3) |
      (error_field << 1);
    /* ssc.CDBByte[ 9 ] = READ_CD_USERDATA_MODE2; */
    
    /* Result buffer */
    ssc.SRB_BufPointer  = buf;
    ssc.SRB_BufLen = CDIO_CD_FRAMESIZE_RAW;
    
    /* Initiate transfer */
    ResetEvent( hEvent );
    _obj->lpSendCommand( (void*) &ssc );
    
    /* If the command has still not been processed, wait until it's
     * finished */
    if( ssc.SRB_Status == SS_PENDING ) {
      WaitForSingleObject( hEvent, INFINITE );
    }
    CloseHandle( hEvent );

    /* check that the transfer went as planned */
    if( ssc.SRB_Status != SS_COMP ) {
      return 1;
    }

  } else {
    DWORD dwBytesReturned;
    RAW_READ_INFO cdrom_raw;
    
    /* Initialize CDROM_RAW_READ structure */
    cdrom_raw.DiskOffset.QuadPart = CDIO_CD_FRAMESIZE * lsn;
    cdrom_raw.SectorCount = 1;
    cdrom_raw.TrackMode = XAForm2;
    
    if( DeviceIoControl( _obj->h_device_handle,
			 IOCTL_CDROM_RAW_READ, &cdrom_raw,
			 sizeof(RAW_READ_INFO), buf,
			 sizeof(buf), &dwBytesReturned, NULL )
	== 0 ) {
	return 1;
      }
   }

  /* FIXME! remove the 8 (SUBHEADER size) below... */
  memcpy (data, buf+8, CDIO_CD_FRAMESIZE_RAW);

  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sector (void *user_data, void *data, lsn_t lsn, 
		    bool mode2_form2)
{
  char buf[M2RAW_SECTOR_SIZE] = { 0, };
  _img_private_t *_obj = user_data;
  int ret;

  if (_obj->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");

  if (_obj->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (_obj->gen.ioctls_debugged < 75 
      || (_obj->gen.ioctls_debugged < (30 * 75)  
	  && _obj->gen.ioctls_debugged % 75 == 0)
      || _obj->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %d", lsn);
  
  _obj->gen.ioctls_debugged++;
 
  ret = _cdio_read_raw_sector(user_data, buf, lsn);

  if( ret != 0 ) return ret;
  
  if (mode2_form2)
    memcpy (data, buf, M2RAW_SECTOR_SIZE);
  else
    memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
  
  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sectors (void *user_data, void *data, lsn_t lsn, 
		     bool mode2_form2, unsigned nblocks)
{
  _img_private_t *_obj = user_data;
  int i;
  int retval;

  for (i = 0; i < nblocks; i++) {
    if (mode2_form2) {
      if ( (retval = _cdio_read_mode2_sector (_obj, 
					  ((char *)data) + (M2RAW_SECTOR_SIZE * i),
					  lsn + i, true)) )
	return retval;
    } else {
      char buf[M2RAW_SECTOR_SIZE] = { 0, };
      if ( (retval = _cdio_read_mode2_sector (_obj, buf, lsn + i, true)) )
	return retval;
      
      memcpy (((char *)data) + (CDIO_CD_FRAMESIZE * i), 
	      buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    }
  }
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_cdio_stat_size (void *user_data)
{
  _img_private_t *_obj = user_data;

  return _obj->tocent[_obj->total_tracks].start_lsn;
}

/*!
  Set the key "arg" to "value" in source device.
*/
static int
_cdio_set_arg (void *user_data, const char key[], const char value[])
{
  _img_private_t *_obj = user_data;

  if (!strcmp (key, "source"))
    {
      if (!value)
	return -2;

      free (_obj->gen.source_name);
      
      _obj->gen.source_name = strdup (value);
    }
  else 
    return -1;

  return 0;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
static bool
_cdio_read_toc (_img_private_t *_obj) 
{

  if( _obj->hASPI ) {
    HANDLE hEvent;
    struct SRB_ExecSCSICmd ssc;
    unsigned char p_tocheader[ 4 ];
    
    /* Create the transfer completion event */
    hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    if( hEvent == NULL ) {
      return false;
    }
    
    memset( &ssc, 0, sizeof( ssc ) );
    
    ssc.SRB_Cmd         = SC_EXEC_SCSI_CMD;
    ssc.SRB_Flags       = SRB_DIR_IN | SRB_EVENT_NOTIFY;
    ssc.SRB_HaId        = LOBYTE( _obj->i_sid );
    ssc.SRB_Target      = HIBYTE( _obj->i_sid );
    ssc.SRB_SenseLen    = SENSE_LEN;
    
    ssc.SRB_PostProc = (LPVOID) hEvent;
    ssc.SRB_CDBLen      = 10;
    
    /* Operation code */
    ssc.CDBByte[ 0 ] = READ_TOC;
    
    /* Format */
    ssc.CDBByte[ 2 ] = READ_TOC_FORMAT_TOC;
    
    /* Starting track */
    ssc.CDBByte[ 6 ] = 0;
    
    /* Allocation length and buffer */
    ssc.SRB_BufLen = sizeof( p_tocheader );
    ssc.SRB_BufPointer  = p_tocheader;
    ssc.CDBByte[ 7 ] = ( ssc.SRB_BufLen >>  8 ) & 0xff;
    ssc.CDBByte[ 8 ] = ( ssc.SRB_BufLen       ) & 0xff;
    
    /* Initiate transfer */
    ResetEvent( hEvent );
    _obj->lpSendCommand( (void*) &ssc );
    
    /* If the command has still not been processed, wait until it's
     * finished */
    if( ssc.SRB_Status == SS_PENDING )
      WaitForSingleObject( hEvent, INFINITE );
    
    /* check that the transfer went as planned */
    if( ssc.SRB_Status != SS_COMP ) {
      CloseHandle( hEvent );
      return false;
    }

    _obj->first_track_num = p_tocheader[2];
    _obj->total_tracks    = p_tocheader[3] - p_tocheader[2] + 1;
    
    {
      int i, i_toclength;
      unsigned char *p_fulltoc;
      
      i_toclength = 4 /* header */ + p_tocheader[0] +
	((unsigned int)p_tocheader[1] << 8);
      
      p_fulltoc = malloc( i_toclength );
      
      if( p_fulltoc == NULL ) {
	cdio_error( "out of memory" );
	CloseHandle( hEvent );
	return false;
      }
      
      /* Allocation length and buffer */
      ssc.SRB_BufLen = i_toclength;
      ssc.SRB_BufPointer  = p_fulltoc;
      ssc.CDBByte[ 7 ] = ( ssc.SRB_BufLen >>  8 ) & 0xff;
      ssc.CDBByte[ 8 ] = ( ssc.SRB_BufLen       ) & 0xff;
      
      /* Initiate transfer */
      ResetEvent( hEvent );
      _obj->lpSendCommand( (void*) &ssc );
      
      /* If the command has still not been processed, wait until it's
       * finished */
      if( ssc.SRB_Status == SS_PENDING )
	WaitForSingleObject( hEvent, INFINITE );
      
      /* check that the transfer went as planned */
      if( ssc.SRB_Status != SS_COMP )
	_obj->total_tracks = 0;
      
      for( i = 0 ; i <= _obj->total_tracks ; i++ ) {
	int i_index = 8 + 8 * i;
	_obj->tocent[ i ].start_lsn = ((int)p_fulltoc[ i_index ] << 24) +
	  ((int)p_fulltoc[ i_index+1 ] << 16) +
	  ((int)p_fulltoc[ i_index+2 ] << 8) +
	  (int)p_fulltoc[ i_index+3 ];
	
	cdio_debug( "p_sectors: %i %i", i, _obj->tocent[i].start_lsn );
      }
	
      free( p_fulltoc );
    }
    
    CloseHandle( hEvent );
    return true;
    
  } else {
    DWORD dwBytesReturned;
    CDROM_TOC cdrom_toc;
    int i;
    
    if( DeviceIoControl( _obj->h_device_handle,
			 IOCTL_CDROM_READ_TOC,
			 NULL, 0, &cdrom_toc, sizeof(CDROM_TOC),
			 &dwBytesReturned, NULL ) == 0 ) {
      cdio_debug( "could not read TOCHDR" );
      return false;
    }
    
    _obj->first_track_num = cdrom_toc.FirstTrack;
    _obj->total_tracks    = cdrom_toc.LastTrack - cdrom_toc.FirstTrack + 1;
    
      
    for( i = 0 ; i <= _obj->total_tracks ; i++ ) {
	_obj->tocent[ i ].start_lsn = MSF_TO_LBA2(
					 cdrom_toc.TrackData[i].Address[1],
					 cdrom_toc.TrackData[i].Address[2],
					 cdrom_toc.TrackData[i].Address[3] );
	cdio_debug("p_sectors: %i, %i", i, (_obj->tocent[i].start_lsn));
      }
  }
  return true;
}

/*!
  Eject media. Return 1 if successful, 0 otherwise.
 */
static int 
_cdio_eject_media (void *user_data) {

  _img_private_t *_obj = user_data;


  MCI_OPEN_PARMS op;
  MCI_STATUS_PARMS st;
  DWORD i_flags;
  char psz_drive[4];
  int ret;
    
  memset( &op, 0, sizeof(MCI_OPEN_PARMS) );
  op.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  strcpy( psz_drive, "X:" );
  psz_drive[0] = _obj->gen.source_name[0];
  op.lpstrElementName = psz_drive;
  
  /* Set the flags for the device type */
  i_flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
    MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE;
  
  if( !mciSendCommand( 0, MCI_OPEN, i_flags, (unsigned long)&op ) ) {
    st.dwItem = MCI_STATUS_READY;
    /* Eject disc */
    ret = mciSendCommand( op.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, 0 ) != 0;
    /* Release access to the device */
    mciSendCommand( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
  } else 
    ret = 0;
  
  return ret;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_cdio_get_arg (void *user_data, const char key[])
{
  _img_private_t *_obj = user_data;

  if (!strcmp (key, "source")) {
    return _obj->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    if ( WIN_NT ) 
      return "winNT/2K/XP ioctl";
    else if (_obj->hASPI) 
      return "ASPI";
    else 
      return "undefined WIN32";
  } 
  return NULL;
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *user_data) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return _obj->first_track_num;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_num_tracks(void *user_data) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  return _obj->total_tracks;
}

/*!  
  Get format of track. 
*/
static track_format_t
_cdio_get_track_format(void *user_data, track_t track_num) 
{
  _img_private_t *_obj = user_data;

  MCI_OPEN_PARMS op;
  MCI_STATUS_PARMS st;
  DWORD i_flags;
  char psz_drive[4];
  int ret;

  memset( &op, 0, sizeof(MCI_OPEN_PARMS) );
  op.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  strcpy( psz_drive, "X:" );
  psz_drive[0] = _obj->gen.source_name[0];
  op.lpstrElementName = psz_drive;
  
  /* Set the flags for the device type */
  i_flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
    MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE;
  
  if( !mciSendCommand( 0, MCI_OPEN, i_flags, (unsigned long)&op ) ) {
    st.dwItem  = MCI_CDA_STATUS_TYPE_TRACK;
    st.dwTrack = track_num;
    i_flags = MCI_TRACK | MCI_STATUS_ITEM ;
    ret = mciSendCommand( op.wDeviceID, MCI_STATUS, i_flags, 
			  (unsigned long) &st );
    mciSendCommand( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );

    switch(st.dwReturn) {
    case MCI_CDA_TRACK_AUDIO:
      return TRACK_FORMAT_AUDIO;
    case MCI_CDA_TRACK_OTHER:
      return TRACK_FORMAT_DATA;
    default:
      return TRACK_FORMAT_XA;
    }

    /* Release access to the device */
    mciSendCommand( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
  }

  return TRACK_FORMAT_ERROR;
  
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8

  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_cdio_get_track_green(void *user_data, track_t track_num) 
{
  _img_private_t *_obj = user_data;
  
  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

  if (track_num > _obj->total_tracks+1 || track_num == 0)
    return false;

  /* FIXME! */
  return true;
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
_cdio_get_track_msf(void *user_data, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = user_data;

  if (NULL == msf) return false;

  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

  if (track_num > _obj->total_tracks+1 || track_num == 0) {
    return false;
  } else {
    cdio_lsn_to_msf(_obj->tocent[track_num-1].start_lsn, msf);
    return true;
  }
}

#endif /* HAVE_WIN32_CDROM */

/*!
  Return a string containing the default CD device if none is specified.
  if CdIo is NULL (we haven't initialized a specific device driver), 
  then find a suitable one and return the default device for that.
  
  NULL is returned if we couldn't get a default device.
*/
char *
cdio_get_default_device_win32(void)
{

#ifdef HAVE_WIN32_CDROM
  char drive_letter;

  for (drive_letter='A'; drive_letter <= 'Z'; drive_letter++) {
    const char *drive_str=cdio_have_cdrom_drive(drive_letter);
    if (drive_str != NULL) {
      return strdup(drive_str);
    }
  }
#endif
  return NULL;
}

/*!  
  Return true if source_name could be a device containing a CD-ROM.
*/
bool
cdio_is_device_win32(const char *source_name)
{
  unsigned int len;
  
  len = strlen(source_name);
  return ((len == 2) && (source_name[len-1] == ':'));
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_win32 (const char *source_name)
{

#ifdef HAVE_WIN32_CDROM
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = _cdio_win32_free,
    .get_arg            = _cdio_get_arg,
    .get_default_device = cdio_get_default_device_win32,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = NULL, /* This could be implemented if need be. */
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = NULL,
    .read               = NULL,
    .read_audio_sector  = _cdio_read_raw_sector,
    .read_mode2_sector  = _cdio_read_mode2_sector,
    .read_mode2_sectors = _cdio_read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->gen.init       = false;
  _data->gen.fd         = -1;

  _cdio_set_arg(_data, "source", (NULL == source_name) 
		? cdio_get_default_device_win32(): source_name);

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (_cdio_init_win32(_data))
    return ret;
  else {
    _cdio_win32_free (_data);
    return NULL;
  }
  
#else 
  return NULL;
#endif /* HAVE_WIN32_CDROM */

}

bool
cdio_have_win32 (void)
{
#ifdef HAVE_WIN32_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_WIN32_CDROM */
}
