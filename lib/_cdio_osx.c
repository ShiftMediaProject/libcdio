/*
    $Id: _cdio_osx.c,v 1.37 2004/06/17 01:16:50 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com> 
    from vcdimager code: 
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    and VideoLAN code Copyright (C) 1998-2001 VideoLAN
      Authors: Johan Bilien <jobi@via.ecp.fr>
               Gildas Bazin <gbazin@netcourrier.com>
               Jon Lech Johansen <jon-vl@nanocrew.net>
               Derk-Jan Hartman <hartman at videolan.org>

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

static const char _rcsid[] = "$Id: _cdio_osx.c,v 1.37 2004/06/17 01:16:50 rocky Exp $";

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"

#include <string.h>

#ifdef HAVE_DARWIN_CDROM

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <paths.h>
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFNumber.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOBSD.h>
#include <IOKit/storage/IOCDTypes.h>
#include <IOKit/storage/IODVDTypes.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IOCDMedia.h>
#include <IOKit/storage/IOCDMediaBSDClient.h>
#include <IOKit/storage/IODVDMediaBSDClient.h>

/* Note leadout should be 0xAA, But OSX seems to use 0xA2. */
#define	OSX_CDROM_LEADOUT_TRACK 0xA2

#define TOTAL_TRACKS    (env->i_last_track - env->i_first_track)

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
  bool toc_init;             /* if true, info below is valid. */
  CDTOC *pTOC;
  int i_descriptors;
  track_t i_last_track;      /* highest track number */
  track_t i_first_track;     /* first track */
  lsn_t   *pp_lba;

} _img_private_t;

static void 
_free_osx (void *user_data) {
  _img_private_t *env = user_data;
  if (NULL == env) return;
  cdio_generic_free(env);
  if (NULL != env->pp_lba) free((void *) env->pp_lba);
  if (NULL != env->pTOC) free((void *) env->pTOC);
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
    cdio_error( "could not read block %d, %s", lsn, strerror(errno) );
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
    cdio_error( "could not read block %d, %s", lsn, strerror(errno) );
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
  
  cd_read.offset = lsn * kCDSectorSizeCDDA;
  cd_read.sectorArea = kCDSectorAreaUser;
  cd_read.sectorType = kCDSectorTypeCDDA;
  
  cd_read.buffer = data;
  cd_read.bufferLength = kCDSectorSizeCDDA * nblocks;
  
  if( ioctl( env->gen.fd, DKIOCCDREAD, &cd_read ) == -1 )
  {
    cdio_error( "could not read block %d", lsn );
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
	cdio_error ("unknown access type: %s. ignored.", value);
    }
  else 
    return -1;

  return 0;
}

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return false if successful or true if an error.
*/
static bool
_cdio_read_toc (_img_private_t *env) 
{
  mach_port_t port;
  char *psz_devname;
  kern_return_t ret;
  io_iterator_t iterator;
  io_registry_entry_t service;
  CFMutableDictionaryRef properties;
  CFDataRef data;
  
  env->gen.fd = open( env->gen.source_name, O_RDONLY | O_NONBLOCK );
  if (-1 == env->gen.fd) {
    cdio_error("Failed to open %s: %s", env->gen.source_name,
	       strerror(errno));
    return false;
  }

  /* get the device name */
  if( ( psz_devname = strrchr( env->gen.source_name, '/') ) != NULL )
    ++psz_devname;
  else
    psz_devname = env->gen.source_name;
  
  /* unraw the device name */
  if( *psz_devname == 'r' )
    ++psz_devname;
  
  /* get port for IOKit communication */
  if( ( ret = IOMasterPort( MACH_PORT_NULL, &port ) ) != KERN_SUCCESS )
    {
      cdio_error( "IOMasterPort: 0x%08x", ret );
      return false;
    }
  
  /* get service iterator for the device */
  if( ( ret = IOServiceGetMatchingServices( 
					   port, IOBSDNameMatching( port, 0, psz_devname ),
					   &iterator ) ) != KERN_SUCCESS )
    {
        cdio_error( "IOServiceGetMatchingServices: 0x%08x", ret );
        return false;
    }
  
  /* first service */
  service = IOIteratorNext( iterator );
  IOObjectRelease( iterator );
  
  /* search for kIOCDMediaClass */ 
  while( service && !IOObjectConformsTo( service, kIOCDMediaClass ) )
    {

      ret = IORegistryEntryGetParentIterator( service, kIOServicePlane, 
					      &iterator );
      if( ret != KERN_SUCCESS )
        {
	  cdio_error( "IORegistryEntryGetParentIterator: 0x%08x", ret );
	  IOObjectRelease( service );
	  return false;
        }
      
      IOObjectRelease( service );
      service = IOIteratorNext( iterator );
      IOObjectRelease( iterator );
    }
  
  if( service == 0 )
    {
      cdio_error( "search for kIOCDMediaClass came up empty" );
      return false;
    }
  
  /* create a CF dictionary containing the TOC */
  ret = IORegistryEntryCreateCFProperties( service, &properties,
					   kCFAllocatorDefault, kNilOptions );
  
  if(  ret != KERN_SUCCESS )
    {
      cdio_error( "IORegistryEntryCreateCFProperties: 0x%08x", ret );
      IOObjectRelease( service );
      return false;
    }
  
  /* get the TOC from the dictionary */
  data = (CFDataRef) CFDictionaryGetValue( properties,
					   CFSTR(kIOCDMediaTOCKey) );
  if( data  != NULL )
    {
      CFRange range;
      CFIndex buf_len;
      
      buf_len = CFDataGetLength( data ) + 1;
      range = CFRangeMake( 0, buf_len );
      
      if( ( env->pTOC = (CDTOC *)malloc( buf_len ) ) != NULL ) {
	CFDataGetBytes( data, range, (u_char *) env->pTOC );
      } else {
	cdio_error( "Trouble allocating CDROM TOC" );
	return false;
      }
    }
  else
    {
      cdio_error( "CFDictionaryGetValue failed" );
    }
  
  CFRelease( properties );
  IOObjectRelease( service ); 

  env->i_descriptors = CDTOCGetDescriptorCount ( env->pTOC );

  /* Read in starting sectors. There may be non-tracks mixed in with
     the real tracks.  So find the first and last track number by
     scanning. Also find the lead-out track position.
   */
  {
    int i, i_leadout = -1;
    
    CDTOCDescriptor *pTrackDescriptors;
    track_t i_track;
    
    env->pp_lba = malloc( TOTAL_TRACKS * sizeof(int) );
    if( env->pp_lba == NULL )
      {
	cdio_error("Out of memory in allocating track starting LSNs" );
	free( env->pTOC );
	return false;
      }
    
    pTrackDescriptors = env->pTOC->descriptors;

    env->i_first_track = CDIO_CD_MAX_TRACKS+1;
    env->i_last_track  = CDIO_CD_MIN_TRACK_NO-1;
    
    for( i = 0; i <= env->i_descriptors; i++ )
      {
	i_track = pTrackDescriptors[i].point;

	if( i_track == OSX_CDROM_LEADOUT_TRACK )
	  /* Note leadout should be 0xAA, But OSX seems to use 0xA2. */
	  i_leadout = i;

	if( i_track > CDIO_CD_MAX_TRACKS || i_track < CDIO_CD_MIN_TRACK_NO )
	  continue;
	
	if (env->i_first_track > i_track) 
	  env->i_first_track = i_track;
	
	if (env->i_last_track < i_track) 
	  env->i_last_track = i_track;
	
      }

    /* Now that we know what the first track number is, we can make sure
       index positions are ordered starting at 0.
     */
    for( i = 0; i <= env->i_descriptors; i++ )
      {
	i_track = pTrackDescriptors[i].point;

	if( i_track > CDIO_CD_MAX_TRACKS || i_track < CDIO_CD_MIN_TRACK_NO )
	  continue;
	
	env->pp_lba[i_track - env->i_first_track] =
	  cdio_lsn_to_lba(CDConvertMSFToLBA( pTrackDescriptors[i].p ));
      }
    
    if( i_leadout == -1 )
      {
	cdio_error( "CD leadout not found" );
	free( env->pp_lba );
	free( (void *) env->pTOC );
	return false;
      }
    
    /* set leadout sector */
    env->pp_lba[i_leadout] =
      cdio_lsn_to_lba(CDConvertMSFToLBA( pTrackDescriptors[i_leadout].p ));
  }

  env->toc_init   = true;

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
_get_track_lba_osx(void *user_data, track_t i_track)
{
  _img_private_t *env = user_data;

  if (!env->toc_init) _cdio_read_toc (env) ;

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = env->i_last_track+1;

  if (i_track > env->i_last_track + 1 || i_track < env->i_first_track) {
    return CDIO_INVALID_LSN;
  } else {
    return env->pp_lba[i_track - env->i_first_track];
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

  _img_private_t *env = user_data;

  FILE *p_eject;
  char *psz_disk;
  char sz_cmd[32];

  if( ( psz_disk = (char *)strstr( env->gen.source_name, "disk" ) ) != NULL &&
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
  return _get_track_lba_osx(user_data, CDIO_CDROM_LEADOUT_TRACK);
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_osx (void *user_data, const char key[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source")) {
    return env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    switch (env->access_mode) {
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
  _img_private_t *env = user_data;
  
  if (!env->toc_init) _cdio_read_toc (env) ;

  return env->i_first_track;
}

/*!
  Return the media catalog number MCN.
 */
static char *
_get_mcn_osx (const void *user_data) {
  const _img_private_t *env = user_data;
  dk_cd_read_mcn_t cd_read;

  memset( &cd_read, 0, sizeof(cd_read) );

  if( ioctl( env->gen.fd, DKIOCCDREADMCN, &cd_read ) < 0 )
  {
    cdio_warn( "could not read MCN, %s", strerror(errno) );
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
  _img_private_t *env = user_data;
  
  if (!env->toc_init) _cdio_read_toc (env) ;

  return( TOTAL_TRACKS );
}

/*!  
  Get format of track. 
*/
static track_format_t
_get_track_format_osx(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  dk_cd_read_track_info_t cd_read;
  CDTrackInfo a_track;

  if (i_track > env->i_last_track || i_track < env->i_first_track)
    return TRACK_FORMAT_ERROR;
    
  memset( &cd_read, 0, sizeof(cd_read) );

  cd_read.address = i_track;
  cd_read.addressType = kCDTrackInfoAddressTypeTrackNumber;
  
  cd_read.buffer = &a_track;
  cd_read.bufferLength = sizeof(CDTrackInfo);
  
  if( ioctl( env->gen.fd, DKIOCCDREADTRACKINFO, &cd_read ) == -1 )
  {
    cdio_error( "could not read trackinfo for track %d", i_track );
    return TRACK_FORMAT_ERROR;
  }

  /*cdio_warn( "trackinfo trackMode: %x dataMode: %x", a_track.trackMode, a_track.dataMode );*/

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
_get_track_green_osx(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  CDTrackInfo a_track;

  if (!env->toc_init) _cdio_read_toc (env) ;

  if ( i_track > env->i_last_track || i_track < env->i_first_track )
    return false;

  else {

    dk_cd_read_track_info_t cd_read;
    
    memset( &cd_read, 0, sizeof(cd_read) );
    
    cd_read.address      = i_track;
    cd_read.addressType  = kCDTrackInfoAddressTypeTrackNumber;
    
    cd_read.buffer       = &a_track;
    cd_read.bufferLength = sizeof(CDTrackInfo);
    
    if( ioctl( env->gen.fd, DKIOCCDREADTRACKINFO, &cd_read ) == -1 ) {
      cdio_error( "could not read trackinfo for track %d", i_track );
      return -1;
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
    .get_default_device = cdio_get_default_device_osx,
    .get_devices        = cdio_get_devices_osx,
    .get_drive_cap      = NULL,
    .get_first_track_num= _get_first_track_num_osx,
    .get_mcn            = _get_mcn_osx,
    .get_num_tracks     = _get_num_tracks_osx,
    .get_track_format   = _get_track_format_osx,
    .get_track_green    = _get_track_green_osx,
    .get_track_lba      = _get_track_lba_osx,
    .get_track_msf      = NULL,
    .lseek              = cdio_generic_lseek,
    .read               = cdio_generic_read,
    .read_audio_sectors = _get_read_audio_sectors_osx,
    .read_mode1_sector  = _get_read_mode1_sector_osx,
    .read_mode1_sectors = _get_read_mode1_sectors_osx,
    .read_mode2_sector  = _get_read_mode2_sector_osx,
    .read_mode2_sectors = _get_read_mode2_sectors_osx,
    .set_arg            = _set_arg_osx,
    .stat_size          = _stat_size_osx
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->access_mode    = _AM_OSX;
  _data->gen.init       = false;
  _data->gen.fd         = -1;

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

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (cdio_generic_init(_data))
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
