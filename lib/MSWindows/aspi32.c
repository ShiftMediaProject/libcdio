/*
    $Id: aspi32.c,v 1.17 2004/07/13 03:59:09 rocky Exp $

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

static const char _rcsid[] = "$Id: aspi32.c,v 1.17 2004/07/13 03:59:09 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/scsi_mmc.h>
#include "cdio_assert.h"

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
#include "cdtext_private.h"

/* Amount of time we are willing to wait for an operation to complete.
   10 seconds? 
*/
#define OP_TIMEOUT_MS 10000

static const 
char *aspierror(int nErrorCode) 
{
  switch (nErrorCode)
    {
    case SS_PENDING:
      return "SRB being processed";
      break;
    case SS_COMP:
      return "SRB completed without error";
      break;
    case SS_ABORTED:
      return "SRB aborted";
      break;
    case SS_ABORT_FAIL:
      return "Unable to abort SRB";
      break;
    case SS_ERR:
      return "SRB completed with error";
      break;
    case SS_INVALID_CMD:
      return "Invalid ASPI command";
      break;
    case SS_INVALID_HA:
      return "Invalid host adapter number";
      break;
    case SS_NO_DEVICE:
      return "SCSI device not installed";
      break;
    case SS_INVALID_SRB:
      return "Invalid parameter set in SRB";
      break;
    case SS_OLD_MANAGER:
      return "ASPI manager doesn't support";
      break;
    case SS_ILLEGAL_MODE:
      return "Unsupported MS Windows mode";
      break;
    case SS_NO_ASPI:
      return "No ASPI managers";
      break;
    case SS_FAILED_INIT:
      return "ASPI for windows failed init";
      break;
    case SS_ASPI_IS_BUSY:
      return "No resources available to execute command.";
      break;
    case SS_BUFFER_TOO_BIG:
      return "Buffer size is too big to handle.";
      break;
    case SS_MISMATCHED_COMPONENTS:
      return "The DLLs/EXEs of ASPI don't version check";
      break;
    case SS_NO_ADAPTERS:
      return "No host adapters found";
      break;
    case SS_INSUFFICIENT_RESOURCES:
      return "Couldn't allocate resources needed to init";
      break;
    case SS_ASPI_IS_SHUTDOWN:
      return "Call came to ASPI after PROCESS_DETACH";
      break;
    case SS_BAD_INSTALL:
      return "The DLL or other components are installed wrong.";
      break;
    default: 
      return "Unknown ASPI error.";
    }
}

/* General ioctl() CD-ROM command function */
static bool 
mciSendCommand_aspi(int id, UINT msg, DWORD flags, void *arg)
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

/*
  See if the ASPI DLL is loadable. If so pointers are returned
  and we return true. Return false if there was a problem.
 */
static bool
have_aspi( HMODULE *hASPI, 
	   long (**lpGetSupport)( void ), 
	   long (**lpSendCommand)( void* ) )
{
  /* check if aspi is available */
  *hASPI = LoadLibrary( "wnaspi32.dll" );

  if( *hASPI == NULL ) {
    cdio_debug("Unable to load ASPI DLL");
    return false;
  }

  (FARPROC) *lpGetSupport = GetProcAddress( *hASPI,
					    "GetASPI32SupportInfo" );
  (FARPROC) *lpSendCommand = GetProcAddress( *hASPI,
					     "SendASPI32Command" );
  
  /* make sure that we've got both function addresses */
  if( *lpGetSupport == NULL || *lpSendCommand == NULL ) {
    cdio_debug("Unable to get ASPI function pointers");
    FreeLibrary( *hASPI );
    return false;
  }

  return true;
}

const char *
is_cdrom_aspi(const char drive_letter) 
{
  static char psz_win32_drive[7];
  HMODULE hASPI = NULL;
  long (*lpGetSupport)( void ) = NULL;
  long (*lpSendCommand)( void* ) = NULL;
  DWORD dwSupportInfo;
  int i_adapter, i_hostadapters;
  char c_drive;
  int i_rc;

  if ( !have_aspi(&hASPI, &lpGetSupport, &lpSendCommand) )
    return NULL;

  /* ASPI support seems to be there. */
  
  dwSupportInfo = lpGetSupport();

  i_rc = HIBYTE( LOWORD ( dwSupportInfo ) );

  if( SS_COMP != i_rc ) {
    cdio_debug("ASPI: %s", aspierror(i_rc));
    FreeLibrary( hASPI );
    return NULL;
  }
  
  i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
  if( i_hostadapters == 0 ) {
    FreeLibrary( hASPI );
    return NULL;
  }
  
  c_drive = toupper(drive_letter) - 'A';
  
  for( i_adapter = 0; i_adapter < i_hostadapters; i_adapter++ ) {
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
	      /* Make sure this is a CD-ROM device. */
	      struct SRB_GDEVBlock   srbGDEVBlock;
	      
	      memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
	      srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
	      srbDiskInfo.SRB_HaId    = i_adapter;
	      srbGDEVBlock.SRB_Target = i_target;
	      srbGDEVBlock.SRB_Lun    = i_lun;
	      
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
init_aspi (_img_private_t *env)
{
  HMODULE hASPI = NULL;
  long (*lpGetSupport)( void ) = NULL;
  long (*lpSendCommand)( void* ) = NULL;
  DWORD dwSupportInfo;
  int i_adapter, i_hostadapters;
  char c_drive;
  int i_rc;

  if (2 == strlen(env->gen.source_name) && isalpha(env->gen.source_name[0]) ) 
  {
    c_drive = env->gen.source_name[0];
  } else if ( 6 == strlen(env->gen.source_name) 
	      && isalpha(env->gen.source_name[4] )) {
    c_drive = env->gen.source_name[4];
  }
  
  if ( !have_aspi(&hASPI, &lpGetSupport, &lpSendCommand) )
    return NULL;

  /* ASPI support seems to be there. */
  
  dwSupportInfo = lpGetSupport();
  
  i_rc = HIBYTE( LOWORD ( dwSupportInfo ) );

  if( SS_COMP != i_rc ) {
    cdio_info("ASPI: %s", aspierror(i_rc));
    FreeLibrary( hASPI );
    return false;
  }
  
  i_hostadapters = LOBYTE( LOWORD( dwSupportInfo ) );
  if( i_hostadapters == 0 ) {
    FreeLibrary( hASPI );
    return false;
  }
  
  c_drive = toupper(c_drive) - 'A';
  
  for( i_adapter = 0; i_adapter < i_hostadapters; i_adapter++ ) {
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
	for (i_lun = 0; i_lun < 8; i_lun++ ) {
	  srbDiskInfo.SRB_Cmd         = SC_GET_DISK_INFO;
	  srbDiskInfo.SRB_Flags       = 0;
	  srbDiskInfo.SRB_Hdr_Rsvd    = 0;
	  srbDiskInfo.SRB_HaId        = i_adapter;
	  srbDiskInfo.SRB_Target      = i_target;
	  srbDiskInfo.SRB_Lun         = i_lun;
	  
	  lpSendCommand( (void*) &srbDiskInfo );
	  
	  if( (srbDiskInfo.SRB_Status == SS_COMP) ) {
	    
	    if (srbDiskInfo.SRB_Int13HDriveInfo != c_drive)
	      {
		continue;
	      } else {
		/* Make sure this is a CD-ROM device. */
		struct SRB_GDEVBlock   srbGDEVBlock;
		
		memset( &srbGDEVBlock, 0, sizeof(struct SRB_GDEVBlock) );
		srbGDEVBlock.SRB_Cmd    = SC_GET_DEV_TYPE;
		srbGDEVBlock.SRB_HaId   = i_adapter;
		srbGDEVBlock.SRB_Target = i_target;
		
		lpSendCommand( (void*) &srbGDEVBlock );
		
		if( ( srbGDEVBlock.SRB_Status == SS_COMP ) &&
		    ( srbGDEVBlock.SRB_DeviceType == DTYPE_CDROM ) ) {
		  env->i_sid = MAKEWORD( i_adapter, i_target );
		  env->hASPI = (long)hASPI;
		  env->lpSendCommand = lpSendCommand;
		  env->b_aspi_init   = true;
		  env->i_lun         = i_lun;
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
mmc_read_sectors_aspi (const _img_private_t *env, void *data, lsn_t lsn, 
		       int sector_type, unsigned int nblocks)
{
  HANDLE hEvent;
  struct SRB_ExecSCSICmd ssc;

#if 0  
  sector_type = 0; /*all types */
#endif
  int sync        = 0;
  int header_code = 2;
  int i_user_data   = 1;
  int edc_ecc     = 0;
  int error_field = 0;
  
  
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
  ssc.SRB_Lun         = env->i_lun;
  ssc.SRB_SenseLen    = SENSE_LEN;
  
  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen      = 12;
  
  CDIO_MMC_SET_COMMAND(ssc.CDBByte, CDIO_MMC_GPCMD_READ_CD);
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
  ssc.SRB_BufPointer  = data;

  switch (sector_type) {
  case CDIO_MMC_READ_TYPE_ANY: 
  case CDIO_MMC_READ_TYPE_CDDA: 
    ssc.SRB_BufLen = CDIO_CD_FRAMESIZE_RAW;
    break;
  case CDIO_MMC_READ_TYPE_M2F1:
    ssc.SRB_BufLen = CDIO_CD_FRAMESIZE;
    break;
  case CDIO_MMC_READ_TYPE_M2F2:
    ssc.SRB_BufLen = 2324;
    break;
  case CDIO_MMC_READ_TYPE_MODE1:
    ssc.SRB_BufLen = CDIO_CD_FRAMESIZE;
    break;
  default:
    ssc.SRB_BufLen = CDIO_CD_FRAMESIZE_RAW;
  }
  
  /* Initiate transfer */
  ResetEvent( hEvent );
  env->lpSendCommand( (void*) &ssc );
  
  /* If the command has still not been processed, wait until it's
   * finished */
  if( ssc.SRB_Status == SS_PENDING ) {
    WaitForSingleObject( hEvent, OP_TIMEOUT_MS );
  }
  CloseHandle( hEvent );
  
  /* check that the transfer went as planned */
  if( ssc.SRB_Status != SS_COMP ) {
    cdio_info("ASPI: %s", aspierror(ssc.SRB_Status));
    return 1;
  }

  return 0;
}

/*!
   Reads an audio device into data starting from lsn.
   Returns 0 if no error. 
 */
int
read_audio_sectors_aspi (_img_private_t *env, void *data, lsn_t lsn, 
			     unsigned int nblocks) 
{
  if (mmc_read_sectors_aspi(env, data, lsn, CDIO_MMC_READ_TYPE_CDDA, 1)) {
    return mmc_read_sectors_aspi(env, data, lsn, CDIO_MMC_READ_TYPE_ANY, 1);
  }
  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
read_mode2_sector_aspi (const _img_private_t *env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  return mmc_read_sectors_aspi(env, data, lsn, b_form2 
			       ? CDIO_MMC_READ_TYPE_M2F2 
			       : CDIO_MMC_READ_TYPE_M2F1, 
			       1);
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
read_mode1_sector_aspi (const _img_private_t *env, void *data, lsn_t lsn, 
			 bool b_form2)
{
  return mmc_read_sectors_aspi(env, data, lsn, CDIO_MMC_READ_TYPE_MODE1, 1);
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool
read_toc_aspi (_img_private_t *env) 
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
  ssc.SRB_Lun         = env->i_lun;
  ssc.SRB_SenseLen    = SENSE_LEN;
  
  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen      = 10;
  
  /* Operation code */
  CDIO_MMC_SET_COMMAND(ssc.CDBByte, CDIO_MMC_GPCMD_READ_TOC);
  
  /* Format */
  ssc.CDBByte[ 2 ] = READ_TOC_FORMAT_TOC;
  
  /* Starting track */
  CDIO_MMC_SET_START_TRACK(ssc.CDBByte, 0);
  
  /* Allocation length and buffer */
  ssc.SRB_BufLen      = sizeof( p_tocheader );
  ssc.SRB_BufPointer  = p_tocheader;
  CDIO_MMC_SET_READ_LENGTH(ssc.CDBByte, ssc.SRB_BufLen);
  
  /* Initiate transfer */
  ResetEvent( hEvent );
  env->lpSendCommand( (void*) &ssc );
  
  /* If the command has still not been processed, wait until it's
   * finished */
  if( ssc.SRB_Status == SS_PENDING )
    WaitForSingleObject( hEvent, OP_TIMEOUT_MS );
  
  /* check that the transfer went as planned */
  if( ssc.SRB_Status != SS_COMP ) {
    cdio_info("ASPI: %s", aspierror(ssc.SRB_Status));
    CloseHandle( hEvent );
    return false;
  }
  
  env->i_first_track = p_tocheader[2];
  env->i_tracks  = p_tocheader[3] - p_tocheader[2] + 1;
  
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
    ssc.CDBByte[ 7 ] = (unsigned char) ( ssc.SRB_BufLen >>  8 ) & 0xff;
    ssc.CDBByte[ 8 ] = (unsigned char) ( ssc.SRB_BufLen       ) & 0xff;
    
    /* Initiate transfer */
    ResetEvent( hEvent );
    env->lpSendCommand( (void*) &ssc );
    
    /* If the command has still not been processed, wait until it's
     * finished */
    if( ssc.SRB_Status == SS_PENDING )
      WaitForSingleObject( hEvent, OP_TIMEOUT_MS );
    
    /* check that the transfer went as planned */
    if( ssc.SRB_Status != SS_COMP ) {
      cdio_info("ASPI: %s", aspierror(ssc.SRB_Status));
      env->i_tracks = 0;
    }
    
    for( i = 0 ; i <= env->i_tracks ; i++ ) {
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
  
  if( mciSendCommand_aspi( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem = MCI_STATUS_READY;
    /* Eject disc */
    ret = mciSendCommand_aspi( op.wDeviceID, MCI_SET, 
				 MCI_SET_DOOR_OPEN, 0 ) != 0;
    /* Release access to the device */
    mciSendCommand_aspi( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
  } else 
    ret = 0;
  
  return ret;
}
#endif

#define set_cdtext_field(FIELD)						\
  if( i_track == 0 )							\
    env->cdtext.field[FIELD] = strdup(buffer);				\
  else									\
    env->tocent[i_track-1].cdtext.field[FIELD]				\
      = strdup(buffer);							\
  i_track++;								\
  idx = 0;

/*!
  Return the value associated with the key "arg".
*/
const cdtext_t *
get_cdtext_aspi (_img_private_t *env)
{
  uint8_t  bigbuffer[5000];
  HANDLE hEvent;
  struct SRB_ExecSCSICmd ssc;

  /* Create the transfer completion event */
  hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
  if( hEvent == NULL ) {
    return NULL;
  }
  
  memset( &ssc, 0, sizeof( ssc ) );

  ssc.SRB_Cmd         = SC_EXEC_SCSI_CMD;
  ssc.SRB_Flags       = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  ssc.SRB_HaId        = LOBYTE( env->i_sid );
  ssc.SRB_Target      = HIBYTE( env->i_sid );
  ssc.SRB_Lun         = env->i_lun;
  ssc.SRB_SenseLen    = SENSE_LEN;
  
  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen      = 10;
  
  CDIO_MMC_SET_COMMAND(ssc.CDBByte, CDIO_MMC_GPCMD_READ_TOC); 
  ssc.CDBByte[1]   = 0x02;   // MSF mode 
  ssc.CDBByte[2]   = 0x05;   // CD text
  CDIO_MMC_SET_READ_LENGTH(ssc.CDBByte, sizeof(bigbuffer));

  /* Result buffer */
  ssc.SRB_BufPointer  = bigbuffer;
  ssc.SRB_BufLen      = sizeof(bigbuffer);

  memset ( bigbuffer, 0, sizeof(bigbuffer) );

  /* Initiate transfer */
  ResetEvent( hEvent );
  env->lpSendCommand( (void*) &ssc );
  
  /* If the command has still not been processed, wait until it's
   * finished */
  if( ssc.SRB_Status == SS_PENDING ) {
    WaitForSingleObject( hEvent, OP_TIMEOUT_MS );
  }
  CloseHandle( hEvent );
  
  /* check that the transfer went as planned */
  if( ssc.SRB_Status != SS_COMP ) {
    cdio_info("ASPI: %s", aspierror(ssc.SRB_Status));
    return NULL;
  }

  {
    CDText_data_t *pdata;
    int           i;
    int           j;
    char          buffer[256];
    int           idx;
    int           i_track;

    memset( buffer, 0x00, sizeof(buffer) );
    idx = 0;
  
    pdata = (CDText_data_t *) (&bigbuffer[4]);
    for( i=0; i < CDIO_CDTEXT_MAX_PACK_DATA; i++ ) {
      if( pdata->seq != i )
	break;
      
      if( (pdata->type >= 0x80) 
	  && (pdata->type <= 0x85) && (pdata->block == 0) ) {
	i_track = pdata->i_track;
	
	for( j=0; j < CDIO_CDTEXT_MAX_TEXT_DATA; j++ ) {
	  if( pdata->text[j] == 0x00 )
	    {
	      switch( pdata->type) {
	      case CDIO_CDTEXT_TITLE: 
		set_cdtext_field(CDTEXT_TITLE);
		break;
	      case CDIO_CDTEXT_PERFORMER:  
		set_cdtext_field(CDTEXT_PERFORMER);
		break;
	      case CDIO_CDTEXT_SONGWRITER:
		set_cdtext_field(CDTEXT_SONGWRITER);
		break;
	      case CDIO_CDTEXT_COMPOSER:
		set_cdtext_field(CDTEXT_COMPOSER);
		break;
	      case CDIO_CDTEXT_ARRANGER:
		set_cdtext_field(CDTEXT_ARRANGER);
		break;
	      case CDIO_CDTEXT_MESSAGE:
		set_cdtext_field(CDTEXT_MESSAGE);
		break;
	      case CDIO_CDTEXT_DISCID: 
		set_cdtext_field(CDTEXT_DISCID);
		break;
	      case CDIO_CDTEXT_GENRE: 
		set_cdtext_field(CDTEXT_GENRE);
		break;
	      }
	    }
	  else 	    {
	    buffer[idx++] = pdata->text[j];
	  }
	  buffer[idx] = 0x00;
	}
      }
      pdata++;
    }
  }

  env->b_cdtext_init = true;
  return &(env->cdtext);
}
/*!
  Return the the kind of drive capabilities of device.

 */
cdio_drive_cap_t
get_drive_cap_aspi (const _img_private_t *env) 
{
  int32_t i_drivetype = CDIO_DRIVE_CAP_CD_AUDIO | CDIO_DRIVE_CAP_UNKNOWN;
  BYTE        buf[256] = { 0, };
  HANDLE hEvent;
  struct SRB_ExecSCSICmd ssc;

  /* Create the transfer completion event */
  hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
  if( hEvent == NULL ) {
    return CDIO_DRIVE_CAP_ERROR;
  }

  memset( &ssc, 0, sizeof( ssc ) );
  
  /* If device supports SCSI-3, then we can get the CD drive
     capabilities, i.e. ability to read/write to CD-ROM/R/RW
     or/and read/write to DVD-ROM/R/RW.   */
  
  ssc.SRB_Cmd      = SC_EXEC_SCSI_CMD;
  ssc.SRB_Flags    = SRB_DIR_IN | SRB_EVENT_NOTIFY;
  ssc.SRB_HaId     = LOBYTE( env->i_sid );
  ssc.SRB_Target   = HIBYTE( env->i_sid );
  ssc.SRB_Lun      = env->i_lun;
  ssc.SRB_SenseLen = SENSE_LEN;

  ssc.SRB_PostProc = (LPVOID) hEvent;
  ssc.SRB_CDBLen   = 12;

  /* Operation code */
  CDIO_MMC_SET_COMMAND(ssc.CDBByte, CDIO_MMC_GPCMD_MODE_SENSE_10);
  ssc.CDBByte[1]             = 0x0;
  ssc.CDBByte[2]             = CDIO_MMC_ALL_PAGES;
  ssc.CDBByte[7]             = 0x01;
  ssc.CDBByte[8]             = 0x00; 

  /* Allocation length and buffer */
  ssc.SRB_BufPointer  = buf;
  ssc.SRB_BufLen = sizeof( buf );
  
  /* Initiate transfer */
  ResetEvent( hEvent );
  env->lpSendCommand( (void*) &ssc );
  
  /* If the command has still not been processed, wait until it's
   * finished */
  if( ssc.SRB_Status == SS_PENDING )
    WaitForSingleObject( hEvent, OP_TIMEOUT_MS );
  
  CloseHandle( hEvent );

  /* check that the transfer went as planned */
  if( ssc.SRB_Status != SS_COMP ) {
    cdio_info("ASPI: %s", aspierror(ssc.SRB_Status));
    return i_drivetype;
  } else {
    BYTE *p;
    int lenData  = ((unsigned int)buf[0] << 8) + buf[1];
    BYTE *pMax = buf + 256;

    i_drivetype = 0;
    /* set to first sense mask, and then walk through the masks */
    p = buf + 8;
    while( (p < &(buf[2+lenData])) && (p < pMax) )       {
      BYTE which;
      
      which = p[0] & 0x3F;
      switch( which )
	{
	case CDIO_MMC_AUDIO_CTL_PAGE:
	case CDIO_MMC_R_W_ERROR_PAGE:
	case CDIO_MMC_CDR_PARMS_PAGE:
	  /* Don't handle these yet. */
	  break;
	case CDIO_MMC_CAPABILITIES_PAGE:
	  i_drivetype |= cdio_get_drive_cap_mmc(p);
	  break;
	default: ;
	}
      p += (p[1] + 2);
    }
  }
  return i_drivetype;
}

/*!  
  Get format of track. 
*/
track_format_t
get_track_format_aspi(const _img_private_t *env, track_t track_num) 
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
  
  if( mciSendCommand_aspi( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem  = MCI_CDA_STATUS_TYPE_TRACK;
    st.dwTrack = track_num;
    i_flags = MCI_TRACK | MCI_STATUS_ITEM ;
    ret = mciSendCommand_aspi( op.wDeviceID, MCI_STATUS, i_flags, &st );
    
    /* Release access to the device */
    mciSendCommand_aspi( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
    
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


