/*
    $Id: cdio_private.h,v 1.2 2003/03/29 17:32:00 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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

#ifdef  HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  typedef struct {
    
    /*!
      Eject media in CD drive. If successful, as a side effect we 
      also free obj. Return 0 if success and 1 for failure.
    */
    int (*eject_media) (void *user_data);
    
    /*!
      Release and free resources associated with cd. 
    */
    void (*free) (void *user_data);
    
    /*!
      Return the value associated with the key "arg".
    */
    const char * (*get_arg) (void *user_data, const char key[]);
    
    /*!
      Return a string containing the default VCD device if none is specified.
    */
    char * (*get_default_device)(void);
    
    /*!
      Return the number of of the first track. 
      CDIO_INVALID_TRACK is returned on error.
    */
    track_t (*get_first_track_num) (void *user_data);
    
    /*! 
      Return the number of tracks in the current medium.
      CDIO_INVALID_TRACK is returned on error.
    */
    track_t (*get_num_tracks) (void *user_data);
    
    /*!  
      Return the starting LBA for track number
      track_num in obj.  Tracks numbers start at 1.
      The "leadout" track is specified either by
      using track_num LEADOUT_TRACK or the total tracks+1.
      CDIO_INVALID_LBA is returned on error.
    */
    lba_t (*get_track_lba) (void *user_data, track_t track_num);
    
    /*!  
      Get format of track. 
    */
    track_format_t (*get_track_format) (void *user_data, track_t track_num);
    
    /*!
      Return true if we have XA data (green, mode2 form1) or
      XA data (green, mode2 form2). That is track begins:
      sync - header - subheader
      12     4      -  8
      
      FIXME: there's gotta be a better design for this and get_track_format?
    */
    bool (*get_track_green) (void *user_data, track_t track_num);
    
    /*!  
      Return the starting MSF (minutes/secs/frames) for track number
      track_num in obj.  Tracks numbers start at 1.
      The "leadout" track is specified either by
      using track_num LEADOUT_TRACK or the total tracks+1.
      False is returned on error.
    */
    bool (*get_track_msf) (void *user_data, track_t track_num, msf_t *msf);
    
    /*!
      lseek - reposition read/write file offset
      Returns (off_t) -1 on error. 
      Similar to libc's lseek()
    */
    off_t (*lseek) (void *user_data, off_t offset, int whence);
    
    /*!
      Reads into buf the next size bytes.
      Returns -1 on error. 
      Similar to libc's read()
    */
    ssize_t (*read) (void *user_data, void *buf, size_t size);
    
    /*!
      Reads a single mode2 sector from cd device into buf starting
      from lsn. Returns 0 if no error. 
    */
    int (*read_mode2_sector) (void *user_data, void *buf, lsn_t lsn, 
			      bool mode2raw);
    
    /*!
      Reads nblocks of mode2 sectors from cd device into data starting
      from lsn.
      Returns 0 if no error. 
    */
    int (*read_mode2_sectors) (void *user_data, void *buf, lsn_t lsn, 
			       bool mode2raw, unsigned nblocks);
    
    /*!
      Set the arg "key" with "value" in the source device.
    */
    int (*set_arg) (void *user_data, const char key[], const char value[]);
    
    /*!
      Return the size of the CD in logical block address (LBA) units.
    */
    uint32_t (*stat_size) (void *user_data);
    
  } cdio_funcs;


  /* Things that just about all private device structures have. */
  typedef struct {
    char *source_name;     /* Name used in open. */
    bool init;             /* True if structure has been initialized */
    int fd;                /* File descriptor of device */
  } generic_img_private_t;

  CdIo * cdio_new (void *user_data, const cdio_funcs *funcs);

  /* The below structure describes a specific CD Input driver  */
  typedef struct 
  {
    unsigned int flags;
    driver_id_t  id;
    const char  *name;
    const char  *describe;
    bool (*have_driver) (void); 
    CdIo *(*driver_open) (const char *source_name); 
  } CdIo_driver_t;

  /* The below array gives of the drivers that are currently available for 
     on a particular host. */
  extern CdIo_driver_t CdIo_driver[MAX_DRIVER];

  /* The last valid entry of Cdio_driver. -1 means uninitialzed. -2 
     means some sort of error.
   */
  extern int CdIo_last_driver; 

  /* The below array gives all drivers that can possibly appear.
     on a particular host. */
  extern CdIo_driver_t CdIo_all_drivers[MAX_DRIVER+1];

  /*!
    Release and free resources associated with cd. 
  */
  void cdio_generic_free (void *user_data);

  /*!
    Reads into buf the next size bytes.
    Returns -1 on error. 
    Is in fact libc's read().
  */
  off_t cdio_generic_lseek (void *user_data, off_t offset, int whence);

  /*!
    Reads into buf the next size bytes.
    Returns -1 on error. 
    Is in fact libc's read().
  */
  ssize_t cdio_generic_read (void *user_data, void *buf, size_t size);

  /*!
    Reads nblocks of mode2 sectors from cd device into data starting
    from lsn.
    Returns 0 if no error. 
  */
  int cdio_generic_read_mode2_sectors (CdIo *obj, void *buf, lsn_t lsn, 
				       bool mode2raw,  unsigned num_sectors);

  struct _CdIo {
    void *user_data;
    cdio_funcs op;
  };


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_PRIVATE_H__ */
