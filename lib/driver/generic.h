/*
    $Id: generic.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

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

/* Internal routines for CD I/O drivers. */


#ifndef __CDIO_GENERIC_H__
#define __CDIO_GENERIC_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <cdio/iso9660.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /*!
    Things common to private device structures. Even though not all
    devices may have some of these fields, by listing common ones
    we facilitate writing generic routines and even cut-and-paste
    code.
   */
  typedef struct {
    char *source_name;      /**< Name used in open. */
    bool  init;             /**< True if structure has been initialized */
    bool  toc_init;         /**< True if TOC read in */
    bool  b_cdtext_init;    /**< True if CD-Text read in */
    bool  b_cdtext_error;   /**< True if trouble reading CD-Text */
    
    int   ioctls_debugged;  /**< for debugging */

    /* Only one of data_source or fd is used; fd  is for CD-ROM
       devices and the data_source for stream reading (bincue, nrg, toc,
       network).
     */
    CdioDataSource *data_source;
    int     fd;             /**< File descriptor of device */
    track_t i_first_track;  /**< The starting track number. */
    track_t i_tracks;       /**< The number of tracks. */

    uint8_t i_joliet_level; /**< 0 = no Joliet extensions.
			       1-3: Joliet level. */
    iso9660_pvd_t pvd;      
    iso9660_svd_t svd;      
    CdIo *cdio;             /**< a way to call general cdio routines. */
    cdtext_t  cdtext;       /**< CD-Text for disc. */
    cdtext_t  cdtext_track[CDIO_CD_MAX_TRACKS+1]; /*CD-TEXT for each track*/

  } generic_img_private_t;

  /*!
    Bogus eject media when there is no ejectable media, e.g. a disk image
    We always return 2. Should we also free resources? 
  */
  int cdio_generic_bogus_eject_media (void *env);

  /*!
    Release and free resources associated with cd. 
  */
  void cdio_generic_free (void *env);

  /*!
    Initialize CD device.
  */
  bool cdio_generic_init (void *env);

  /*!
    Reads into buf the next size bytes.
    Returns -1 on error. 
    Is in fact libc's read().
  */
  off_t cdio_generic_lseek (void *env, off_t offset, int whence);

  /*!
    Reads into buf the next size bytes.
    Returns -1 on error. 
    Is in fact libc's read().
  */
  ssize_t cdio_generic_read (void *env, void *buf, size_t size);

  /*!
    Reads a single form1 sector from cd device into data starting
    from lsn. Returns 0 if no error. 
  */
  int cdio_generic_read_form1_sector (void * user_data, void *data, 
				      lsn_t lsn);
  
  /*!
    Release and free resources associated with stream or disk image.
  */
  void cdio_generic_stdio_free (void *env);

  /*!  
    Return true if source_name could be a device containing a CD-ROM on
    Win32
  */
  bool cdio_is_device_win32(const char *source_name);

  
  /*!  
    Return true if source_name could be a device containing a CD-ROM on
    most Unix servers with block and character devices.
  */
  bool cdio_is_device_generic(const char *source_name);

  
  /*!  
    Like above, but don't give a warning device doesn't exist.
  */
  bool cdio_is_device_quiet_generic(const char *source_name);

  /*! 
    Get cdtext information for a CdIo object .
    
    @param obj the CD object that may contain CD-TEXT information.
    @return the CD-TEXT object or NULL if obj is NULL
    or CD-TEXT information does not exist.
  */
  const cdtext_t *get_cdtext_generic (void *p_user_data, track_t i_track);

  /*!
    Return the number of of the first track. 
    CDIO_INVALID_TRACK is returned on error.
  */
  track_t get_first_track_num_generic(void *p_user_data);

  /*!
    Return the number of tracks in the current medium.
  */
  track_t get_num_tracks_generic(void *p_user_data);
  
  /*! 
    Get disc type associated with cd object.
  */
  discmode_t get_discmode_generic (void *p_user_data );
  
  /*! 
    Same as above but only handles CD cases
  */
  discmode_t get_discmode_cd_generic (void *p_user_data );
  
  void set_cdtext_field_generic(void *user_data, track_t i_track, 
				track_t i_first_track,
				cdtext_field_t e_field, const char *psz_value);
  /*!
    Read cdtext information for a CdIo object .
  
    return true on success, false on error or CD-Text information does
    not exist.
  */
  bool init_cdtext_generic (generic_img_private_t *p_env);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_GENERIC_H__ */
