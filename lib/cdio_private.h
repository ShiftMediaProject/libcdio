/*
    $Id: cdio_private.h,v 1.24 2004/06/26 00:39:01 rocky Exp $

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

/* Internal routines for CD I/O drivers. */


#ifndef __CDIO_PRIVATE_H__
#define __CDIO_PRIVATE_H__

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* Opaque type */
  typedef struct _CdioDataSource CdioDataSource;

  typedef struct {
    
    /*!
      Eject media in CD drive. If successful, as a side effect we 
      also free obj. Return 0 if success and 1 for failure.
    */
    int (*eject_media) (void *env);
    
    /*!
      Release and free resources associated with cd. 
    */
    void (*free) (void *env);
    
    /*!
      Return the value associated with the key "arg".
    */
    const char * (*get_arg) (void *env, const char key[]);
    
    /*!
      Return an array of device names. if CdIo is NULL (we haven't
      initialized a specific device driver), then find a suitable device 
      driver.
      
      NULL is returned if we couldn't return a list of devices.
    */
    char ** (*get_devices) (void);
    
    /*!
      Return a string containing the default CD device if none is specified.
    */
    char * (*get_default_device)(void);
    
    /*!
      Return the what kind of device we've got.
      
      See cd_types.h for a list of bitmasks for the drive type;
    */
    cdio_drive_cap_t (*get_drive_cap) (const void *env);

    /*!  
      Return the media catalog number MCN from the CD or NULL if
      there is none or we don't have the ability to get it.
    */
    char * (*get_mcn) (const void *env);

    /*!
      Return the number of of the first track. 
      CDIO_INVALID_TRACK is returned on error.
    */
    track_t (*get_first_track_num) (void *env);
    
    /*! 
      Return the number of tracks in the current medium.
      CDIO_INVALID_TRACK is returned on error.
    */
    track_t (*get_num_tracks) (void *env);
    
    /*!  
      Return the starting LBA for track number
      track_num in obj.  Tracks numbers start at 1.
      The "leadout" track is specified either by
      using track_num LEADOUT_TRACK or the total tracks+1.
      CDIO_INVALID_LBA is returned on error.
    */
    lba_t (*get_track_lba) (void *env, track_t track_num);
    
    /*!  
      Get format of track. 
    */
    track_format_t (*get_track_format) (void *env, track_t track_num);
    
    /*!
      Return true if we have XA data (green, mode2 form1) or
      XA data (green, mode2 form2). That is track begins:
      sync - header - subheader
      12     4      -  8
      
      FIXME: there's gotta be a better design for this and get_track_format?
    */
    bool (*get_track_green) (void *env, track_t track_num);
    
    /*!  
      Return the starting MSF (minutes/secs/frames) for track number
      track_num in obj.  Tracks numbers start at 1.
      The "leadout" track is specified either by
      using track_num LEADOUT_TRACK or the total tracks+1.
      False is returned on error.
    */
    bool (*get_track_msf) (void *env, track_t track_num, msf_t *msf);
    
    /*!
      lseek - reposition read/write file offset
      Returns (off_t) -1 on error. 
      Similar to libc's lseek()
    */
    off_t (*lseek) (void *env, off_t offset, int whence);
    
    /*!
      Reads into buf the next size bytes.
      Returns -1 on error. 
      Similar to libc's read()
    */
    ssize_t (*read) (void *env, void *buf, size_t size);
    
    /*!
      Reads a single mode2 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_audio_sectors) (void *env, void *buf, lsn_t lsn,
			       unsigned int nblocks);
    
    /*!
      Reads a single mode2 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_mode2_sector) (void *env, void *buf, lsn_t lsn, 
			      bool mode2_form2);
    
    /*!
      Reads nblocks of mode2 sectors from cd device into data starting
      from lsn.
      Returns 0 if no error. 
    */
    int (*read_mode2_sectors) (void *env, void *buf, lsn_t lsn, 
			       bool mode2_form2, unsigned int nblocks);
    
    /*!
      Reads a single mode1 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_mode1_sector) (void *env, void *buf, lsn_t lsn, 
			      bool mode1_form2);
    
    /*!
      Reads nblocks of mode1 sectors from cd device into data starting
      from lsn.
      Returns 0 if no error. 
    */
    int (*read_mode1_sectors) (void *env, void *buf, lsn_t lsn, 
			       bool mode1_form2, unsigned int nblocks);
    
    /*!
      Set the arg "key" with "value" in the source device.
    */
    int (*set_arg) (void *env, const char key[], const char value[]);
    
    /*!
      Return the size of the CD in logical block address (LBA) units.
    */
    uint32_t (*stat_size) (void *env);
    
  } cdio_funcs;


  /* Implementation of CdIo type */
  struct _CdIo {
    driver_id_t driver_id; /* Particular driver opened. */
    cdio_funcs op;         /* driver-specific routines handling implimentatin*/
    void *env;       /* environment. Passed to routine above. */
  };

  /*!
    Things common to private device structures. Even though not all
    devices may have some of these fields, by listing common ones
    we facilitate writing generic routines and even cut-and-paste
    code.
   */
  typedef struct {
    char *source_name;      /* Name used in open. */
    bool  init;             /* True if structure has been initialized */
    bool  toc_init;         /* True TOC read in */
    int   ioctls_debugged;  /* for debugging */

    /* Only one of the below is used. The first is for CD-ROM devices 
       and the second for stream reading (bincue, nrg, toc, network).
     */
    int   fd;               /* File descriptor of device */
    CdioDataSource *data_source;
  } generic_img_private_t;

  /* This is used in drivers that must keep their own internal 
     position pointer for doing seeks. Stream-based drivers (like bincue,
     nrg, toc, network) would use this. 
   */
  typedef struct 
  {
    off_t   buff_offset;      /* buffer offset in disk-image seeks. */
    track_t index;            /* Current track index in tocent. */
    lba_t   lba;              /* Current LBA */
  } internal_position_t;
  
  CdIo * cdio_new (void *env, const cdio_funcs *funcs);

  /* The below structure describes a specific CD Input driver  */
  typedef struct 
  {
    driver_id_t  id;
    unsigned int flags;
    const char  *name;
    const char  *describe;
    bool (*have_driver) (void); 
    CdIo *(*driver_open) (const char *psz_source_name); 
    CdIo *(*driver_open_am) (const char *psz_source_name, 
			     const char *psz_access_mode); 
    char *(*get_default_device) (void); 
    bool (*is_device) (const char *psz_source_name);
    char **(*get_devices) (void);
  } CdIo_driver_t;

  /* The below array gives of the drivers that are currently available for 
     on a particular host. */
  extern CdIo_driver_t CdIo_driver[CDIO_MAX_DRIVER];

  /* The last valid entry of Cdio_driver. -1 means uninitialzed. -2 
     means some sort of error.
   */
  extern int CdIo_last_driver; 

  /* The below array gives all drivers that can possibly appear.
     on a particular host. */
  extern CdIo_driver_t CdIo_all_drivers[CDIO_MAX_DRIVER+1];

  /*! 
    Add/allocate a drive to the end of drives. 
    Use cdio_free_device_list() to free this device_list.
  */
  void cdio_add_device_list(char **device_list[], const char *drive, 
			    int *num_drives);

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

  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_PRIVATE_H__ */
