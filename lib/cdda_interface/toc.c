/*
  $Id: toc.c,v 1.2 2005/01/05 04:16:11 rocky Exp $

  Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  derived from code (C) 1994-1996 Heiko Eissfeldt

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
/******************************************************************
 * Table of contents convenience functions
 ******************************************************************/

#include "low_interface.h"
#include "utils.h"

/*! Return the lsn for the start of track i_track */
lsn_t
cdda_track_firstsector(cdrom_drive_t *d, track_t i_track)
{
  if(!d->opened){
    cderror(d,"400: Device not open\n");
    return(-1);
  }

  if (i_track == 0) {
    if (d->disc_toc[0].dwStartSector == 0) {
      /* first track starts at lsn 0 -> no pre-gap */
      cderror(d,"401: Invalid track number\n");
      return(-1);
    }
    else {
      return 0; /* pre-gap of first track always starts at lba 0 */
    }
  }

  if( i_track>d->tracks) {
    cderror(d,"401: Invalid track number\n");
    return(-1);
  }
  return(d->disc_toc[i_track-1].dwStartSector);
}

/*! Get first lsn of the first audio track. -1 is returned on error. */
lsn_t
cdda_disc_firstsector(cdrom_drive_t *d)
{
  int i;
  if(!d->opened){
    cderror(d,"400: Device not open\n");
    return(-1);
  }

  /* look for an audio track */
  for ( i=0; i<d->tracks; i++ )
    if( cdda_track_audiop(d, i+1)==1 ) {
      if (i == 0) /* disc starts at lba 0 if first track is an audio track */
       return 0;
      else
       return cdda_track_firstsector(d, i+1);
    }

  cderror(d,"403: No audio tracks on disc\n");  
  return(-1);
}

/*! Get last lsn of the track. The lsn is generally one less than the
  start of the next track. -1 is returned on error. */
lsn_t
cdda_track_lastsector(cdrom_drive_t *d, track_t i_track)
{
  if (!d->opened) {
    cderror(d,"400: Device not open\n");
    return -1;
  }

  if (i_track == 0) {
    if (d->disc_toc[0].dwStartSector == 0) {
      /* first track starts at lba 0 -> no pre-gap */
      cderror(d,"401: Invalid track number\n");
      return(-1);
    }
    else {
      return d->disc_toc[0].dwStartSector-1;
    }
  }

  if ( i_track<1 || i_track>d->tracks ) {
    cderror(d,"401: Invalid track number\n");
    return -1;
  }
  /* Safe, we've always the leadout at disc_toc[tracks] */
  return(d->disc_toc[i_track].dwStartSector-1);
}

/*! Get last lsn of the last audio track. The last lssn generally one
  less than the start of the next track after the audio track. -1 is
  returned on error. */
lsn_t
cdda_disc_lastsector(cdrom_drive_t *d)
{
  int i;
  if (!d->opened) {
    cderror(d,"400: Device not open\n");
    return -1;
  }

  /* look for an audio track */
  for ( i=d->tracks-1; i>=0; i-- )
    if ( cdda_track_audiop(d,i+1) == 1 )
      return (cdda_track_lastsector(d,i+1));

  cderror(d,"403: No audio tracks on disc\n");  
  return -1;
}

track_t
cdda_tracks(cdrom_drive_t *d)
{
  if (!d->opened){
    cderror(d,"400: Device not open\n");
    return -1;
  }
  return(d->tracks);
}

int 
cdda_sector_gettrack(cdrom_drive_t *d, lsn_t lsn)
{
  if (!d->opened) {
    cderror(d,"400: Device not open\n");
    return -1;
  } else {
    if (lsn < d->disc_toc[0].dwStartSector)
      return 0; /* We're in the pre-gap of first track */

    return cdio_get_track(d->p_cdio, lsn);
  }
}

/*! Return number of channels in track: 2 or 4; -2 if not
  implemented or -1 for error.
  Not meaningful if track is not an audio track.
*/
int 
cdda_track_channels(cdrom_drive_t *d, track_t i_track)
{
  return(cdio_get_track_channels(d->p_cdio, i_track));
}

/*! Return 1 is track is an audio track, 0 otherwise. */
int 
cdda_track_audiop(cdrom_drive_t *d, track_t i_track)
{
  track_format_t track_format = cdio_get_track_format(d->p_cdio, i_track);
  return TRACK_FORMAT_AUDIO == track_format ? 1 : 0;
}

/*! Return 1 is track is an audio track, 0 otherwise. */
int 
cdda_track_copyp(cdrom_drive_t *d, track_t i_track)
{
  track_flag_t track_flag = cdio_get_track_copy_permit(d->p_cdio, i_track);
  return CDIO_TRACK_FLAG_TRUE == track_flag ? 1 : 0;
}

/*! Return 1 is audio track has linear preemphasis set, 0 otherwise. 
    Only makes sense for audio tracks.
 */
int 
cdda_track_preemp(cdrom_drive_t *d, track_t i_track)
{
  track_flag_t track_flag = cdio_get_track_preemphasis(d->p_cdio, i_track);
  return CDIO_TRACK_FLAG_TRUE == track_flag ? 1 : 0;
}

