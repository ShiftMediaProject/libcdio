/* -*- c -*-
    $Id: cdio.h,v 1.32 2003/11/04 12:28:08 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
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

/* Public CD Input and Control Interface . */


#ifndef __CDIO_H__
#define __CDIO_H__

/** Application Interface or Protocol version number. If the public
 *  interface changes, we increase this number.
 */
#define CDIO_API_VERSION 1

#include <cdio/version.h>

#ifdef  HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cdio/types.h>
#include <cdio/sector.h>

/* Flags specifying the category of device to open or is opened. */

#define CDIO_SRC_IS_DISK_IMAGE_MASK 0x0001 /**< Read source is a CD image. */
#define CDIO_SRC_IS_DEVICE_MASK     0x0002 /**< Read source is a CD device. */
#define CDIO_SRC_IS_SCSI_MASK       0x0004
#define CDIO_SRC_IS_NATIVE_MASK     0x0008

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /** This is an opaque structure. */
  typedef struct _CdIo CdIo; 

  /** The below enumerations may be used to tag a specific driver
   * that is opened or is desired to be opened. Note that this is
   * different than what is available on a given host.
   *
   * Order is a little significant since the order is used in scans.
   * We have to start with UNKNOWN and devices should come before
   * disk-image readers. By putting something towards the top (a lower
   * enumeration number), in an iterative scan we prefer that to
   * something with a higher enumeration number.
   *
   * NOTE: IF YOU MODIFY ENUM MAKE SURE INITIALIZATION IN CDIO.C AGREES.
   *     
   */
  typedef enum  {
    DRIVER_UNKNOWN, 
    DRIVER_BSDI,    /**< BSDI driver */
    DRIVER_FREEBSD, /**< FreeBSD driver */
    DRIVER_LINUX,   /**< GNU/Linux Driver */
    DRIVER_SOLARIS, /**< Sun Solaris Driver */
    DRIVER_OSX,     /**< Apple OSX Driver */
    DRIVER_WIN32,   /**< Microsoft Windows Driver */
    DRIVER_BINCUE,  /**< BIN/CUE format CD image. This is listed before NRG, 
		         to make the code prefer BINCUE over NRG when both 
			 exist. */
    DRIVER_NRG,     /**< Nero NRG format CD image. */
    DRIVER_DEVICE   /**< Is really a set of the above; should come last */
  } driver_id_t;

/** Make sure what's listed for CDIO_MIN_DRIVER is the last
    enumeration in driver_id_t. Since we have a bogus (but useful) 0th
    entry above we don't have to add one below.
*/
#define CDIO_MIN_DRIVER        DRIVER_BSDI
#define CDIO_MIN_DEVICE_DRIVER CDIO_MIN_DRIVER
#define CDIO_MAX_DRIVER        DRIVER_NRG
#define CDIO_MAX_DEVICE_DRIVER DRIVER_WIN32

  typedef enum  {
    TRACK_FORMAT_AUDIO,   /**< Audio track, e.g. CD-DA */
    TRACK_FORMAT_CDI,     /**< CD-i. How this is different from DATA below? */
    TRACK_FORMAT_XA,      /**< Mode2 of some sort */
    TRACK_FORMAT_DATA,    /**< Mode1 of some sort */
    TRACK_FORMAT_PSX,     /**< Playstation CD. Like audio but only 2336 bytes
			   *   of user data.
			   */
    TRACK_FORMAT_ERROR    /**< Dunno what is, or some other error. */
  } track_format_t;

  /*! Printable tags for track_format_t enumeration.  */
  extern const char *track_format2str[6];
  
  /*!
    Eject media in CD drive if there is a routine to do so. 
    Return 0 if success and 1 for failure, and 2 if no routine.
    If the CD is ejected *obj is freed and obj set to NULL.
  */
  int cdio_eject_media (CdIo **obj);

  /*!
    Free any resources associated with obj.
   */
  void cdio_destroy (CdIo *obj);

  /*!
    Free device list returned by cdio_get_devices or
    cdio_get_devices_with_cap.
  */
  void cdio_free_device_list (char * device_list[]);

  /*!
    Return the value associatied with key. NULL is returned if obj is NULL
    or "key" does not exist.
  */
  const char * cdio_get_arg (const CdIo *obj,  const char key[]);

  /*!
    Return an array of device names in search_devices that have at
    least the capabilities listed by cap.  If search_devices is NULL,
    then we'll search all possible CD drives.  
    
    If "any" is set false then every capability listed in the extended
    portion of capabilities (i.e. not the basic filesystem) must be
    satisified. If "any" is set true, then if any of the capabilities
    matches, we call that a success.

    To find a CD-drive of any type, use the mask CDIO_FS_MATCH_ALL.
  
    NULL is returned if we couldn't get a default device.
  */
  char ** cdio_get_devices_with_cap (char* search_devices[],
				     cdio_fs_anal_t capabilities, bool any);

  /*! Return an array of device names. If you want a specific
    devices, dor a driver give that device, if you want hardware
    devices, give DRIVER_DEVICE and if you want all possible devices,
    image drivers and hardware drivers give DRIVER_UNKNOWN.
    
    NULL is returned if we couldn't return a list of devices.
  */
  char ** cdio_get_devices (driver_id_t driver);

  /*!
    Return a string containing the default CD device.
    if obj is NULL (we haven't initialized a specific device driver), 
    then find a suitable one and return the default device for that.
    
    NULL is returned if we couldn't get a default device.
  */
  char * cdio_get_default_device (const CdIo *obj);

  /*!
    Return the media catalog number (MCN) from the CD or NULL if there
    is none or we don't have the ability to get it.

    Note: string is malloc'd so caller has to free() the returned
    string when done with it.
  */
  char *cdio_get_mcn (const CdIo *obj);

  /*!
    Return a string containing the name of the driver in use.
    if CdIo is NULL (we haven't initialized a specific device driver), 
    then return NULL.
  */
  const char * cdio_get_driver_name (const CdIo *obj);

  /*!
    Return the number of the first track. 
    CDIO_INVALID_TRACK is returned on error.
  */
  track_t cdio_get_first_track_num(const CdIo *obj);
  
  /*!
    Return a string containing the default CD device if none is specified.
  */
  track_t cdio_get_num_tracks (const CdIo *obj);
  
  /*!  
    Get the format (audio, mode2, mode1) of track. 
  */
  track_format_t cdio_get_track_format(const CdIo *obj, track_t track_num);
  
  /*!
    Return true if we have XA data (green, mode2 form1) or
    XA data (green, mode2 form2). That is track begins:
    sync - header - subheader
    12     4      -  8
    
    FIXME: there's gotta be a better design for this and get_track_format?
  */
  bool cdio_get_track_green(const CdIo *obj, track_t track_num);
    
  /*!  
    Return the starting LBA for track number
    track_num in obj.  Tracks numbers start at 1.
    The "leadout" track is specified either by
    using track_num LEADOUT_TRACK or the total tracks+1.
    CDIO_INVALID_LBA is returned on error.
  */
  lba_t cdio_get_track_lba(const CdIo *obj, track_t track_num);
  
  /*!  
    Return the starting LSN for track number
    track_num in obj.  Tracks numbers start at 1.
    The "leadout" track is specified either by
    using track_num LEADOUT_TRACK or the total tracks+1.
    CDIO_INVALID_LBA is returned on error.
  */
  lsn_t cdio_get_track_lsn(const CdIo *obj, track_t track_num);
  
  /*!  
    Return the starting MSF (minutes/secs/frames) for track number
    track_num in obj.  Track numbers start at 1.
    The "leadout" track is specified either by
    using track_num LEADOUT_TRACK or the total tracks+1.
    False is returned if there is no track entry.
  */
  bool cdio_get_track_msf(const CdIo *obj, track_t track_num, 
			  /*out*/ msf_t *msf);
  
  /*!  
    Return the number of sectors between this track an the next.  This
    includes any pregap sectors before the start of the next track.
    Tracks start at 1.
    0 is returned if there is an error.
  */
  unsigned int cdio_get_track_sec_count(const CdIo *obj, track_t track_num);

  /*!
    lseek - reposition read/write file offset
    Returns (off_t) -1 on error. 
    Similar to (if not the same as) libc's lseek()
  */
  off_t cdio_lseek(const CdIo *obj, off_t offset, int whence);
    
  /*!
    Reads into buf the next size bytes.
    Returns -1 on error. 
    Similar to (if not the same as) libc's read()
  */
  ssize_t cdio_read(const CdIo *obj, void *buf, size_t size);
    
  /*!
    Reads a audio sector from cd device into data starting
    from lsn. Returns 0 if no error. 
  */
  int cdio_read_audio_sector (const CdIo *obj, void *buf, lsn_t lsn);

  /*!
    Reads a audio sector from cd device into data starting
    from lsn. Returns 0 if no error. 
  */
  int cdio_read_audio_sectors (const CdIo *obj, void *buf, lsn_t lsn,
			       unsigned int nblocks);

  /*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
  */
  int cdio_read_mode1_sector (const CdIo *obj, void *buf, lsn_t lsn, 
			      bool is_form2);
  
  /*!
    Reads nblocks of mode1 sectors from cd device into data starting
    from lsn. Returns 0 if no error. 
  */
  int cdio_read_mode1_sectors (const CdIo *obj, void *buf, lsn_t lsn, 
			       bool is_form2, unsigned int num_sectors);
  
  /*!
    Reads a single mode2 sector from cd device into data starting
    from lsn. Returns 0 if no error. 
  */
  int cdio_read_mode2_sector (const CdIo *obj, void *buf, lsn_t lsn, 
			      bool is_form2);
  
  /*!
    Reads nblocks of mode2 sectors from cd device into data starting
    from lsn.
    Returns 0 if no error. 
  */
  int cdio_read_mode2_sectors (const CdIo *obj, void *buf, lsn_t lsn, 
			       bool is_form2, unsigned int num_sectors);
  
  /*!
    Set the arg "key" with "value" in the source device.
    0 is returned if no error was found, and nonzero if there as an error.
  */
  int cdio_set_arg (CdIo *obj, const char key[], const char value[]);
  
  /*!
    Return the size of the CD in logical block address (LBA) units.
  */
  uint32_t cdio_stat_size (const CdIo *obj);
  
  /*!
    Initialize CD Reading and control routines. Should be called first.
  */
  bool cdio_init(void);
  
  /* True if xxx driver is available. where xxx=linux, solaris, nrg, ...
   */
  bool cdio_have_bsdi    (void);
  bool cdio_have_freebsd (void);
  bool cdio_have_linux   (void);
  bool cdio_have_solaris (void);
  bool cdio_have_osx     (void);
  bool cdio_have_win32   (void);
  bool cdio_have_nrg     (void);
  bool cdio_have_bincue  (void);

  /* Like above but uses the enumeration instead. */
  bool cdio_have_driver (driver_id_t driver_id);
  
  /*! Return a string decribing driver_id. */
  const char *cdio_driver_describe (driver_id_t driver_id);
  
  /*! Sets up to read from place specified by source_name and
     driver_id This should be called before using any other routine,
     except cdio_init. This will call cdio_init, if that hasn't been
     done previously.  to call one of the specific routines below.

     NULL is returned on error.
  */
  CdIo * cdio_open (const char *source_name, driver_id_t driver_id);

  /*! cdrao BIN/CUE CD disk-image routines. Source is the .bin file

     NULL is returned on error.
   */
  CdIo * cdio_open_bincue (const char *bin_name);
  
  char * cdio_get_default_device_bincue(void);

  char **cdio_get_devices_bincue(void);

  /*! CD routines. Source is the some sort of device.

     NULL is returned on error.
   */
  CdIo * cdio_open_cd (const char *device_name);

  /*! cdrao BIN/CUE CD disk-image routines. Source is the .cue file

     NULL is returned on error.
   */
  CdIo * cdio_open_cue (const char *cue_name);

  /*! BSDI CD-reading routines. 
     NULL is returned on error.
  */
  CdIo * cdio_open_bsdi (const char *source_name);
  
  char * cdio_get_default_device_bsdi(void);

  char **cdio_get_devices_bsdi(void);
  
  /*! BSDI CD-reading routines. 
     NULL is returned on error.
  */
  CdIo * cdio_open_freebsd (const char *source_name);
  
  char * cdio_get_default_device_freebsd(void);

  char **cdio_get_devices_freebsd(void);
  
  /*! Linux CD-reading routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_linux (const char *source_name);

  char * cdio_get_default_device_linux(void);

  char **cdio_get_devices_linux(void);
  
  /*! Solaris CD-reading routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_solaris (const char *source_name);
  
  char * cdio_get_default_device_solaris(void);
  
  char **cdio_get_devices_solaris(void);
  
  /*! Darwin OS X CD-reading routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_osx (const char *source_name);

  char * cdio_get_default_device_osx(void);
  
  char **cdio_get_devices_osx(void);
  
  /*! Win32 CD-reading routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_win32 (const char *source_name);
  
  char * cdio_get_default_device_win32(void);

  char **cdio_get_devices_win32(void);
  
  /*! Nero CD disk-image routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_nrg (const char *source_name);
  
  char * cdio_get_default_device_nrg(void);

  char **cdio_get_devices_nrg(void);

  /*! Return corresponding BIN file if cue_name is a cue file or NULL
    if not a CUE file.
  */
  char *cdio_is_cuefile(const char *cue_name);
  
  /*! Return corresponding CUE file if bin_name is a fin file or NULL
    if not a BIN file. NOTE: when we handle TOC something will have to 
    change here....
  */
  char *cdio_is_binfile(const char *bin_name);
  
  /*! Return true if source name is a device.
  */
  bool cdio_is_device(const char *source_name, driver_id_t driver_id);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_H__ */
