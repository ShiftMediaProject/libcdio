/* -*- c -*-
    $Id: track.h,v 1.1 2004/12/31 05:48:09 rocky Exp $

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

/** \file track.h 
 *  \brief  The top-level header for track-related libcdio calls
 */
#ifndef __CDIO_TRACK_H__
#define __CDIO_TRACK_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  typedef enum {
    CDIO_TRACK_FLAG_FALSE, 
    CDIO_TRACK_FLAG_TRUE, 
    CDIO_TRACK_FLAG_ERROR, 
    CDIO_TRACK_FLAG_UNKNOWN
  } track_flag_t;

  typedef struct {
    track_flag_t preemphasis;
    track_flag_t copy_permit;
    int channels;
  } track_flags_t;
    
  /*! Return number of channels in track: 2 or 4; -2 if not
      implemented or -1 for error.
      Not meaningful if track is not an audio track.
  */
  int cdio_get_track_channels(const CdIo *p_cdio, track_t i_track);
  
  /*! Return copy protection status on a track. Is this meaningful
      if not an audio track?
   */
  track_flag_t cdio_get_track_copy_permit(const CdIo *p_cdio, track_t i_track);
  
  /*!  
    Get the format (audio, mode2, mode1) of track. 
  */
  track_format_t cdio_get_track_format(const CdIo *p_cdio, track_t i_track);
  
  /*!
    Return true if we have XA data (green, mode2 form1) or
    XA data (green, mode2 form2). That is track begins:
    sync - header - subheader
    12     4      -  8
    
    FIXME: there's gotta be a better design for this and get_track_format?
  */
  bool cdio_get_track_green(const CdIo *p_cdio, track_t i_track);
    
  /*!  
    Get the starting LBA for track number
    i_track in p_cdio.  Track numbers usually start at something 
    greater than 0, usually 1.

    The "leadout" track is specified either by
    using i_track CDIO_CDROM_LEADOUT_TRACK or the total tracks+1.

    @param p_cdio object to get information from
    @param i_track  the track number we want the LSN for
    @return the starting LBA or CDIO_INVALID_LBA on error.
  */
  lba_t cdio_get_track_lba(const CdIo *p_cdio, track_t i_track);
  
  /*!  
    Return the starting MSF (minutes/secs/frames) for track number
    i_track in p_cdio.  Track numbers usually start at something 
    greater than 0, usually 1.

    The "leadout" track is specified either by
    using i_track CDIO_CDROM_LEADOUT_TRACK or the total tracks+1.

    @param p_cdio object to get information from
    @param i_track  the track number we want the LSN for
    @return the starting LSN or CDIO_INVALID_LSN on error.
  */
  lsn_t cdio_get_track_lsn(const CdIo *p_cdio, track_t i_track);
  
  /*!  
    Return the starting MSF (minutes/secs/frames) for track number
    i_track in p_cdio.  Track numbers usually start at something 
    greater than 0, usually 1.

    The "leadout" track is specified either by
    using i_track CDIO_CDROM_LEADOUT_TRACK or the total tracks+1.
    
    @return true if things worked or false if there is no track entry.
  */
  bool cdio_get_track_msf(const CdIo *p_cdio, track_t i_track, 
			  /*out*/ msf_t *msf);
  
  /*! Get linear preemphasis status on an audio track 
      This is not meaningful if not an audio track?
   */
  track_flag_t cdio_get_track_preemphasis(const CdIo *p_cdio, track_t i_track);
  
  /*!  
    Get the number of sectors between this track an the next.  This
    includes any pregap sectors before the start of the next track.
    Track numbers usually start at something 
    greater than 0, usually 1.

    @return the number of sectors or 0 if there is an error.
  */
  unsigned int cdio_get_track_sec_count(const CdIo *p_cdio, track_t i_track);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_H__ */

