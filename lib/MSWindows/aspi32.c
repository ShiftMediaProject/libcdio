/*
    $Id: aspi32.c,v 1.3 2004/04/30 21:36:54 rocky Exp $

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

/* This file contains Win32-specific code and implements low-level 
   control of the CD drive via the ASPI API.
   Inspired by vlc's cdrom.h code 
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: aspi32.c,v 1.3 2004/04/30 21:36:54 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "scsi_mmc.h"

#include <string.h>

#ifdef HAVE_WIN32_CDROM

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <windows.h>
#include "win32.h"

#include <sys/stat.h>
#include <sys/types.h>
#include "aspi32.h"

/* General ioctl() CD-ROM command function */
static bool 
wnaspi32_mciSendCommand(int id, UINT msg, DWORD flags, void *arg)
{
  MCIERROR mci_error;
  
  mci_error = mciSendCommand(id, msg, flags, (DWORD)arg);
  if ( mci_error ) {
    char error[256];
    
    mciGetErrorString(mci_error, error, 256);
    cdio_warn("mciSendCommand() error: %s", error);
  }
  return(mci_error == 0);
}

const char *
wnaspi32_is_cdrom(const char drive_letter) 
{
  static char psz_win32_drive[7];
  HMODULE hASPI = NULL;
  long (*lpGetSupport)( void ) = NULL;
  long (*lpSendCommand)( void* ) = NULL;
  DWORD dwSupportInfo;
  int i_adapter, i_num_adapters;
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
  
  i_num_adapters = LOBYTE( LOWORD( dwSupportInfo ) );
  if( i_num_adapters == 0 ) {
    FreeLibrary( hASPI );
    return NULL;
  }
  
  c_drive = toupper(drive_letter) - 'A';
  
  for( i_adapter = 0; i_adapter < i_num_adapters; i_adapter++ ) {
    struct SRB_GetDiskInfo srbDiskInfo;
    int i_target;
    SRB_HAInquiry srbInquiry;
    
    srbInquiry.SRB_Cmd         = SC_HA_INQUIRY;
    srbInquiry.SRB_HaId        = i_adapter;
    
    lpSendCommand( (void*) &srbInquiry );
    
    if( srbInquiry.SRB_Status != SS_COMP ) continue;
    if( !srbInquiry.HA_Unique[3]) srbInquiry.HA_Unique[3]=8;
    
    for(i_target=0; i_target < srbInquiry.HA_Unique[3]; i_target++)
      {
	int i_lun;
	for( i_lun=0; i_lun<8; i_lun++)
	  {
	    srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
	    srbDiskInfo.SRB_Flags       = 0;
	    srbDiskInfo.SRB_Hdr_Rsvd    = 0;
	    srbDiskInfo.SRB_HaId        = i_adapter;
	    srbDiskInfo.SRB_Target      = i_target;
	    srbDiskInfo.SRB_Lun         = i_lun;
	    
	    lpSendCommand( (void*) &srbDiskInfo );
	    
	    if( (srbDiskInfo.SRB_Status == SS_COMP) &&
		(srbDiskInfo.SRB_Int13HDriveInfo == c_drive) ) {
	      /* Make sure this is a cdrom device */
	      struct SRB_GDEVBlock   srbGDEVBlock;
	      
	      memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
	      srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
	      srbDiskInfo.SRB_HaId    = i_adapter;
	      srbGDEVBlock.SRB_Target = i_target;
	      
	      lpSendCommand( (void*) &srbGDEVBlock );
	      
	      if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
		  ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
		sprintf( psz_win32_drive, "%c:", drive_letter );
		FreeLibrary( hASPI );
		return(psz_win32_drive);
	      }
	    }
	  }
      }
  }
  FreeLibrary( hASPI );
  return NULL;
}

/*!
  Initialize CD device.
 */
bool
wnaspi32_init_win32 (_img_private_t *env)
{
  HMODULE hASPI = NULL;
  long (*lpGetSupport)( void ) = NULL;
  long (*lpSendCommand)( void* ) = NULL;
  DWORD dwSupportInfo;
  int i, j, i_num_adapters;
  char c_drive = env->gen.source_name[0];
  
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
    return false;
  }
  
  if( HIBYTE( LOWORD ( dwSupportInfo ) ) != SS_COMP ) {
    cdio_debug("unable to initalize ASPI layer");
    FreeLibrary( hASPI );
    return false;
  }
  
  i_num_adapters = LOBYTE( LOWORD( dwSupportInfo ) );
  if( i_num_adapters == 0 ) {
    FreeLibrary( hASPI );
    return false;
  }
  
  c_drive = toupper(c_drive) - 'A';
  
  for( i = 0; i < i_num_adapters; i++ ) {
    for( j = 0; j < 15; j++ ) {
      struct SRB_GetDiskInfo srbDiskInfo;
      int lun;
      
      for (lun = 0; lun < 8; lun++ ) {
	srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
	srbDiskInfo.SRB_HaId        = i;
	srbDiskInfo.SRB_Flags       = 0;
	srbDiskInfo.SRB_Hdr_Rsvd    = 0;
	srbDiskInfo.SRB_Target      = j;
	srbDiskInfo.SRB_Lun         = lun;
	
	lpSendCommand( (void*) &srbDiskInfo );
	
	if( (srbDiskInfo.SRB_Status == SS_COMP) ) {
	  
	  if (srbDiskInfo.SRB_Int13HDriveInfo != c_drive)
	    {
	      continue;
	    } else {
	      /* Make sure this is a cdrom device */
	      struct SRB_GDEVBlock   srbGDEVBlock;
	      
	      memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
	      srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
	      srbGDEVBlock.SRB_HaId   = i;
	      srbGDEVBlock.SRB_Target = j;
	      
	      lpSendCommand( (void*) &srbGDEVBlock );
	      
	      if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
		  ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
		env->i_sid = MAKEWORD( i, j );
		env->hASPI = (long)hASPI;
		env->lpSendCommand = lpSendCommand;
		env->b_aspi_init   = true;
		cdio_debug("Using ASPI layer");

		return true;
	      } else {
		FreeLibrary( hASPI );
		cdio_debug( "%c: is not a CD-ROM drive",
			    env->gen.source_name[0] );
		return false;
	      }
	    }
	}
	}
    }
  }
  
  FreeLibrary( hASPI );
  cdio_debug( "Unable to get HaId and target (ASPI)" );
  return false;
}

/*!
   Reads a single mode2 sector from cd device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
wnaspi32_mmc_read_sectors (_img_private_t *env, void *data, lsn_t lsn, 
			   int sector_type, unsigned int nblocks)
{
  unsigned char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  HANDLE hEvent;
  struct SRB_ExecSCSICmd ssc;
  
#if 1
  sector_type = 0; /*all types */
  int sync        = 0;
  int header_code = 2;
  int i_user_data   = 1;
  int edc_ecc     = 0;
  int error_field = 0;
#endif
  
  
  /* Create the transfer completion event */
  hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
  if( hEvent == NULL ) {
    return 1;
  }
  
  /* Data selection */
  
  memset( &ssc, 0, sizeof( ssc ) );
  
  ssc.SRB_Cmd         = SC_EXEC_SCSI_CMD;
  ssc.SRB_Flags       = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  ssc.SRB_HaId        = LOBYTE( env->i_sid );
  ssc.SRB_Target      = HIBYTE( env->i_sid );
  ssc.SRB_SenseLen    = SENSE_LEN;
  
  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen      = 12;
  
  CDIO_MMC_SET_COMMAND(ssc.CDBByte, CDIO_MMC_GPCMD_READ_CD));
  CDIO_MMC_SET_READ_TYPE(ssc.CDBByte, sector_type);
  CDIO_MMC_SET_READ_LBA(ssc.CDBByte, lsn);
  CDIO_MMC_SET_READ_LENGTH(ssc.CDBByte, nblocks);
  
#if 1
  ssc.CDBByte[ 9 ] = (sync << 7) |
    (header_code << 5) |
    (i_user_data << 4) |
    (edc_ecc << 3) |
    (error_field << 1);
  /* ssc.CDBByte[ 9 ] = READ_CD_USERDATA_MODE2; */
#else 
  CDIO_MMC_SET_MAIN_CHANNEL_SELECTION_BITS(ssc.CDBByte,
					   CDIO_MMC_MCSB_ALL_HEADERS);
#endif
  
  /* Result buffer */
  ssc.SRB_BufPointer  = buf;
  ssc.SRB_BufLen = CDIO_CD_FRAMESIZE_RAW;
  
  /* Initiate transfer */
  ResetEvent( hEvent );
  env->lpSendCommand( (void*) &ssc );
  
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
  
  /* FIXME! remove the 8 (SUBHEADER size) below... */
  memcpy (data, buf, CDIO_CD_FRAMESIZE_RAW);
  
  return 0;
}

/*!
   Reads an audio device into data starting from lsn.
   Returns 0 if no error. 
 */
int
wnaspi32_read_audio_sectors (_img_private_t *env, void *data, lsn_t lsn, 
			     unsigned int nblocks) 
{
  return wnaspi32_mmc_read_sectors( env, data, lsn, CDIO_MMC_READ_TYPE_CDDA, 
				    nblocks );
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
wnaspi32_read_mode2_sector (_img_private_t *env, void *data, lsn_t lsn)
{
  return wnaspi32_mmc_read_sectors(env, data, lsn, CDIO_MMC_READ_TYPE_ANY, 
				 1);
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool
wnaspi32_read_toc (_img_private_t *env) 
{
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
  ssc.SRB_HaId        = LOBYTE( env->i_sid );
  ssc.SRB_Target      = HIBYTE( env->i_sid );
  ssc.SRB_SenseLen    = SENSE_LEN;
  
  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen      = 10;
  
  /* Operation code */
  CDIO_MMC_SET_COMMAND(ssc.CDBByte, CDIO_MMC_READ_TOC);
  
  /* Format */
  ssc.CDBByte[ 2 ] = READ_TOC_FORMAT_TOC;
  
  /* Starting track */
  CDIO_MMC_SET_START_TRACK(ssc.CDBByte, 0);
  
  /* Allocation length and buffer */
  ssc.SRB_BufLen = sizeof( p_tocheader );
  ssc.SRB_BufPointer  = p_tocheader;
  ssc.CDBByte[ 7 ] = ( ssc.SRB_BufLen >>  8 ) & 0xff;
  ssc.CDBByte[ 8 ] = ( ssc.SRB_BufLen       ) & 0xff;
  
  /* Initiate transfer */
  ResetEvent( hEvent );
  env->lpSendCommand( (void*) &ssc );
  
  /* If the command has still not been processed, wait until it's
   * finished */
  if( ssc.SRB_Status == SS_PENDING )
    WaitForSingleObject( hEvent, INFINITE );
  
  /* check that the transfer went as planned */
  if( ssc.SRB_Status != SS_COMP ) {
    CloseHandle( hEvent );
    return false;
  }
  
  env->first_track_num = p_tocheader[2];
  env->total_tracks    = p_tocheader[3] - p_tocheader[2] + 1;
  
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
    env->lpSendCommand( (void*) &ssc );
    
    /* If the command has still not been processed, wait until it's
     * finished */
    if( ssc.SRB_Status == SS_PENDING )
      WaitForSingleObject( hEvent, INFINITE );
    
    /* check that the transfer went as planned */
    if( ssc.SRB_Status != SS_COMP )
      env->total_tracks = 0;
    
    for( i = 0 ; i <= env->total_tracks ; i++ ) {
      int i_index = 8 + 8 * i;
      env->tocent[ i ].start_lsn = ((int)p_fulltoc[ i_index ] << 24) +
	((int)p_fulltoc[ i_index+1 ] << 16) +
	((int)p_fulltoc[ i_index+2 ] << 8) +
	(int)p_fulltoc[ i_index+3 ];
      env->tocent[ i ].Control   = (UCHAR)p_fulltoc[ 1 + 8 * i ];
      
      cdio_debug( "p_sectors: %i %lu", 
		  i, (unsigned long int) env->tocent[i].start_lsn );
    }
    
    free( p_fulltoc );
  }
  
  CloseHandle( hEvent );
  env->gen.toc_init = true;
  return true;
}

/* Eject media will eventually get removed from _cdio_win32.c */
#if 0
/*!
  Eject media. Return 1 if successful, 0 otherwise.
 */
int 
wnaspi32_eject_media (void *user_data) {

  _img_private_t *env = user_data;


  MCI_OPEN_PARMS op;
  MCI_STATUS_PARMS st;
  DWORD i_flags;
  char psz_drive[4];
  int ret;
    
  memset( &op, 0, sizeof(MCI_OPEN_PARMS) );
  op.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  strcpy( psz_drive, "X:" );
  psz_drive[0] = env->gen.source_name[0];
  op.lpstrElementName = psz_drive;
  
  /* Set the flags for the device type */
  i_flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
    MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE;
  
  if( wnaspi32_mciSendCommand( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem = MCI_STATUS_READY;
    /* Eject disc */
    ret = wnaspi32_mciSendCommand( op.wDeviceID, MCI_SET, 
				 MCI_SET_DOOR_OPEN, 0 ) != 0;
    /* Release access to the device */
    wnaspi32_mciSendCommand( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
  } else 
    ret = 0;
  
  return ret;
}
#endif

/*!  
  Get format of track. 
*/
track_format_t
wnaspi32_get_track_format(_img_private_t *env, track_t track_num) 
{
  MCI_OPEN_PARMS op;
  MCI_STATUS_PARMS st;
  DWORD i_flags;
  int ret;

  memset( &op, 0, sizeof(MCI_OPEN_PARMS) );
  op.lpstrDeviceType = (LPCSTR)MCI_DEVTYPE_CD_AUDIO;
  op.lpstrElementName = env->gen.source_name;
  
  /* Set the flags for the device type */
  i_flags = MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID |
    MCI_OPEN_ELEMENT | MCI_OPEN_SHAREABLE;
  
  if( wnaspi32_mciSendCommand( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem  = MCI_CDA_STATUS_TYPE_TRACK;
    st.dwTrack = track_num;
    i_flags = MCI_TRACK | MCI_STATUS_ITEM ;
    ret = wnaspi32_mciSendCommand( op.wDeviceID, MCI_STATUS, i_flags, &st );
    
    /* Release access to the device */
    wnaspi32_mciSendCommand( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
    
    switch(st.dwReturn) {
    case MCI_CDA_TRACK_AUDIO:
      return TRACK_FORMAT_AUDIO;
    case MCI_CDA_TRACK_OTHER:
      return TRACK_FORMAT_DATA;
    default:
      return TRACK_FORMAT_XA;
    }
  }
  return TRACK_FORMAT_ERROR;
}

#endif /* HAVE_WIN32_CDROM */


