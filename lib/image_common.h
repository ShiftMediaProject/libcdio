/*
    $Id: image_common.h,v 1.13 2004/07/17 22:16:47 rocky Exp $

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

/*! Common image routines. 
  
  Because _img_private_t may vary over image formats, the routines are
  included into the image drivers after _img_private_t is defined.  In
  order for the below routines to work, there is a large part of
  _img_private_t that is common among image drivers. For example, see
  image.h
*/

#ifndef __CDIO_IMAGE_COMMON_H__
#define __CDIO_IMAGE_COMMON_H__

#define free_if_notnull(obj) \
  if (NULL != obj) { free(obj); obj=NULL; };

/*!
  We don't need the image any more. Free all memory associated with
  it.
 */
static void 
_free_image (void *user_data) 
{
  _img_private_t *env = user_data;
  track_t i_track;

  if (NULL == env) return;

  for (i_track=0; i_track < env->i_tracks; i_track++) {
    free_if_notnull(env->tocent[i_track].filename);
    free_if_notnull(env->tocent[i_track].isrc);
    cdtext_destroy(&(env->tocent[i_track].cdtext));
  }

  free_if_notnull(env->psz_mcn);
  free_if_notnull(env->psz_cue_name);
  cdtext_destroy(&(env->cdtext));
  cdio_generic_stdio_free(env);
  free(env);
}

#ifdef NEED_MEDIA_EJECT_IMAGE
/*!
  Eject media -- there's nothing to do here except free resources.
  We always return 2.
 */
static int
_eject_media_image(void *user_data)
{
  _free_image (user_data);
  return 2;
}
#endif

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_image (void *user_data, const char key[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source")) {
    return env->gen.source_name;
  } else if (!strcmp (key, "cue")) {
    return env->psz_cue_name;
  } else if (!strcmp(key, "access-mode")) {
    return "image";
  } 
  return NULL;
}

/*! 
  Get cdtext information for a CdIo object.
  
  @param obj the CD object that may contain CD-TEXT information.
  @return the CD-TEXT object or NULL if obj is NULL
  or CD-TEXT information does not exist.
  
  If i_track is 0 or CDIO_CDROM_LEADOUT_TRACK the track returned
  is the information assocated with the CD. 
*/
static const cdtext_t *
_get_cdtext_image (void *user_data, track_t i_track)
{
  const _img_private_t *env = user_data;

  if ( NULL == env || 
       ( 0 != i_track
	 && i_track >= env->i_tracks + env->i_first_track ) )
    return NULL;

  if (CDIO_CDROM_LEADOUT_TRACK == i_track) 
    i_track = 0;
  
  if (0 == i_track) 
    return &(env->cdtext);
  else 
    return &(env->tocent[i_track-env->i_first_track].cdtext);
  
}

/*!
  Return the media catalog number (MCN) from the CD or NULL if there
  is none or we don't have the ability to get it.

  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  */
static char *
_get_mcn_image(const void *user_data)
{
  const _img_private_t *env = user_data;
  
  if (NULL == env || NULL == env->psz_mcn) return NULL;
  return strdup(env->psz_mcn);
}

/*!  
  Return the starting MSF (minutes/secs/frames) for the track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.

*/
static bool
_get_track_msf_image(void *user_data, track_t track_num, msf_t *msf)
{
  _img_private_t *env = user_data;

  if (NULL == msf) return false;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = env->i_tracks+1;

  if (track_num <= env->i_tracks+1 && track_num != 0) {
    *msf = env->tocent[track_num-env->i_first_track].start_msf;
    return true;
  } else 
    return false;
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_get_first_track_num_image(void *user_data) 
{
  _img_private_t *env = user_data;
  
  return env->i_first_track;
}

/*!
  Return the number of tracks. 
*/
static track_t
_get_num_tracks_image(void *user_data) 
{
  _img_private_t *env = user_data;

  return env->i_tracks;
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" to set the source device in I/O operations 
  is the only valid key.

  0 is returned if no error was found, and nonzero if there as an error.
*/
static int
_set_arg_image (void *user_data, const char key[], const char value[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source"))
    {
      free_if_notnull (env->gen.source_name);

      if (!value)
	return -2;

      env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "cue"))
    {
      free_if_notnull (env->psz_cue_name);

      if (!value)
	return -2;

      env->psz_cue_name = strdup (value);
    }
  else
    return -1;

  return 0;
}

/*!
  Return the the kind of drive capabilities of device.

 */
static void
_get_drive_cap_image (const void *user_data,
		      cdio_drive_read_cap_t  *p_read_cap,
		      cdio_drive_write_cap_t *p_write_cap,
		      cdio_drive_misc_cap_t  *p_misc_cap)
{

  *p_read_cap  = CDIO_DRIVE_CAP_READ_AUDIO 
    | CDIO_DRIVE_CAP_READ_CD_G
    | CDIO_DRIVE_CAP_READ_CD_R
    | CDIO_DRIVE_CAP_READ_CD_RW
    ;

  *p_write_cap = 0;

  /* In the future we may want to simulate
     LOCK, OPEN_TRAY, CLOSE_TRAY, SELECT_SPEED, etc.
  */
  *p_misc_cap  = CDIO_DRIVE_CAP_MISC_FILE;
}

#endif /* __CDIO_IMAGE_COMMON_H__ */
