/*
    $Id: _cdio_osx.c,v 1.57 2004/08/22 00:43:07 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com> 
    from vcdimager code: 
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    and VideoLAN code Copyright (C) 1998-2001 VideoLAN
      Authors: Johan Bilien <jobi@via.ecp.fr>
               Gildas Bazin <gbazin@netcourrier.com>
               Jon Lech Johansen <jon-vl@nanocrew.net>
               Derk-Jan Hartman <hartman at videolan.org>
               Justin F. Hallett <thesin@southofheaven.org>

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

/* This file contains OSX-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_osx.c,v 1.57 2004/08/22 00:43:07 rocky Exp $";

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"

#include <string.h>

#ifdef HAVE_DARWIN_CDROM
#undef VERSION 

#include <mach/mach.h>
#include <Carbon/Carbon.h>
#include <IOKit/scsi-commands/SCSITaskLib.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
#include <mach/mach_error.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>


#include <paths.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFNumber.h>
#include <CoreFoundation/CFString.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/scsi-commands/IOSCSIMultimediaCommandsDevice.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IODVDTypes.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#include <IOKit/storage/IODVDMediaBSDClient.h>
#include <IOKit/storage/IOStorageDeviceCharacteristics.h>

/* Note leadout is normally defined 0xAA, But on OSX 0xA0 is "lead in" while
   0xA2 is "lead out". Don't ask me why. */
#define	OSX_CDROM_LEADOUT_TRACK 0xA2

#define TOTAL_TRACKS    (p_env->i_last_track - p_env->i_first_track + 1)

#define CDROM_CDI_TRACK 0x1
#define CDROM_XA_TRACK  0x2

typedef enum {
  _AM_NONE,
  _AM_OSX,
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 

  access_mode_t access_mode;

  /* Track information */
  CDTOC *pTOC;
  int i_descriptors;
  track_t i_last_track;      /* highest track number */
  track_t i_first_track;     /* first track */
  track_t i_last_session;    /* highest session number */
  track_t i_first_session;   /* first session number */
  lsn_t   *pp_lba;
  CFMutableDictionaryRef dict;
  io_service_t service;
  SCSITaskDeviceInterface **pp_scsiTaskDeviceInterface;

} _img_private_t;

static bool read_toc_osx (void *p_user_data);

static bool 
init_osx(_img_private_t *p_env) {
  mach_port_t port;
  char *psz_devname;
  kern_return_t ret;
  io_iterator_t iterator;
  
  p_env->gen.fd = open( p_env->gen.source_name, O_RDONLY | O_NONBLOCK );
  if (-1 == p_env->gen.fd) {
    cdio_warn("Failed to open %s: %s", p_env->gen.source_name,
	       strerror(errno));
    return false;
  }

  /* get the device name */
  if( ( psz_devname = strrchr( p_env->gen.source_name, '/') ) != NULL )
    ++psz_devname;
  else
    psz_devname = p_env->gen.source_name;
  
  /* unraw the device name */
  if( *psz_devname == 'r' )
    ++psz_devname;
  
  /* get port for IOKit communication */
  if( ( ret = IOMasterPort( MACH_PORT_NULL, &port ) ) != KERN_SUCCESS )
    {
      cdio_warn( "IOMasterPort: 0x%08x", ret );
      return false;
    }
  
  /* get service iterator for the device */
  if( ( ret = IOServiceGetMatchingServices( 
					   port, IOBSDNameMatching( port, 0, psz_devname ),
					   &iterator ) ) != KERN_SUCCESS )
    {
        cdio_warn( "IOServiceGetMatchingServices: 0x%08x", ret );
        return false;
    }
  
  /* first service */
  p_env->service = IOIteratorNext( iterator );
  IOObjectRelease( iterator );
  
  /* search for kIOCDMediaClass */ 
  while( p_env->service && !IOObjectConformsTo( p_env->service, kIOCDMediaClass ) )
    {

      ret = IORegistryEntryGetParentIterator( p_env->service, kIOServicePlane, 
					      &iterator );
      if( ret != KERN_SUCCESS )
        {
	  cdio_warn( "IORegistryEntryGetParentIterator: 0x%08x", ret );
	  IOObjectRelease( p_env->service );
	  return false;
        }
      
      IOObjectRelease( p_env->service );
      p_env->service = IOIteratorNext( iterator );
      IOObjectRelease( iterator );
    }
  
  if( p_env->service == 0 )
    {
      cdio_warn( "search for kIOCDMediaClass came up empty" );
      return false;
    }

  /* create a CF dictionary containing the TOC */
  ret = IORegistryEntryCreateCFProperties( p_env->service, &(p_env->dict),
					   kCFAllocatorDefault, kNilOptions );
  
  if(  ret != KERN_SUCCESS )
    {
      cdio_warn( "IORegistryEntryCreateCFProperties: 0x%08x", ret );
      IOObjectRelease( p_env->service );
      return false;
    }
  return true;
}

/*!
  Run a SCSI MMC command. 
 
  cdio	        CD structure set by cdio_open().
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  We return true if command completed successfully and false if not.
 */
static int
run_scsi_cmd_osx( const void *p_user_data, 
		  unsigned int i_timeout_ms,
		  unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
		  scsi_mmc_direction_t e_direction, 
		  unsigned int i_buf, /*in/out*/ void *p_buf )
{

#ifndef SCSI_MMC_FIXED
  return 2;
#else 
  const _img_private_t *p_env = p_user_data;
  SCSITaskDeviceInterface **sc;
  SCSITaskInterface **cmd = NULL;
  IOVirtualRange iov;
  SCSI_Sense_Data senseData;
  SCSITaskStatus status;
  UInt64 bytesTransferred;
  IOReturn ioReturnValue;
  int ret = 0;

  if (NULL == p_user_data) return 2;

  /* Make sure pp_scsiTaskDeviceInterface is initialized. FIXME: The code
     should probably be reorganized better for this. */
  if (!p_env->gen.toc_init) read_toc_osx (p_user_data) ;

  sc = p_env->pp_scsiTaskDeviceInterface;

  if (NULL == sc) return 3;

  cmd = (*sc)->CreateSCSITask(sc);
  if (cmd == NULL) {
    cdio_warn("Failed to create SCSI task");
    return -1;
  }

  iov.address = (IOVirtualAddress) p_buf;
  iov.length = i_buf;

  ioReturnValue = (*cmd)->SetCommandDescriptorBlock(cmd, (UInt8 *) p_cdb, 
						    i_cdb);
  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("SetCommandDescriptorBlock failed with status %x", 
	      ioReturnValue);
    return -1;
  }

  ioReturnValue = (*cmd)->SetScatterGatherEntries(cmd, &iov, 1, i_buf,
						  (SCSI_MMC_DATA_READ == e_direction ) ? 
						  kSCSIDataTransfer_FromTargetToInitiator :
						  kSCSIDataTransfer_FromInitiatorToTarget);
  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("SetScatterGatherEntries failed with status %x", ioReturnValue);
    return -1;
  }

  ioReturnValue = (*cmd)->SetTimeoutDuration(cmd, i_timeout_ms );
  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("SetTimeoutDuration failed with status %x", ioReturnValue);
    return -1;
  }

  memset(&senseData, 0, sizeof(senseData));

  ioReturnValue = (*cmd)->ExecuteTaskSync(cmd,
					  &senseData, &status, &bytesTransferred);

  if (ioReturnValue != kIOReturnSuccess) {
    cdio_warn("Command execution failed with status %x", ioReturnValue);
    return -1;
  }

  if (cmd != NULL) {
    (*cmd)->Release(cmd);
  }

  return (ret);
#endif
}

/***************************************************************************
 * GetFeaturesFlagsForDrive -Gets the bitfield which represents the
 * features flags.
 ***************************************************************************/

static bool
GetFeaturesFlagsForDrive ( CFMutableDictionaryRef dict,
			   uint32_t *i_cdFlags,
			   uint32_t *i_dvdFlags )
{
  CFDictionaryRef propertiesDict = 0;
  CFNumberRef     flagsNumberRef = 0;
  
  *i_cdFlags = 0;
  *i_dvdFlags= 0;
  
  propertiesDict = ( CFDictionaryRef ) 
    CFDictionaryGetValue ( dict, 
			   CFSTR ( kIOPropertyDeviceCharacteristicsKey ) );
  if ( propertiesDict == 0 ) return false;
  
  /* Get the CD features */
  flagsNumberRef = ( CFNumberRef ) 
    CFDictionaryGetValue ( propertiesDict, 
			   CFSTR ( kIOPropertySupportedCDFeatures ) );
  if ( flagsNumberRef != 0 ) {
    CFNumberGetValue ( flagsNumberRef, kCFNumberLongType, i_cdFlags );
  }
  
  /* Get the DVD features */
  flagsNumberRef = ( CFNumberRef ) 
    CFDictionaryGetValue ( propertiesDict, 
			   CFSTR ( kIOPropertySupportedDVDFeatures ) );
  if ( flagsNumberRef != 0 ) {
    CFNumberGetValue ( flagsNumberRef, kCFNumberLongType, i_dvdFlags );
  }

  return true;
}

static void
get_drive_cap_osx(const void *p_user_data,
		  /*out*/ cdio_drive_read_cap_t  *p_read_cap,
		  /*out*/ cdio_drive_write_cap_t *p_write_cap,
		  /*out*/ cdio_drive_misc_cap_t  *p_misc_cap)
{
  const _img_private_t *p_env = p_user_data;
  uint32_t i_cdFlags;
  uint32_t i_dvdFlags;

  if (! GetFeaturesFlagsForDrive ( p_env->dict, &i_cdFlags, &i_dvdFlags ) )
    return;

  /* Reader */

  if ( 0 != (i_cdFlags & kCDFeaturesAnalogAudioMask) )
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_AUDIO;      

  if ( 0 != (i_cdFlags & kCDFeaturesWriteOnceMask) ) 
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_R;

  if ( 0 != (i_cdFlags & kCDFeaturesCDDAStreamAccurateMask) )
    *p_read_cap  |= CDIO_DRIVE_CAP_READ_CD_DA;

  if ( 0 != (i_dvdFlags & kDVDFeaturesReadStructuresMask) )
      *p_read_cap  |= CDIO_DRIVE_CAP_READ_DVD_ROM;

  if ( 0 != (i_cdFlags & kCDFeaturesReWriteableMask) )
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_CD_RW;
  
  if ( 0 != (i_dvdFlags & kDVDFeaturesWriteOnceMask) ) 
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_R;
  
  if ( 0 != (i_dvdFlags & kDVDFeaturesRandomWriteableMask) )
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_RAM;
  
  if ( 0 != (i_dvdFlags & kDVDFeaturesReWriteableMask) )
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_RW;

  /***
  if ( 0 != (i_dvdFlags & kDVDFeaturesPlusRMask) )
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_PR;
  
  if ( 0 != (i_dvdFlags & kDVDFeaturesPlusRWMask )
    *p_write_cap |= CDIO_DRIVE_CAP_WRITE_DVD_PRW;
    ***/

}

#if ADDED_DRIVE_INFO
/****************************************************************************
 * GetDriveDescription - Gets drive description. 
 ****************************************************************************/

static bool
GetDriveDescription ( CFMutableDictionaryRef dict,
		      /*out*/ scsi_mmc_hwinfo_t *hw_info)
{
  
  CFDictionaryRef    deviceDict  = NULL;
  CFStringRef        vendor      = NULL;
  CFStringRef        product     = NULL;
  CFStringRef        revision    = NULL;
  
  if ( dict != 0 ) return false;
  
  deviceDict = ( CFDictionaryRef ) 
    CFDictionaryGetValue ( dict, 
			   CFSTR ( kIOPropertyDeviceCharacteristicsKey ) );

  if ( deviceDict != 0 ) return false;
  
  vendor = ( CFStringRef ) 
    CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyVendorNameKey ) );

  if( CFStringGetCString( vendor,
			  (char *) &(hw_info->vendor),
			  sizeof(hw_info->vendor),
			  kCFStringEncodingASCII ) )
    CFRelease( vendor );

  product = ( CFStringRef ) 
    CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyProductNameKey ) );
  
  if( CFStringGetCString( product,
			  (char *) &(hw_info->model),
			  sizeof(hw_info->model),
			  kCFStringEncodingASCII ) )
    CFRelease( product );
  
  revision = ( CFStringRef ) 
    CFDictionaryGetValue ( deviceDict, CFSTR ( kIOPropertyProductRevisionLevelKey ) );
  
  if( CFStringGetCString( product,
			  (char *) &(hw_info->revision),
			  sizeof(hw_info->revision),
			  kCFStringEncodingASCII ) )
    CFRelease( revision );
  
  return true;
  
}
#endif

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static const cdtext_t *
get_cdtext_osx (void *p_user_data, track_t i_track) 
{
  return NULL;
}

static void 
_free_osx (void *p_user_data) {
  _img_private_t *p_env = p_user_data;
  if (NULL == p_env) return;
  cdio_generic_free(p_env);
  if (NULL != p_env->pp_lba)  free((void *) p_env->pp_lba);
  if (NULL != p_env->pTOC)    free((void *) p_env->pTOC);
  if (NULL != p_env->dict)    CFRelease( p_env->dict ); 
  IOObjectRelease( p_env->service );

  if (NULL != p_env->pp_scsiTaskDeviceInterface) 
    ( *(p_env->pp_scsiTaskDeviceInterface) )->
      Release ( (p_env->pp_scsiTaskDeviceInterface) );

}

/*!
   Reads nblocks of mode2 form2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_get_read_mode1_sectors_osx (void *user_data, void *data, lsn_t lsn, 
			     bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  dk_cd_read_t cd_read;
  
  memset( &cd_read, 0, sizeof(cd_read) );
  
  cd_read.sectorArea  = kCDSectorAreaUser;
  cd_read.buffer      = data;
  cd_read.sectorType  = kCDSectorTypeMode1;
  
  if (b_form2) {
    cd_read.offset       = lsn * kCDSectorSizeMode2;
    cd_read.bufferLength = kCDSectorSizeMode2 * nblocks;
  } else {
    cd_read.offset       = lsn * kCDSectorSizeMode1;
    cd_read.bufferLength = kCDSectorSizeMode1 * nblocks;
  }
  
   if( ioctl( env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_info( "could not read block %d, %s", lsn, strerror(errno) );
    return -1;
  }
  return 0;
}

  
/*!
   Reads nblocks of mode2 form2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_get_read_mode2_sectors_osx (void *user_data, void *data, lsn_t lsn, 
			     bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  dk_cd_read_t cd_read;
  
  memset( &cd_read, 0, sizeof(cd_read) );
  
  cd_read.sectorArea = kCDSectorAreaUser;
  cd_read.buffer = data;
  
  if (b_form2) {
    cd_read.offset       = lsn * kCDSectorSizeMode2Form2;
    cd_read.sectorType   = kCDSectorTypeMode2Form2;
    cd_read.bufferLength = kCDSectorSizeMode2Form2 * nblocks;
  } else {
    cd_read.offset       = lsn * kCDSectorSizeMode2Form1;
    cd_read.sectorType   = kCDSectorTypeMode2Form1;
    cd_read.bufferLength = kCDSectorSizeMode2Form1 * nblocks;
  }
  
  if( ioctl( env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_info( "could not read block %d, %s", lsn, strerror(errno) );
    return -1;
  }
  return 0;
}

  
/*!
   Reads a single audio sector from CD device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
_get_read_audio_sectors_osx (void *user_data, void *data, lsn_t lsn, 
			     unsigned int nblocks)
{
  _img_private_t *env = user_data;
  dk_cd_read_t cd_read;
  
  memset( &cd_read, 0, sizeof(cd_read) );
  
  cd_read.offset       = lsn * kCDSectorSizeCDDA;
  cd_read.sectorArea   = kCDSectorAreaUser;
  cd_read.sectorType   = kCDSectorTypeCDDA;
  
  cd_read.buffer       = data;
  cd_read.bufferLength = kCDSectorSizeCDDA * nblocks;
  
  if( ioctl( env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_info( "could not read block %d", lsn );
    return -1;
  }
  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_get_read_mode1_sector_osx (void *user_data, void *data, lsn_t lsn, 
			    bool b_form2)
{
  return _get_read_mode1_sectors_osx(user_data, data, lsn, b_form2, 1);
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_get_read_mode2_sector_osx (void *user_data, void *data, lsn_t lsn, 
			    bool b_form2)
{
  return _get_read_mode2_sectors_osx(user_data, data, lsn, b_form2, 1);
}

/*!
  Set the key "arg" to "value" in source device.
*/
static int
_set_arg_osx (void *user_data, const char key[], const char value[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source"))
    {
      if (!value)
	return -2;

      free (env->gen.source_name);
      
      env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      if (!strcmp(value, "OSX"))
	env->access_mode = _AM_OSX;
      else
	cdio_warn ("unknown access type: %s. ignored.", value);
    }
  else 
    return -1;

  return 0;
}

static void TestDevice(_img_private_t *p_env, io_service_t service)
{
  SInt32                          score;
  HRESULT                         herr;
  kern_return_t                   err;
  IOCFPlugInInterface             **plugInInterface = NULL;
  MMCDeviceInterface              **mmcInterface = NULL;

  /* Create the IOCFPlugIn interface so we can query it. */

  err = IOCreatePlugInInterfaceForService ( service,
					    kIOMMCDeviceUserClientTypeID,
					    kIOCFPlugInInterfaceID,
					    &plugInInterface,
					    &score );
  if ( err != noErr ) {
    printf("IOCreatePlugInInterfaceForService returned %d\n", err);
    return;
  }
  
  /* Query the interface for the MMCDeviceInterface. */
  
  herr = ( *plugInInterface )->QueryInterface ( plugInInterface,
						CFUUIDGetUUIDBytes ( kIOMMCDeviceInterfaceID ),
						( LPVOID ) &mmcInterface );
  
  if ( herr != S_OK )     {
    printf("QueryInterface returned %ld\n", herr);
    return;
  }
  
  p_env->pp_scsiTaskDeviceInterface = 
    ( *mmcInterface )->GetSCSITaskDeviceInterface ( mmcInterface );
  
  if ( NULL == p_env->pp_scsiTaskDeviceInterface )  {
    printf("GetSCSITaskDeviceInterface returned NULL\n");
    return;
  }
  
  ( *mmcInterface )->Release ( mmcInterface );
  IODestroyPlugInInterface ( plugInInterface );
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return false if successful or true if an error.
*/
static bool
read_toc_osx (void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;
  CFDataRef data;

  /* get the TOC from the dictionary */
  data = (CFDataRef) CFDictionaryGetValue( p_env->dict,
					   CFSTR(kIOCDMediaTOCKey) );
  if( data  != NULL )
    {
      CFRange range;
      CFIndex buf_len;
      
      buf_len = CFDataGetLength( data ) + 1;
      range = CFRangeMake( 0, buf_len );
      
      if( ( p_env->pTOC = (CDTOC *)malloc( buf_len ) ) != NULL ) {
	CFDataGetBytes( data, range, (u_char *) p_env->pTOC );
      } else {
	cdio_warn( "Trouble allocating CDROM TOC" );
	return false;
      }
    }
  else
    {
      cdio_warn( "CFDictionaryGetValue failed" );
      return false;
    }

  /* TestDevice(p_env, service); */
  IOObjectRelease( p_env->service ); 

  p_env->i_descriptors = CDTOCGetDescriptorCount ( p_env->pTOC );

  /* Read in starting sectors. There may be non-tracks mixed in with
     the real tracks.  So find the first and last track number by
     scanning. Also find the lead-out track position.
   */
  {
    int i, i_leadout = -1;
    
    CDTOCDescriptor *pTrackDescriptors;
    
    p_env->pp_lba = malloc( p_env->i_descriptors * sizeof(int) );
    if( p_env->pp_lba == NULL )
      {
	cdio_warn("Out of memory in allocating track starting LSNs" );
	free( p_env->pTOC );
	return false;
      }
    
    pTrackDescriptors = p_env->pTOC->descriptors;

    p_env->i_first_track   = CDIO_CD_MAX_TRACKS+1;
    p_env->i_last_track    = CDIO_CD_MIN_TRACK_NO;
    p_env->i_first_session = CDIO_CD_MAX_TRACKS+1;
    p_env->i_last_session  = CDIO_CD_MIN_TRACK_NO;
    
    for( i = 0; i <= p_env->i_descriptors; i++ )
      {
	track_t i_track     = pTrackDescriptors[i].point;
	session_t i_session = pTrackDescriptors[i].session;

	cdio_debug( "point: %d, tno: %d, session: %d, adr: %d, control:%d, "
		    "address: %d:%d:%d, p: %d:%d:%d", 
		    i_track,
		    pTrackDescriptors[i].tno, i_session,
		    pTrackDescriptors[i].adr, pTrackDescriptors[i].control,
		    pTrackDescriptors[i].address.minute,
		    pTrackDescriptors[i].address.second,
		    pTrackDescriptors[i].address.frame, 
		    pTrackDescriptors[i].p.minute,
		    pTrackDescriptors[i].p.second, 
		    pTrackDescriptors[i].p.frame );

	/* track information has adr = 1 */
	if ( 0x01 != pTrackDescriptors[i].adr ) 
	  continue;

	if( i_track == OSX_CDROM_LEADOUT_TRACK )
	  i_leadout = i;

	if( i_track > CDIO_CD_MAX_TRACKS || i_track < CDIO_CD_MIN_TRACK_NO )
	  continue;

	if (p_env->i_first_track > i_track) 
	  p_env->i_first_track = i_track;
	
	if (p_env->i_last_track < i_track) 
	  p_env->i_last_track = i_track;
	
	if (p_env->i_first_session > i_session) 
	  p_env->i_first_track = i_session;
	
	if (p_env->i_last_session < i_session) 
	  p_env->i_last_track = i_session;
      }

    /* Now that we know what the first track number is, we can make sure
       index positions are ordered starting at 0.
     */
    for( i = 0; i <= p_env->i_descriptors; i++ )
      {
	track_t i_track = pTrackDescriptors[i].point;

	if( i_track > CDIO_CD_MAX_TRACKS || i_track < CDIO_CD_MIN_TRACK_NO )
	  continue;

	/* Note what OSX calls a LBA we call an LSN. So below re we 
	   really have have MSF -> LSN -> LBA.
	 */
	p_env->pp_lba[i_track - p_env->i_first_track] =
	  cdio_lsn_to_lba(CDConvertMSFToLBA( pTrackDescriptors[i].p ));
      }
    
    if( i_leadout == -1 )
      {
	cdio_warn( "CD leadout not found" );
	free( p_env->pp_lba );
	free( (void *) p_env->pTOC );
	return false;
      }
    
    /* Set leadout sector. 
       Note what OSX calls a LBA we call an LSN. So below re we 
       really have have MSF -> LSN -> LBA.
    */
    p_env->pp_lba[TOTAL_TRACKS] =
      cdio_lsn_to_lba(CDConvertMSFToLBA( pTrackDescriptors[i_leadout].p ));
  }

  p_env->gen.toc_init   = true;

  return( true ); 

}

/*!  
  Return the starting LSN track number
  i_track in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lsn_t
get_track_lba_osx(void *user_data, track_t i_track)
{
  _img_private_t *p_env = user_data;

  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = p_env->i_last_track+1;

  if (i_track > p_env->i_last_track + 1 || i_track < p_env->i_first_track) {
    return CDIO_INVALID_LSN;
  } else {
    return p_env->pp_lba[i_track - p_env->i_first_track];
  }
}

/*!
  Eject media . Return 1 if successful, 0 otherwise.

  The only way to cleanly unmount the disc under MacOS X is to use the
  'disktool' command line utility. It uses the non-public Disk
  Arbitration API, which can not be used by Cocoa or Carbon
  applications.

 */

static int 
_eject_media_osx (void *user_data) {

  _img_private_t *p_env = user_data;

  FILE *p_eject;
  char *psz_disk;
  char sz_cmd[32];

  if( ( psz_disk = (char *)strstr( p_env->gen.source_name, "disk" ) ) != NULL &&
      strlen( psz_disk ) > 4 )
    {
#define EJECT_CMD "/usr/sbin/hdiutil eject %s"
      snprintf( sz_cmd, sizeof(sz_cmd), EJECT_CMD, psz_disk );
#undef EJECT_CMD
      
      if( ( p_eject = popen( sz_cmd, "r" ) ) != NULL )
        {
	  char psz_result[0x200];
	  int i_ret = fread( psz_result, 1, sizeof(psz_result) - 1, p_eject );
	  
	  if( i_ret == 0 && ferror( p_eject ) != 0 )
            {
	      pclose( p_eject );
	      return 0;
            }
	  
	  pclose( p_eject );
	  
	  psz_result[ i_ret ] = 0;
	  
	  if( strstr( psz_result, "Disk Ejected" ) != NULL )
            {
	      return 1;
            }
        }
    }
  
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_stat_size_osx (void *user_data)
{
  return get_track_lba_osx(user_data, CDIO_CDROM_LEADOUT_TRACK);
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_osx (void *user_data, const char key[])
{
  _img_private_t *p_env = user_data;

  if (!strcmp (key, "source")) {
    return p_env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (p_env->access_mode) {
    case _AM_OSX:
      return "OS X";
    case _AM_NONE:
      return "no access method";
    }
  } 
  return NULL;
}

/*!
  Return the number of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_get_first_track_num_osx(void *user_data) 
{
  _img_private_t *p_env = user_data;
  
  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;

  return p_env->i_first_track;
}

/*!
  Return the media catalog number MCN.
 */
static char *
get_mcn_osx (const void *user_data) {
  const _img_private_t *p_env = user_data;
  dk_cd_read_mcn_t cd_read;

  memset( &cd_read, 0, sizeof(cd_read) );

  if( ioctl( p_env->gen.fd, DKIOCCDREADMCN, &cd_read ) < 0 )
  {
    cdio_debug( "could not read MCN, %s", strerror(errno) );
    return NULL;
  }
  return strdup((char*)cd_read.mcn);
}


/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
  This is the externally called interface.
*/
static track_t
_get_num_tracks_osx(void *user_data) 
{
  _img_private_t *p_env = user_data;
  
  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;

  return( TOTAL_TRACKS );
}

/*!  
  Get format of track. 
*/
static track_format_t
get_track_format_osx(void *user_data, track_t i_track) 
{
  _img_private_t *p_env = user_data;
  dk_cd_read_track_info_t cd_read;
  CDTrackInfo a_track;

  if (i_track > p_env->i_last_track || i_track < p_env->i_first_track)
    return TRACK_FORMAT_ERROR;
    
  memset( &cd_read, 0, sizeof(cd_read) );

  cd_read.address = i_track;
  cd_read.addressType = kCDTrackInfoAddressTypeTrackNumber;
  
  cd_read.buffer = &a_track;
  cd_read.bufferLength = sizeof(CDTrackInfo);
  
  if( ioctl( p_env->gen.fd, DKIOCCDREADTRACKINFO, &cd_read ) == -1 )
  {
    cdio_warn( "could not read trackinfo for track %d", i_track );
    return TRACK_FORMAT_ERROR;
  }

  cdio_debug( "%d: trackinfo trackMode: %x dataMode: %x", i_track, a_track.trackMode, a_track.dataMode );

  if (a_track.trackMode == CDIO_CDROM_DATA_TRACK) {
    if (a_track.dataMode == CDROM_CDI_TRACK) {
      return TRACK_FORMAT_CDI;
    } else if (a_track.dataMode == CDROM_XA_TRACK) {
      return TRACK_FORMAT_XA;
    } else {
      return TRACK_FORMAT_DATA;
    }
  } else {
    return TRACK_FORMAT_AUDIO;
  }

}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8

  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
get_track_green_osx(void *user_data, track_t i_track) 
{
  _img_private_t *p_env = user_data;
  CDTrackInfo a_track;

  if (!p_env->gen.toc_init) read_toc_osx (p_env) ;

  if ( i_track > p_env->i_last_track || i_track < p_env->i_first_track )
    return false;

  else {

    dk_cd_read_track_info_t cd_read;
    
    memset( &cd_read, 0, sizeof(cd_read) );
    
    cd_read.address      = i_track;
    cd_read.addressType  = kCDTrackInfoAddressTypeTrackNumber;
    
    cd_read.buffer       = &a_track;
    cd_read.bufferLength = sizeof(CDTrackInfo);
    
    if( ioctl( p_env->gen.fd, DKIOCCDREADTRACKINFO, &cd_read ) == -1 ) {
      cdio_warn( "could not read trackinfo for track %d", i_track );
      return false;
    }
    return ((a_track.trackMode & CDIO_CDROM_DATA_TRACK) != 0);
  }
}

#endif /* HAVE_DARWIN_CDROM */

/*!
  Return a string containing the default CD device if none is specified.
 */
char **
cdio_get_devices_osx(void)
{
#ifndef HAVE_DARWIN_CDROM
  return NULL;
#else
  io_object_t   next_media;
  mach_port_t   master_port;
  kern_return_t kern_result;
  io_iterator_t media_iterator;
  CFMutableDictionaryRef classes_to_match;
  char        **drives = NULL;
  unsigned int  num_drives=0;
  
  kern_result = IOMasterPort( MACH_PORT_NULL, &master_port );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  classes_to_match = IOServiceMatching( kIOCDMediaClass );
  if( classes_to_match == NULL )
    {
      return( NULL );
    }
  
  CFDictionarySetValue( classes_to_match, CFSTR(kIOMediaEjectableKey),
			kCFBooleanTrue );
  
  kern_result = IOServiceGetMatchingServices( master_port, 
					      classes_to_match,
					      &media_iterator );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  next_media = IOIteratorNext( media_iterator );
  if( next_media != 0 )
    {
      char psz_buf[0x32];
      size_t dev_path_length;
      CFTypeRef str_bsd_path;
      
      do
	{
	  str_bsd_path = IORegistryEntryCreateCFProperty( next_media,
							  CFSTR( kIOBSDNameKey ),
							  kCFAllocatorDefault,
							  0 );
	  if( str_bsd_path == NULL )
	    {
	      IOObjectRelease( next_media );
	      continue;
	    }
	  
	  snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
	  dev_path_length = strlen( psz_buf );
	  
	  if( CFStringGetCString( str_bsd_path,
				  (char*)&psz_buf + dev_path_length,
				  sizeof(psz_buf) - dev_path_length,
				  kCFStringEncodingASCII ) )
	    {
	      CFRelease( str_bsd_path );
	      IOObjectRelease( next_media );
	      IOObjectRelease( media_iterator );
	      cdio_add_device_list(&drives, psz_buf, &num_drives);
	    }
	  
	  CFRelease( str_bsd_path );
	  IOObjectRelease( next_media );
	  
	} while( ( next_media = IOIteratorNext( media_iterator ) ) != 0 );
    }
  IOObjectRelease( media_iterator );
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /* HAVE_DARWIN_CDROM */
}

/*!
  Return a string containing the default CD device if none is specified.
 */
char *
cdio_get_default_device_osx(void)
{
#ifndef HAVE_DARWIN_CDROM
  return NULL;
#else
  io_object_t   next_media;
  mach_port_t   master_port;
  kern_return_t kern_result;
  io_iterator_t media_iterator;
  CFMutableDictionaryRef classes_to_match;
  
  kern_result = IOMasterPort( MACH_PORT_NULL, &master_port );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  classes_to_match = IOServiceMatching( kIOCDMediaClass );
  if( classes_to_match == NULL )
    {
      return( NULL );
    }
  
  CFDictionarySetValue( classes_to_match, CFSTR(kIOMediaEjectableKey),
			kCFBooleanTrue );
  
  kern_result = IOServiceGetMatchingServices( master_port, 
					      classes_to_match,
					      &media_iterator );
  if( kern_result != KERN_SUCCESS )
    {
      return( NULL );
    }
  
  next_media = IOIteratorNext( media_iterator );
  if( next_media != 0 )
    {
      char psz_buf[0x32];
      size_t dev_path_length;
      CFTypeRef str_bsd_path;
      
      do
	{
	  str_bsd_path = IORegistryEntryCreateCFProperty( next_media,
							  CFSTR( kIOBSDNameKey ),
							  kCFAllocatorDefault,
							  0 );
	  if( str_bsd_path == NULL )
	    {
	      IOObjectRelease( next_media );
	      continue;
	    }
	  
	  snprintf( psz_buf, sizeof(psz_buf), "%s%c", _PATH_DEV, 'r' );
	  dev_path_length = strlen( psz_buf );
	  
	  if( CFStringGetCString( str_bsd_path,
				  (char*)&psz_buf + dev_path_length,
				  sizeof(psz_buf) - dev_path_length,
				  kCFStringEncodingASCII ) )
	    {
	      CFRelease( str_bsd_path );
	      IOObjectRelease( next_media );
	      IOObjectRelease( media_iterator );
	      return strdup( psz_buf );
	    }
	  
	  CFRelease( str_bsd_path );
	  IOObjectRelease( next_media );
	  
	} while( ( next_media = IOIteratorNext( media_iterator ) ) != 0 );
    }
  IOObjectRelease( media_iterator );
  return NULL;
#endif /* HAVE_DARWIN_CDROM */
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_osx (const char *psz_source_name, const char *psz_access_mode)
{

  if (psz_access_mode != NULL)
    cdio_warn ("there is only one access mode for OS X. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_osx(psz_source_name);
}


/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_osx (const char *psz_orig_source)
{

#ifdef HAVE_DARWIN_CDROM
  CdIo *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs _funcs = {
    .eject_media        = _eject_media_osx,
    .free               = _free_osx,
    .get_arg            = _get_arg_osx,
    .get_cdtext         = get_cdtext_generic,
    .get_default_device = cdio_get_default_device_osx,
    .get_devices        = cdio_get_devices_osx,
    .get_discmode       = get_discmode_generic,
#if SCSI_MMC_FIXED
    .get_cdtext         = get_cdtext_generic,
    .get_drive_cap      = scsi_mmc_get_drive_cap_generic,
#else
    .get_cdtext         = get_cdtext_osx,
    .get_drive_cap      = get_drive_cap_osx,
#endif
    .get_first_track_num= _get_first_track_num_osx,
    .get_mcn            = get_mcn_osx,
    .get_num_tracks     = _get_num_tracks_osx,
    .get_track_format   = get_track_format_osx,
    .get_track_green    = get_track_green_osx,
    .get_track_lba      = get_track_lba_osx,
    .get_track_msf      = NULL,
    .lseek              = cdio_generic_lseek,
    .read               = cdio_generic_read,
    .read_audio_sectors = _get_read_audio_sectors_osx,
    .read_mode1_sector  = _get_read_mode1_sector_osx,
    .read_mode1_sectors = _get_read_mode1_sectors_osx,
    .read_mode2_sector  = _get_read_mode2_sector_osx,
    .read_mode2_sectors = _get_read_mode2_sectors_osx,
    .read_toc           =  read_toc_osx,
    .run_scsi_mmc_cmd   =  run_scsi_cmd_osx,
    .set_arg            = _set_arg_osx,
    .stat_size          = _stat_size_osx
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->access_mode    = _AM_OSX;
  _data->service        = 0;
  _data->gen.init       = false;
  _data->gen.fd         = -1;
  _data->gen.toc_init   = false;
  _data->gen.b_cdtext_init  = false;
  _data->gen.b_cdtext_error = false;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_osx();
    if (NULL == psz_source) return NULL;
    _set_arg_osx(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_generic(psz_orig_source))
      _set_arg_osx(_data, "source", psz_orig_source);
    else {
      /* The below would be okay if all device drivers worked this way. */
#if 0
      cdio_info ("source %s is a not a device", psz_orig_source);
#endif
      return NULL;
    }
  }

  ret = cdio_new ((void *)_data, &_funcs);
  if (ret == NULL) return NULL;

  if (cdio_generic_init(_data) && init_osx(_data))
    return ret;
  else {
    cdio_generic_free (_data);
    return NULL;
  }
  
#else 
  return NULL;
#endif /* HAVE_DARWIN_CDROM */

}

bool
cdio_have_osx (void)
{
#ifdef HAVE_DARWIN_CDROM
  return true;
#else 
  return false;
#endif /* HAVE_DARWIN_CDROM */
}
