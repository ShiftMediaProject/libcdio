/*
    $Id: cdio.h,v 1.6 2003/04/04 00:41:10 rocky Exp $

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

#ifdef  HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "types.h"
#include "sector.h"

/* Flags specifying the category of device to open or is opened. */
#define CDIO_SRC_IS_DISK_IMAGE_MASK 0x0001
#define CDIO_SRC_IS_DEVICE_MASK     0x0002
#define CDIO_SRC_IS_SCSI_MASK       0x0004
#define CDIO_SRC_IS_NATIVE_MASK     0x0008

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* opaque structure */
  typedef struct _CdIo CdIo; 

  /* The below enumerations may be used to tag a specific driver
     that is opened or is desired to be opened. Note that this is
     different than what is available on a given host. 

     Order is a little significant since the order is used in scans.
     We have to start with UNKNOWN and devices should come before
     disk-image readers. By putting something towards the top (a lower
     enumeration number), in an iterative scan we prefer that to something
     with a higher enumeration number.

     NOTE: IF YOU MODIFY ENUM MAKE SURE INITIALIZATION IN CDIO.C AGREES.
     
  */
  typedef enum  {
    DRIVER_UNKNOWN, 
    DRIVER_BSDI, 
    DRIVER_FREEBSD, 
    DRIVER_LINUX,
    DRIVER_SOLARIS, 
    DRIVER_BINCUE, /* Prefer bincue over nrg when both exist */
    DRIVER_NRG,    
    DRIVER_DEVICE, /* Is really a set of the above; should come last */
  } driver_id_t;

  /* Make sure what's listed below is the last one above. Since we have
     a bogus (but useful) 0th entry above we don't have to add one below.
   */
#define MAX_DRIVER DRIVER_NRG


  typedef enum  {
    TRACK_FORMAT_AUDIO,   /* Audio track, e.g. CD-DA */
    TRACK_FORMAT_CDI,     /* CD-i. How this is different from DATA below? */
    TRACK_FORMAT_XA,      /* Mode2 of some sort */
    TRACK_FORMAT_DATA,    /* Mode1 of some sort */
    TRACK_FORMAT_ERROR    /* Dunno what is or some other error. */
  } track_format_t;

  /* Printable tags for above enumeration.  */
  extern const char *track_format2str[5];
  
  /*!
    Eject media in CD drive if there is a routine to do so. 
    Return 0 if success and 1 for failure, and 2 if no routine.
  */
  int cdio_eject_media (const CdIo *obj);

  /*!
    Free any resources associated with obj.
   */
  void cdio_destroy (CdIo *obj);

  /*!
    Return the value associated with the key "arg".
  */
  const char * cdio_get_arg (const CdIo *obj,  const char key[]);

  /*!
    Return a string containing the default CD device if none is specified.

    NULL is returned if we couldn't get a default device.
  */
  char * cdio_get_default_device (const CdIo *obj);

  /*!
    Return the number of of the first track. 
    CDIO_INVALID_TRACK is returned on error.
  */
  track_t cdio_get_first_track_num(const CdIo *obj);
  
  /*!
    Return a string containing the default CD device if none is specified.
  */
  track_t cdio_get_num_tracks (const CdIo *obj);
  
  /*!  
    Get format of track. 
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
  off_t cdio_lseek(CdIo *obj, off_t offset, int whence);
    
  /*!
    Reads into buf the next size bytes.
    Returns -1 on error. 
    Similar to (if not the same as) libc's read()
  */
  ssize_t cdio_read(CdIo *obj, void *buf, size_t size);
    
  /*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
  int cdio_read_mode2_sector (CdIo *obj, void *buf, lsn_t lsn, bool mode2raw);

  /*!
    Reads nblocks of mode2 sectors from cd device into data starting
    from lsn.
    Returns 0 if no error. 
  */
  int cdio_read_mode2_sectors (CdIo *obj, void *buf, lsn_t lsn, bool mode2raw, 
			       unsigned int num_sectors);
  
  /*!
    Set the arg "key" with "value" in the source device.
  */
  int cdio_set_arg (CdIo *obj, const char key[], const char value[]);
  
  /*!
    Return the size of the CD in logical block address (LBA) units.
  */
  uint32_t cdio_stat_size (CdIo *obj);

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
  bool cdio_have_nrg     (void);
  bool cdio_have_bincue  (void);

  /* Like above but uses the enumeration instead. */
  bool cdio_have_driver (driver_id_t driver_id);
  
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
  
  /*! cdrao CD routines. Source is the some sort of device.

     NULL is returned on error.
   */
  CdIo * cdio_open_cd (const char *cue_name);

  /*! cdrao BIN/CUE CD disk-image routines. Source is the .cue file

     NULL is returned on error.
   */
  CdIo * cdio_open_cue (const char *cue_name);

  /*! BSDI CD-reading routines. 
     NULL is returned on error.
  */
  CdIo * cdio_open_bsdi (const char *source_name);
  
  /*! BSDI CD-reading routines. 
     NULL is returned on error.
  */
  CdIo * cdio_open_freebsd (const char *source_name);
  
  /*! Linux CD-reading routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_linux (const char *source_name);
  
  /*! Solaris CD-reading routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_solaris (const char *soruce_name);
  
  /*! Nero CD disk-image routines. 
     NULL is returned on error.
   */
  CdIo * cdio_open_nrg (const char *source_name);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_H__ */
