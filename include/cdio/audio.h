/* -*- c -*-
    $Id: audio.h,v 1.2 2005/03/01 10:53:15 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file audio.h 
 *
 *  \brief The top-level header for CD audio-related libcdio
 *         calls.  These control playing of the CD-ROM through its 
 *         line-out jack.
 */
#ifndef __CDIO_AUDIO_H__
#define __CDIO_AUDIO_H__

#include <cdio/types.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /*! This struct is used by the cdio_audio_read_subchannel */
  typedef struct cdio_subchannel_s 
  {
    uint8_t	format;
    uint8_t	audio_status;
    uint8_t	address:	4;
    uint8_t	control:	4;
    uint8_t	track;
    uint8_t	index;
    union cdio_cdrom_addr abs_addr;
    union cdio_cdrom_addr rel_addr;
  } cdio_subchannel_t;
  
  /*! This struct is used by cdio_audio_get_volume and cdio_audio_set_volume */
  typedef struct cdio_audio_volume_s
  {
    uint8_t	channel0;
    uint8_t	channel1;
    uint8_t	channel2;
    uint8_t	channel3;
  } cdio_audio_volume_t;
  

  /* This struct is used by the CDROMPLAYTRKIND ioctl */
  typedef struct cdio_track_index_s
  {
    uint8_t	i_start_track;	/* start track */
    uint8_t	i_start_index;	/* start index */
    uint8_t	i_end_track;	/* end track */
    uint8_t	i_end_index;	/* end index */
  } cdio_track_index_t;

  /*!
    Get volume of an audio CD.
    
    @param p_cdio the CD object to be acted upon.

  */
  driver_return_code_t cdio_audio_get_volume (CdIo_t *p_cdio,  /*out*/
					      cdio_audio_volume_t *p_volume);

  /*!
    Pause playing CD through analog output

    @param p_cdio the CD object to be acted upon.
  */
  driver_return_code_t cdio_audio_pause (CdIo_t *p_cdio);

  /*!
    Playing CD through analog output at the given MSF.

    @param p_cdio the CD object to be acted upon.
  */
  driver_return_code_t cdio_audio_play_msf (CdIo_t *p_cdio, msf_t *p_msf);

  /*!
    Playing CD through analog output at the desired track and index

    @param p_cdio the CD object to be acted upon.
    @param p_track_index location to start/end.
  */
  driver_return_code_t cdio_audio_play_track_index 
  ( CdIo_t *p_cdio,  cdio_track_index_t *p_track_index);

  /*!
    Get subchannel information.

    @param p_cdio the CD object to be acted upon.
  */
  driver_return_code_t cdio_audio_read_subchannel (CdIo_t *p_cdio, 
						   cdio_subchannel_t *p_subchannel);

  /*!
    Resume playing an audio CD.
    
    @param p_cdio the CD object to be acted upon.

  */
  driver_return_code_t cdio_audio_resume (CdIo_t *p_cdio);

  /*!
    Set volume of an audio CD.
    
    @param p_cdio the CD object to be acted upon.

  */
  driver_return_code_t cdio_audio_set_volume (CdIo_t *p_cdio,  const
					      cdio_audio_volume_t *p_volume);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_AUDIO_H__ */
