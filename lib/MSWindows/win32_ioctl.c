/*
    $Id: win32_ioctl.c,v 1.34 2004/08/05 02:58:46 rocky Exp $

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

static const char _rcsid[] = "$Id: win32_ioctl.c,v 1.34 2004/08/05 02:58:46 rocky Exp $";

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
#include <errno.h>
#include <sys/types.h>
#include <cdio/scsi_mmc.h>
#include "cdtext_private.h"

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

#define OP_TIMEOUT_MS 60 

/*!
  Run a SCSI MMC command. 
 
  env	        private CD structure 
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  Return 0 if command completed successfully.
 */
int
run_scsi_cmd_win32ioctl( const void *p_user_data, 
			 unsigned int i_timeout_ms,
			 unsigned int i_cdb, const scsi_mmc_cdb_t * p_cdb,
			 scsi_mmc_direction_t e_direction, 
			 unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;
  SCSI_PASS_THROUGH_DIRECT sptd;
  bool success;
  DWORD dwBytesReturned;
  
  sptd.Length  = sizeof(sptd);
  sptd.PathId  = 0;      /* SCSI card ID will be filled in automatically */
  sptd.TargetId= 0;      /* SCSI target ID will also be filled in */
  sptd.Lun=0;            /* SCSI lun ID will also be filled in */
  sptd.CdbLength         = i_cdb;
  sptd.SenseInfoLength   = 0; /* Don't return any sense data */
  sptd.DataIn            = SCSI_MMC_DATA_READ == e_direction ? 
    SCSI_IOCTL_DATA_IN : SCSI_IOCTL_DATA_OUT; 
  sptd.DataTransferLength= i_buf; 
  sptd.TimeOutValue      = msecs2secs(i_timeout_ms);
  sptd.DataBuffer        = (void *) p_buf;
  sptd.SenseInfoOffset   = 0;

  memcpy(sptd.Cdb, p_cdb, i_cdb);

  /* Send the command to drive */
  success=DeviceIoControl(p_env->h_device_handle,
			  IOCTL_SCSI_PASS_THROUGH_DIRECT,               
			  (void *)&sptd, 
			  (DWORD)sizeof(SCSI_PASS_THROUGH_DIRECT),
			  NULL, 0,                        
			  &dwBytesReturned,
			  NULL);

  if(! success) {
    char *psz_msg = NULL;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPSTR) psz_msg, 0, NULL);
    cdio_info("Error: %s", psz_msg);
    LocalFree(psz_msg);
    return 1;
  }
  return 0;
}

/*! 
  Get disc type associated with cd object.
*/
discmode_t
get_discmode_win32ioctl (_img_private_t *p_env)
{
  track_t i_track;
  discmode_t discmode=CDIO_DISC_MODE_NO_INFO;

  /* See if this is a DVD. */
  cdio_dvd_struct_t dvd;  /* DVD READ STRUCT for layer 0. */

  dvd.physical.type = CDIO_DVD_STRUCT_PHYSICAL;
  dvd.physical.layer_num = 0;
  if (0 == scsi_mmc_get_dvd_struct_physical_private (p_env, 
						     &run_scsi_cmd_win32ioctl,
						     &dvd)) {
    switch(dvd.physical.layer[0].book_type) {
    case CDIO_DVD_BOOK_DVD_ROM:  return CDIO_DISC_MODE_DVD_ROM;
    case CDIO_DVD_BOOK_DVD_RAM:  return CDIO_DISC_MODE_DVD_RAM;
    case CDIO_DVD_BOOK_DVD_R:    return CDIO_DISC_MODE_DVD_R;
    case CDIO_DVD_BOOK_DVD_RW:   return CDIO_DISC_MODE_DVD_RW;
    case CDIO_DVD_BOOK_DVD_PR:   return CDIO_DISC_MODE_DVD_PR;
    case CDIO_DVD_BOOK_DVD_PRW:  return CDIO_DISC_MODE_DVD_PRW;
    default: return CDIO_DISC_MODE_DVD_OTHER;
    }
  }

  if (!p_env->gen.toc_init) 
    read_toc_win32ioctl (p_env);

  if (!p_env->gen.toc_init) 
    return CDIO_DISC_MODE_NO_INFO;

  for (i_track = p_env->gen.i_first_track; 
       i_track < p_env->gen.i_first_track + p_env->gen.i_tracks ; 
       i_track ++) {
    track_format_t track_fmt=get_track_format_win32ioctl(p_env, i_track);

    switch(track_fmt) {
    case TRACK_FORMAT_AUDIO:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_DA;
	  break;
	case CDIO_DISC_MODE_CD_DA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	  discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_XA:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_XA;
	  break;
	case CDIO_DISC_MODE_CD_XA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_DATA:
      switch(discmode) {
	case CDIO_DISC_MODE_NO_INFO:
	  discmode = CDIO_DISC_MODE_CD_DATA;
	  break;
	case CDIO_DISC_MODE_CD_DATA:
	case CDIO_DISC_MODE_CD_MIXED: 
	case CDIO_DISC_MODE_ERROR: 
	  /* No change*/
	  break;
      default:
	discmode = CDIO_DISC_MODE_CD_MIXED;
      }
      break;
    case TRACK_FORMAT_ERROR:
    default:
      discmode = CDIO_DISC_MODE_ERROR;
    }
  }
  return discmode;
}

/*
  Returns a string that can be used in a CreateFile call if 
  c_drive letter is a character. If not NULL is returned.
 */

const char *
is_cdrom_win32ioctl(const char c_drive_letter) 
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
read_audio_sectors_win32ioctl (_img_private_t *env, void *data, lsn_t lsn, 
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
    char *psz_msg = NULL;
    DWORD dw = GetLastError();

    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 
		  NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		  (LPSTR) psz_msg, 0, NULL);

    cdio_info("Error reading audio-mode %lu\n%s)", 
	      (long unsigned int) lsn, psz_msg);
    LocalFree(psz_msg);
    return 1;
  }
  return 0;
}

/*!
   Reads a single raw sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
static int
read_raw_sector (const _img_private_t *p_env, void *p_buf, lsn_t lsn) 
{
  scsi_mmc_cdb_t cdb = {{0, }};

  /* ReadCD CDB12 command.  The values were taken from MMC1 draft paper. */
  CDIO_MMC_SET_COMMAND      (cdb.field, CDIO_MMC_GPCMD_READ_CD);
  CDIO_MMC_SET_READ_LBA     (cdb.field, lsn);
  CDIO_MMC_SET_READ_LENGTH24(cdb.field, 1);

  cdb.field[9]=0xF8;  /* Raw read, 2352 bytes per sector */
  
  return run_scsi_cmd_win32ioctl(p_env, OP_TIMEOUT_MS,
				     scsi_mmc_get_cmd_len(cdb.field[0]), 
				     &cdb, SCSI_MMC_DATA_READ, 
				     CDIO_CD_FRAMESIZE_RAW, p_buf);
}

/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
read_mode2_sector_win32ioctl (const _img_private_t *p_env, void *p_data, 
			      lsn_t lsn, bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int ret = read_raw_sector (p_env, buf, lsn);

  if ( 0 != ret) return ret;

  memcpy (p_data,
	  buf + CDIO_CD_SYNC_SIZE + CDIO_CD_XA_HEADER,
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
  return 0;

}

/*!
   Reads a single mode2 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int
read_mode1_sector_win32ioctl (const _img_private_t *env, void *data, 
			      lsn_t lsn, bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int ret = read_raw_sector (env, buf, lsn);

  if ( 0 != ret) return ret;

  memcpy (data, 
	  buf + CDIO_CD_SYNC_SIZE+CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);
  
  return 0;

}

/*!
  Initialize internal structures for CD device.
 */
bool
init_win32ioctl (_img_private_t *env)
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

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool
read_toc_win32ioctl (_img_private_t *env) 
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
  
  env->gen.i_first_track = cdrom_toc.FirstTrack;
  env->gen.i_tracks  = cdrom_toc.LastTrack - cdrom_toc.FirstTrack + 1;
  
  
  for( i = 0 ; i <= env->gen.i_tracks ; i++ ) {
    env->tocent[ i ].start_lsn = cdio_msf3_to_lba(
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
  Get cdtext information for a CdIo object .
  
  @param obj the CD object that may contain CD-TEXT information.
  @return the CD-TEXT object or NULL if obj is NULL
  or CD-TEXT information does not exist.
*/
bool
init_cdtext_win32ioctl (_img_private_t *p_env)
{
  return scsi_mmc_init_cdtext_private( p_env,
				       &run_scsi_cmd_win32ioctl, 
				       set_cdtext_field_win32
				       );
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
char *
get_mcn_win32ioctl (const _img_private_t *env) {

  DWORD dwBytesReturned;
  SUB_Q_MEDIA_CATALOG_NUMBER mcn;
  CDROM_SUB_Q_DATA_FORMAT q_data_format;
  
  memset( &mcn, 0, sizeof(mcn) );
  
  q_data_format.Format = CDIO_SUBCHANNEL_MEDIA_CATALOG;

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
get_track_format_win32ioctl(const _img_private_t *env, track_t i_track) 
{
  /* This is pretty much copied from the "badly broken" cdrom_count_tracks
     in linux/cdrom.c.
  */
  if (env->tocent[i_track - env->gen.i_first_track].Control & 0x04) {
    if (env->tocent[i_track - env->gen.i_first_track].Format == 0x10)
      return TRACK_FORMAT_CDI;
    else if (env->tocent[i_track - env->gen.i_first_track].Format == 0x20) 
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
void
get_drive_cap_win32ioctl (const _img_private_t *p_env,
			  cdio_drive_read_cap_t  *p_read_cap,
			  cdio_drive_write_cap_t *p_write_cap,
			  cdio_drive_misc_cap_t  *p_misc_cap)
{
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

  CDIO_MMC_SET_COMMAND(sptwb.Spt.Cdb, CDIO_MMC_GPCMD_MODE_SENSE);
  /*sptwb.Spt.Cdb[1]           = 0x08;  /+ doesn't return block descriptors */
  sptwb.Spt.Cdb[1]             = 0x0;
  sptwb.Spt.Cdb[2]             = CDIO_MMC_CAPABILITIES_PAGE;
  sptwb.Spt.Cdb[3]             = 0;     /* Not used */
  sptwb.Spt.Cdb[4]             = 128;
  sptwb.Spt.Cdb[5]             = 0;     /* Not used */
  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,DataBuf) +
    sptwb.Spt.DataTransferLength;
  
  if ( DeviceIoControl(p_env->h_device_handle,
		       IOCTL_SCSI_PASS_THROUGH,
		       &sptwb,
		       sizeof(SCSI_PASS_THROUGH),
		       &sptwb,
		       length,
		       &returned,
		       FALSE) )     {
    unsigned int n=sptwb.DataBuf[3]+4;
    /* Reader? */
    scsi_mmc_get_drive_cap_buf(&(sptwb.DataBuf[n]), p_read_cap, 
			       p_write_cap, p_misc_cap);
  } else {
    *p_read_cap  = CDIO_DRIVE_CAP_UNKNOWN;
    *p_write_cap = CDIO_DRIVE_CAP_UNKNOWN;
    *p_misc_cap  = CDIO_DRIVE_CAP_UNKNOWN;
  }
  return;
}

#endif /*HAVE_WIN32_CDROM*/
