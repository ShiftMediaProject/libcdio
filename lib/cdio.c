/*
    $Id: cdio.c,v 1.7 2003/04/10 04:13:41 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "cdio_assert.h"
#include "util.h"
#include "logging.h"
#include "cdio_private.h"

static const char _rcsid[] = "$Id: cdio.c,v 1.7 2003/04/10 04:13:41 rocky Exp $";


const char *track_format2str[5] = 
  {
    "audio", "CD-i", "XA", "data", "error"
  };

/* The below array gives of the drivers that are currently available for 
   on a particular host. */

CdIo_driver_t CdIo_driver[MAX_DRIVER] = { {0} };

/* The last valid entry of Cdio_driver. 
   -1 or (CDIO_DRIVER_UNINIT) means uninitialzed. 
   -2 means some sort of error.
*/

#define CDIO_DRIVER_UNINIT -1
int CdIo_last_driver = CDIO_DRIVER_UNINIT;

static bool 
cdio_have_false(void)
{
  return false;
}


/* The below array gives all drivers that can possibly appear.
   on a particular host. */

CdIo_driver_t CdIo_all_drivers[MAX_DRIVER+1] = { 
  {DRIVER_UNKNOWN, 
   0,
   "Unknown", 
   "No driver",
   &cdio_have_false,
   NULL,
   NULL
  },

  {DRIVER_BSDI, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "BSDI",
   "BSDI ATAPI and SCSI driver",
   &cdio_have_bsdi,
   cdio_open_bsdi,
   &cdio_get_default_device_bsdi
  },

  {DRIVER_FREEBSD, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "FreeBSD",
   "FreeBSD driver",
   &cdio_have_freebsd,
   &cdio_open_freebsd,
   &cdio_get_default_device_freebsd
  },

  {DRIVER_LINUX, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK,
   "Linux", 
   "Linux ioctl and packet driver",
   &cdio_have_linux,
   &cdio_open_linux,
   &cdio_get_default_device_freebsd
  },

  {DRIVER_SOLARIS, 
   CDIO_SRC_IS_DEVICE_MASK|CDIO_SRC_IS_NATIVE_MASK|CDIO_SRC_IS_SCSI_MASK,
   "Solaris",
   "Solaris ATAPI and SCSI driver",
   &cdio_have_solaris,
   &cdio_open_solaris,
   &cdio_get_default_device_solaris
  },

  {DRIVER_BINCUE,
   CDIO_SRC_IS_DISK_IMAGE_MASK,
   "BIN/CUE",
   "bin/cuesheet disk image driver",
   &cdio_have_bincue,
   &cdio_open_bincue,
   &cdio_get_default_device_bincue
  },

  {DRIVER_NRG,
   CDIO_SRC_IS_DISK_IMAGE_MASK,
   "NRG",
   "Nero NRG disk image driver",
   &cdio_have_nrg,
   &cdio_open_nrg,
   &cdio_get_default_device_nrg
  }

};

/*!
  Eject media in CD drive if there is a routine to do so. 
  Return 0 if success and 1 for failure, and 2 if no routine.
 */
int
cdio_eject_media (const CdIo *obj)
{
  cdio_assert (obj != NULL);

  if (obj->op.eject_media) {
    return obj->op.eject_media (obj->user_data);
  } else {
    return 2;
  }
}

/*!
  Return a string containing the default CD device if none is specified.
 */
const char *
cdio_get_arg (const CdIo *obj, const char key[])
{
  cdio_assert (obj != NULL);

  if (obj->op.get_arg) {
    return obj->op.get_arg (obj->user_data, key);
  } else {
    return NULL;
  }
}

/*!
  Return a string containing the default CD device if none is specified.
  if CdIo is NULL (we haven't initialized a specific device driver), 
  then find a suitable one and return the default device for that.

  NULL is returned if we couldn't get a default device.
 */
char *
cdio_get_default_device (const CdIo *obj)
{
  if (obj == NULL) {
    driver_id_t driver_id;
    /* Scan for driver */
    for (driver_id=DRIVER_UNKNOWN; driver_id<=MAX_DRIVER; driver_id++) {
      if ( (*CdIo_all_drivers[driver_id].have_driver)() &&
           *CdIo_all_drivers[driver_id].get_default_device ) {
        return (*CdIo_all_drivers[driver_id].get_default_device)();
      }
    }
    return NULL;
  }
  
  if (obj->op.get_default_device) {
    return obj->op.get_default_device ();
  } else {
    return NULL;
  }
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
track_t
cdio_get_first_track_num (const CdIo *obj)
{
  cdio_assert (obj != NULL);

  if (obj->op.get_first_track_num) {
    return obj->op.get_first_track_num (obj->user_data);
  } else {
    return CDIO_INVALID_TRACK;
  }
}

/*! 
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
track_t
cdio_get_num_tracks (const CdIo *obj)
{
  if (obj == NULL) return CDIO_INVALID_TRACK;

  if (obj->op.get_num_tracks) {
    return obj->op.get_num_tracks (obj->user_data);
  } else {
    return CDIO_INVALID_TRACK;
  }
}

/*!  
  Get format of track. 
*/
track_format_t
cdio_get_track_format(const CdIo *obj, track_t track_num)
{
  cdio_assert (obj != NULL);

  if (obj->op.get_track_format) {
    return obj->op.get_track_format (obj->user_data, track_num);
  } else {
    return TRACK_FORMAT_ERROR;
  }
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
bool
cdio_get_track_green(const CdIo *obj, track_t track_num)
{
  cdio_assert (obj != NULL);

  if (obj->op.get_track_green) {
    return obj->op.get_track_green (obj->user_data, track_num);
  } else {
    return false;
  }
}

/*!  
  Return the starting LBA for track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  CDIO_INVALID_LBA is returned on error.
*/
lba_t
cdio_get_track_lba(const CdIo *obj, track_t track_num)
{
  if (obj == NULL) return CDIO_INVALID_LBA;

  if (obj->op.get_track_lba) {
    return obj->op.get_track_lba (obj->user_data, track_num);
  } else {
    msf_t msf;
    if (cdio_get_track_msf(obj, track_num, &msf))
      return cdio_msf_to_lba(&msf);
    return CDIO_INVALID_LBA;
  }
}

/*!  
  Return the starting LSN for track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  CDIO_INVALID_LBA is returned on error.
*/
lsn_t
cdio_get_track_lsn(const CdIo *obj, track_t track_num)
{
  if (obj == NULL) return CDIO_INVALID_LBA;

  if (obj->op.get_track_lba) {
    return cdio_lba_to_lsn(obj->op.get_track_lba (obj->user_data, track_num));
  } else {
    msf_t msf;
    if (cdio_get_track_msf(obj, track_num, &msf))
      return cdio_msf_to_lsn(&msf);
    return CDIO_INVALID_LSN;
  }
}

/*!  
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
bool
cdio_get_track_msf(const CdIo *obj, track_t track_num, /*out*/ msf_t *msf)
{
  cdio_assert (obj != NULL);

  if (obj->op.get_track_msf) {
    return obj->op.get_track_msf (obj->user_data, track_num, msf);
  } else {
    return false;
  }
}

/*!  
  Return the number of sectors between this track an the next.  This
  includes any pregap sectors before the start of the next track.
  Tracks start at 1.
  0 is returned if there is an error.
*/
unsigned int
cdio_get_track_sec_count(const CdIo *obj, track_t track_num)
{
  track_t num_tracks = cdio_get_num_tracks(obj);

  if (track_num >=1 && track_num <= num_tracks) 
    return ( cdio_get_track_lba(obj, track_num+1) 
             - cdio_get_track_lba(obj, track_num) );
  return 0;
}

bool
cdio_have_driver(driver_id_t driver_id)
{
  return (*CdIo_all_drivers[driver_id].have_driver)();
}


/*!
  Initialize CD Reading and control routines. Should be called first.
  May be implicitly called by other routines if not called first.
*/
bool
cdio_init(void)
{
  
  CdIo_driver_t *all_dp;
  CdIo_driver_t *dp = CdIo_driver;
  driver_id_t driver_id;

  if (CdIo_last_driver != CDIO_DRIVER_UNINIT) {
    cdio_warn ("Init routine called more than once.");
    return false;
  }

  for (driver_id=DRIVER_UNKNOWN; driver_id<=MAX_DRIVER; driver_id++) {
    all_dp = &CdIo_all_drivers[driver_id];
    if ((*CdIo_all_drivers[driver_id].have_driver)()) {
      *dp++ = *all_dp;
      CdIo_last_driver++;
    }
  }

  return true;
}

CdIo *
cdio_new (void *user_data, const cdio_funcs *funcs)
{
  CdIo *new_obj;

  new_obj = _cdio_malloc (sizeof (CdIo));

  new_obj->user_data = user_data;
  new_obj->op = *funcs;

  return new_obj;
}

/*!
  Free any resources associated with obj.
*/
void
cdio_destroy (CdIo *obj)
{
  cdio_assert (obj != NULL);
  
  obj->op.free (obj->user_data);
  free (obj);
  CdIo_last_driver = CDIO_DRIVER_UNINIT;
}

/*!
  lseek - reposition read/write file offset
  Returns (off_t) -1 on error. 
  Similar to (if not the same as) libc's lseek()
*/
off_t
cdio_lseek (CdIo *obj, off_t offset, int whence)
{
  if (obj == NULL) return -1;
  
  if (obj->op.lseek)
    return obj->op.lseek (obj->user_data, offset, whence);
  return -1;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Similar to (if not the same as) libc's read()
*/
ssize_t
cdio_read (CdIo *obj, void *buf, size_t size)
{
  if (obj == NULL) return -1;
  
  if (obj->op.read)
    return obj->op.read (obj->user_data, buf, size);
  return -1;
}

int
cdio_read_audio_sector (CdIo *obj, void *buf, lsn_t lsn) 
{
  cdio_assert (obj != NULL);
  cdio_assert (buf != NULL);

  if  (obj->op.read_audio_sector != NULL)
    return obj->op.read_audio_sector (obj->user_data, buf, lsn);
  return -1;
}

int
cdio_read_mode2_sectors (CdIo *obj, void *buf, lsn_t lsn, bool mode2raw, 
                         unsigned num_sectors)
{
  cdio_assert (obj != NULL);
  cdio_assert (buf != NULL);
  cdio_assert (obj->op.read_mode2_sectors != NULL);
  
  return obj->op.read_mode2_sectors (obj->user_data, buf, lsn,
                                     mode2raw, num_sectors);
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int
cdio_read_mode2_sector (CdIo *obj, void *buf, uint32_t lsn, bool mode2raw)
{
  cdio_assert (obj != NULL);
  cdio_assert (buf != NULL);
  cdio_assert (obj->op.read_mode2_sector != NULL 
	      || obj->op.read_mode2_sectors != NULL);

  if (obj->op.read_mode2_sector)
    return obj->op.read_mode2_sector (obj->user_data, buf, lsn, mode2raw);

  /* fallback */
  if (obj->op.read_mode2_sectors != NULL)
    return cdio_read_mode2_sectors (obj, buf, lsn, mode2raw, 1);
  return 1;
}

uint32_t
cdio_stat_size (CdIo *obj)
{
  cdio_assert (obj != NULL);

  return obj->op.stat_size (obj->user_data);
}

/*!
  Set the arg "key" with "value" in the source device.
*/
int
cdio_set_arg (CdIo *obj, const char key[], const char value[])
{
  cdio_assert (obj != NULL);
  cdio_assert (obj->op.set_arg != NULL);
  cdio_assert (key != NULL);

  return obj->op.set_arg (obj->user_data, key, value);
}

static CdIo *
scan_for_driver(driver_id_t start, driver_id_t end, const char *source_name) 
{
  driver_id_t driver_id;
  
  for (driver_id=start; driver_id<=end; driver_id++) {
    if ((*CdIo_all_drivers[driver_id].have_driver)()) {
      CdIo *ret=(*CdIo_all_drivers[driver_id].driver_open)(source_name);
      if (ret != NULL) return ret;
    }
  }
  return NULL;
}

/*! Sets up to read from place specified by source_name and
  driver_id This should be called before using any other routine,
  except cdio_init. This will call cdio_init, if that hasn't been
  done previously.  to call one of the specific routines below. 
  
  NULL is returned on error.
*/
/* In the future we'll have more complicated code to allow selection
   of an I/O routine as well as code to find an appropriate default
   routine among the "registered" routines. Possibly classes too
   disk-based, SCSI-based, native-based, vendor (e.g. Sony, or
   Plextor) based 

   For now though, we'll start more simply...
*/
CdIo *
cdio_open (const char *source_name, driver_id_t driver_id)
{
  if (CdIo_last_driver == -1) cdio_init();

  switch (driver_id) {
  case DRIVER_UNKNOWN:
  case DRIVER_DEVICE: 
    {  
      /* Scan for a driver. */
      CdIo *ret = scan_for_driver(DRIVER_UNKNOWN, MAX_DRIVER, source_name);
      return ret;
    }
    break;
  case DRIVER_BSDI:
  case DRIVER_FREEBSD:
  case DRIVER_LINUX:
  case DRIVER_SOLARIS:
  case DRIVER_NRG:
  case DRIVER_BINCUE:
    if ((*CdIo_all_drivers[driver_id].have_driver)()) {
      return (*CdIo_all_drivers[driver_id].driver_open)(source_name);
    }
  }
  
  return NULL;
}


/* In the future we'll have more complicated code to allow selection
   of an I/O routine as well as code to find an appropriate default
   routine among the "registered" routines. Possibly classes too
   disk-based, SCSI-based, native-based, vendor (e.g. Sony, or
   Plextor) based 

   For now though, we'll start more simply...
*/
CdIo *
cdio_open_cd (const char *source_name)
{
  if (CdIo_last_driver == -1) cdio_init();

  /* Scan for a driver. */
  return scan_for_driver(DRIVER_UNKNOWN, MAX_DRIVER, source_name);
}



/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
