/*
    $Id: win32.c,v 1.30 2004/07/25 23:32:15 rocky Exp $

    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

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
   control of the CD drive. Inspired by vlc's cdrom.h code 
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: win32.c,v 1.30 2004/07/25 23:32:15 rocky Exp $";

#include <cdio/cdio.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include <cdio/scsi_mmc.h>
#include "cdio_assert.h"
#include "cdio_private.h" /* protoype for cdio_is_device_win32 */

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
#include "win32.h"

#include <sys/stat.h>
#include <sys/types.h>
#include "aspi32.h"

#define WIN_NT               ( GetVersion() < 0x80000000 )

/* General ioctl() CD-ROM command function */
static bool 
_cdio_mciSendCommand(int id, UINT msg, DWORD flags, void *arg)
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

static access_mode_t 
str_to_access_mode_win32(const char *psz_access_mode) 
{
  const access_mode_t default_access_mode = 
    WIN_NT ? _AM_IOCTL : _AM_ASPI;

  if (NULL==psz_access_mode) return default_access_mode;
  
  if (!strcmp(psz_access_mode, "ioctl"))
    return _AM_IOCTL;
  else if (!strcmp(psz_access_mode, "ASPI"))
    return _AM_ASPI;
  else {
    cdio_warn ("unknown access type: %s. Default used.", 
	       psz_access_mode);
    return default_access_mode;
  }
}

static discmode_t
get_discmode_win32(void *p_user_data) 
{
  _img_private_t *p_env = p_user_data;

  if (p_env->hASPI) {
    return get_discmode_aspi (p_env);
  } else {
    return get_discmode_win32ioctl (p_env);
  }
}

static const char *
is_cdrom_win32(const char drive_letter) {
  if ( WIN_NT ) {
    return is_cdrom_win32ioctl (drive_letter);
  } else {
    return is_cdrom_aspi(drive_letter);
  }
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static void
_cdio_get_drive_cap (const void *env,
		     cdio_drive_read_cap_t  *p_read_cap,
		     cdio_drive_write_cap_t *p_write_cap,
		     cdio_drive_misc_cap_t  *p_misc_cap)
{
  const _img_private_t *_obj = env;

  if (_obj->hASPI) {
    /* A safe guess */ 

    get_drive_cap_aspi (env, p_read_cap, p_write_cap, p_misc_cap);
  } else {
    get_drive_cap_win32ioctl (env, p_read_cap, p_write_cap, p_misc_cap);
  }
}


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
static int
scsi_mmc_run_cmd_win32( const void *p_user_data, int i_timeout,
			unsigned int i_cdb, const scsi_mmc_cdb_t *p_cdb, 
			scsi_mmc_direction_t e_direction, 
			unsigned int i_buf, /*in/out*/ void *p_buf )
{
  const _img_private_t *p_env = p_user_data;

  if (p_env->hASPI) {
    return scsi_mmc_run_cmd_aspi( p_env, i_timeout, i_cdb, p_cdb,  
				  e_direction, i_buf, p_buf );
  } else {
    return scsi_mmc_run_cmd_win32ioctl( p_env, i_timeout, i_cdb, p_cdb,  
					e_direction, i_buf, p_buf );
  }
}

/*!
  Initialize CD device.
 */
static bool
_cdio_init_win32 (void *user_data)
{
  _img_private_t *env = user_data;
  if (env->gen.init) {
    cdio_error ("init called more than once");
    return false;
  }
  
  env->gen.init = true;
  env->toc_init = false;

  /* Initializations */
  env->h_device_handle = NULL;
  env->i_sid           = 0;
  env->hASPI           = 0;
  env->lpSendCommand   = 0;
  env->b_aspi_init     = false;
  env->b_ioctl_init    = false;
  env->b_cdtext_init   = false;

  if ( _AM_IOCTL == env->access_mode ) {
    return init_win32ioctl(env);
  } else {
    return init_aspi(env);
  }
}

/*!
  Release and free resources associated with cd. 
 */
static void
_free_win32 (void *user_data)
{
  _img_private_t *env = user_data;

  if (NULL == env) return;
  free (env->gen.source_name);

  if( env->h_device_handle )
    CloseHandle( env->h_device_handle );
  if( env->hASPI )
    FreeLibrary( (HMODULE)env->hASPI );

  free (env);
}

/*!
   Reads an audio device into data starting from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_audio_sectors (void *user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks) 
{
  _img_private_t *env = user_data;
  if ( env->hASPI ) {
    return read_audio_sectors_aspi( env, data, lsn, nblocks );
  } else {
    return read_audio_sectors_win32ioctl( env, data, lsn, nblocks );
  }
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sector (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *env = user_data;

  if (env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");

  if (env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (env->gen.ioctls_debugged < 75 
      || (env->gen.ioctls_debugged < (30 * 75)  
	  && env->gen.ioctls_debugged % 75 == 0)
      || env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %lu", (unsigned long int) lsn);
  
  env->gen.ioctls_debugged++;

  if ( env->hASPI ) {
    return read_mode1_sector_aspi( env, data, lsn, b_form2 );
  } else {
    return read_mode1_sector_win32ioctl( env, data, lsn, b_form2 );
  }
}

/*!
   Reads nblocks of mode1 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode1_sectors (void *user_data, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int i;
  int retval;

  for (i = 0; i < nblocks; i++) {
    if (b_form2) {
      if ( (retval = _cdio_read_mode1_sector (env, 
					  ((char *)data) + (M2RAW_SECTOR_SIZE * i),
					  lsn + i, true)) )
	return retval;
    } else {
      char buf[M2RAW_SECTOR_SIZE] = { 0, };
      if ( (retval = _cdio_read_mode1_sector (env, buf, lsn + i, false)) )
	return retval;
      
      memcpy (((char *)data) + (CDIO_CD_FRAMESIZE * i), 
	      buf, CDIO_CD_FRAMESIZE);
    }
  }
  return 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sector (void *user_data, void *data, lsn_t lsn, 
		    bool b_form2)
{
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  _img_private_t *env = user_data;

  if (env->gen.ioctls_debugged == 75)
    cdio_debug ("only displaying every 75th ioctl from now on");

  if (env->gen.ioctls_debugged == 30 * 75)
    cdio_debug ("only displaying every 30*75th ioctl from now on");
  
  if (env->gen.ioctls_debugged < 75 
      || (env->gen.ioctls_debugged < (30 * 75)  
	  && env->gen.ioctls_debugged % 75 == 0)
      || env->gen.ioctls_debugged % (30 * 75) == 0)
    cdio_debug ("reading %lu", (unsigned long int) lsn);
  
  env->gen.ioctls_debugged++;

  if ( env->hASPI ) {
    int ret;
    ret = read_mode2_sector_aspi(user_data, buf, lsn, 1);
    if( ret != 0 ) return ret;
    if (b_form2)
      memcpy (data, buf, M2RAW_SECTOR_SIZE);
    else
      memcpy (((char *)data), buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    return 0;
  } else {
    return read_mode2_sector_win32ioctl( env, data, lsn, b_form2 );
  }
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sectors (void *user_data, void *data, lsn_t lsn, 
			  bool b_form2, unsigned int nblocks)
{
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _cdio_read_mode2_sector (user_data, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_cdio_stat_size (void *user_data)
{
  _img_private_t *env = user_data;

  return env->tocent[env->i_tracks].start_lsn;
}

/*!
  Set the key "arg" to "value" in source device.
*/
static int
_set_arg_win32 (void *user_data, const char key[], const char value[])
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
      env->access_mode = str_to_access_mode_win32(value);
      if (env->access_mode == _AM_ASPI && !env->b_aspi_init) 
	return init_aspi(env) ? 1 : -3;
      else if (env->access_mode == _AM_IOCTL && !env->b_ioctl_init) 
	return init_win32ioctl(env) ? 1 : -3;
      else
	return -4;
      return 0;
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
_cdio_read_toc (_img_private_t *p_env) 
{
  bool ret;
  if( p_env->hASPI ) {
    ret = read_toc_aspi( p_env );
  } else {
    ret = read_toc_win32ioctl( p_env );
  }
  if (ret) p_env->gen.toc_init = true ;
  return true;
}

/*!
  Eject media. Return 1 if successful, 0 otherwise.
 */
static int 
_cdio_eject_media (void *user_data) {

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
  
  if( _cdio_mciSendCommand( 0, MCI_OPEN, i_flags, &op ) ) {
    st.dwItem = MCI_STATUS_READY;
    /* Eject disc */
    ret = _cdio_mciSendCommand( op.wDeviceID, MCI_SET, MCI_SET_DOOR_OPEN, 0 ) != 0;
    /* Release access to the device */
    _cdio_mciSendCommand( op.wDeviceID, MCI_CLOSE, MCI_WAIT, 0 );
  } else 
    ret = 0;
  
  return ret;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_win32 (void *user_data, const char key[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source")) {
    return env->gen.source_name;
  } else if (!strcmp (key, "access-mode")) {
    if (env->hASPI) 
      return "ASPI";
    else 
      return "ioctl";
  } 
  return NULL;
}

void
set_cdtext_field_win32(void *user_data, track_t i_track, 
		       track_t i_first_track,
		       cdtext_field_t e_field, const char *psz_value)
{
  char **pp_field;
  _img_private_t *env = user_data;
  
  if( i_track == 0 )
    pp_field = &(env->cdtext.field[e_field]);
  
  else
    pp_field = &(env->tocent[i_track-i_first_track].cdtext.field[e_field]);

  *pp_field = strdup(psz_value);
}

/*!
  Return the value associated with the key "arg".
*/
static const cdtext_t *
_get_cdtext_win32 (void *user_data, track_t i_track)
{
  _img_private_t *env = user_data;

  if (NULL == env) return NULL;

  if ( NULL == env ||
       (0 != i_track 
       && i_track >= env->i_tracks + env->i_first_track ) )
    return NULL;

  if (env->hASPI) {
    env->b_cdtext_init = init_cdtext_aspi(env);
  }  else 
    env->b_cdtext_init = init_cdtext_win32ioctl(env);
    
  if (!env->b_cdtext_init) return NULL;

  if (0 == i_track) 
    return &(env->cdtext);
  else 
    return &(env->tocent[i_track-env->i_first_track].cdtext);

  return NULL;
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *user_data) 
{
  _img_private_t *env = user_data;
  
  if (!env->toc_init) _cdio_read_toc (env) ;

  return env->i_first_track;
}

/*!
  Return the media catalog number MCN.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
_cdio_get_mcn (const void *env) {

  const _img_private_t *_env = env;

  if( _env->hASPI ) {
    return get_mcn_aspi(_env);
  } else 
    return get_mcn_win32ioctl(_env);
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_num_tracks(void *user_data) 
{
  _img_private_t *env = user_data;
  
  if (!env->toc_init) _cdio_read_toc (env) ;

  return env->i_tracks;
}

/*!  
  Get format of track. 
*/
static track_format_t
_cdio_get_track_format(void *obj, track_t i_track) 
{
  _img_private_t *env = obj;
  
  if ( NULL == env ||
       ( i_track < env->i_first_track
	 || i_track >= env->i_tracks + env->i_first_track ) )
    return TRACK_FORMAT_ERROR;

  if( env->hASPI ) {
    return get_track_format_aspi(env, i_track);
  } else {
    return get_track_format_win32ioctl(env, i_track);
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
_cdio_get_track_green(void *obj, track_t i_track) 
{
  _img_private_t *env = obj;
  
  switch (_cdio_get_track_format(env, i_track)) {
  case TRACK_FORMAT_XA:
    return true;
  case TRACK_FORMAT_ERROR:
  case TRACK_FORMAT_CDI:
  case TRACK_FORMAT_AUDIO:
    return false;
  case TRACK_FORMAT_DATA:
    if (_AM_ASPI == env->access_mode ) 
      return false;
  default:
    break;
  }

  /* FIXME: Dunno if this is the right way, but it's what 
     I was using in cd-info for a while.
   */
  return ((env->tocent[i_track-env->i_first_track].Control & 2) != 0);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  i_tracks in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_tracks LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static bool
_cdio_get_track_msf(void *env, track_t i_tracks, msf_t *msf)
{
  _img_private_t *_obj = env;

  if (NULL == msf) return false;

  if (!_obj->toc_init) _cdio_read_toc (_obj) ;

  if (i_tracks == CDIO_CDROM_LEADOUT_TRACK) i_tracks = _obj->i_tracks+1;

  if (i_tracks > _obj->i_tracks+1 || i_tracks == 0) {
    return false;
  } else {
    cdio_lsn_to_msf(_obj->tocent[i_tracks-1].start_lsn, msf);
    return true;
  }
}

#endif /* HAVE_WIN32_CDROM */

/*!
  Return an array of strings giving possible CD devices.
 */
char **
cdio_get_devices_win32 (void)
{
#ifndef HAVE_WIN32_CDROM
  return NULL;
#else
  char **drives = NULL;
  unsigned int num_drives=0;
  char drive_letter;
  
  /* Scan the system for CD-ROM drives.
  */

#if FINISHED
  /* Now check the currently mounted CD drives */
  if (NULL != (ret_drive = cdio_check_mounts("/etc/mtab"))) {
    cdio_add_device_list(&drives, drive, &num_drives);
  }
  
  /* Finally check possible mountable drives in /etc/fstab */
  if (NULL != (ret_drive = cdio_check_mounts("/etc/fstab"))) {
    cdio_add_device_list(&drives, drive, &num_drives);
  }
#endif

  /* Scan the system for CD-ROM drives.
     Not always 100% reliable, so use the USE_MNTENT code above first.
  */
  for (drive_letter='A'; drive_letter <= 'Z'; drive_letter++) {
    const char *drive_str=is_cdrom_win32(drive_letter);
    if (drive_str != NULL) {
      cdio_add_device_list(&drives, drive_str, &num_drives);
    }
  }
  cdio_add_device_list(&drives, NULL, &num_drives);
  return drives;
#endif /*HAVE_WIN32_CDROM*/
}

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
    const char *drive_str=is_cdrom_win32(drive_letter);
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
  if (NULL == source_name) return false;

#ifdef HAVE_WIN32_CDROM
  if ((len == 2) && isalpha(source_name[0]) 
	    && (source_name[len-1] == ':'))
    return true;

  if ( ! WIN_NT ) return false;

  /* Test to see if of form: \\.\x: */
  return ( (len == 6) 
	   && source_name[0] == '\\' && source_name[1] == '\\'
	   && source_name[2] == '.'  && source_name[3] == '\\'
	   && isalpha(source_name[len-2])
	   && (source_name[len-1] == ':') );
#else 
  return false;
#endif
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_win32 (const char *psz_source_name)
{
#ifdef HAVE_WIN32_CDROM
  if ( WIN_NT ) {
    return cdio_open_am_win32(psz_source_name, "ioctl");
  } else {
    return cdio_open_am_win32(psz_source_name, "ASPI");
  }
#else 
  return NULL;
#endif /* HAVE_WIN32_CDROM */
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_win32 (const char *psz_orig_source, const char *psz_access_mode)
{

#ifdef HAVE_WIN32_CDROM
  CdIo *ret;
  _img_private_t *_data;
  char *psz_source;

  cdio_funcs _funcs = {
    .eject_media        = _cdio_eject_media,
    .free               = _free_win32,
    .get_arg            = _get_arg_win32,
    .get_cdtext         = _get_cdtext_win32,
    .get_default_device = cdio_get_default_device_win32,
    .get_devices        = cdio_get_devices_win32,
    .get_discmode       = get_discmode_win32,
    .get_drive_cap      = _cdio_get_drive_cap,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_mcn            = _cdio_get_mcn, 
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = NULL, /* This could be implemented if need be. */
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = NULL,
    .read               = NULL,
    .read_audio_sectors = _cdio_read_audio_sectors,
    .read_mode1_sector  = _cdio_read_mode1_sector,
    .read_mode1_sectors = _cdio_read_mode1_sectors,
    .read_mode2_sector  = _cdio_read_mode2_sector,
    .read_mode2_sectors = _cdio_read_mode2_sectors,
    .run_scsi_mmc_cmd   = scsi_mmc_run_cmd_win32,
    .set_arg            = _set_arg_win32,
    .stat_size          = _cdio_stat_size
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->access_mode    = str_to_access_mode_win32(psz_access_mode);
  _data->gen.init       = false;
  _data->gen.fd         = -1;

  if (NULL == psz_orig_source) {
    psz_source=cdio_get_default_device_win32();
    if (NULL == psz_source) return NULL;
    _set_arg_win32(_data, "source", psz_source);
    free(psz_source);
  } else {
    if (cdio_is_device_win32(psz_orig_source))
      _set_arg_win32(_data, "source", psz_orig_source);
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

  if (_cdio_init_win32(_data))
    return ret;
  else {
    _free_win32 (_data);
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
