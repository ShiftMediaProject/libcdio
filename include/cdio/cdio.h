/* -*- c -*-
    $Id: cdio.h,v 1.73 2005/01/04 10:58:03 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file cdio.h 
 *  \brief  The top-level header for libcdio: the CD Input and Control library.
 */


#ifndef __CDIO_H__
#define __CDIO_H__

/** Application Interface or Protocol version number. If the public
 *  interface changes, we increase this number.
 */
#define CDIO_API_VERSION 3

#include <cdio/version.h>

#ifdef  HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef  HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cdio/types.h>
#include <cdio/sector.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* For compatability. */
#define CdIo CdIo_t
    
  /** This is an opaque structure for the CD object. */
  typedef struct _CdIo CdIo_t; 

  /** This is an opaque structure for the CD-Text object. */
  typedef struct cdtext cdtext_t;

  /*!
    Free any resources associated with p_cdio. Call this when done using p_cdio
    and using CD reading/control operations.

    @param p_cdio the CD object to eliminated.
   */
  void cdio_destroy (CdIo_t *p_cdio);

  /*!
    Get the value associatied with key. 

    @param p_cdio the CD object queried
    @param key the key to retrieve
    @return the value associatd with "key" or NULL if p_cdio is NULL
    or "key" does not exist.
  */
  const char * cdio_get_arg (const CdIo_t *p_cdio,  const char key[]);

  off_t cdio_lseek(const CdIo_t *p_cdio, off_t offset, int whence);
    
  /*!
    Reads into buf the next size bytes.
    Similar to (if not the same as) libc's read()

    @return (ssize_t) -1 on error. 
  */
  ssize_t cdio_read(const CdIo_t *p_cdio, void *buf, size_t size);
    
  /*!
    Read an audio sector

    @param p_cdio object to read from
    @param buf place to read data into
    @param lsn sector to read

    @return 0 if no error, nonzero otherwise.
  */
  int cdio_read_audio_sector (const CdIo_t *p_cdio, void *buf, lsn_t lsn);

  /*!
    Reads audio sectors

    @param p_cdio object to read from
    @param buf place to read data into
    @param lsn sector to read
    @param i_sectors number of sectors to read

    @return 0 if no error, nonzero otherwise.
  */
  int cdio_read_audio_sectors (const CdIo_t *p_cdio, void *buf, lsn_t lsn,
			       unsigned int i_sectors);

  /*!
    Reads a mode 1 sector

    @param p_cdio object to read from
    @param buf place to read data into
    @param lsn sector to read
    @param b_form2 true for reading mode1 form 2 sectors or false for 
    mode 1 form 1 sectors.

    @return 0 if no error, nonzero otherwise.
  */
  int cdio_read_mode1_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
			      bool b_form2);
  
  /*!
    Reads mode 1 sectors

    @param p_cdio object to read from
    @param buf place to read data into
    @param lsn sector to read
    @param b_form2 true for reading mode 1 form 2 sectors or false for 
    mode 1 form 1 sectors.
    @param i_sectors number of sectors to read

    @return 0 if no error, nonzero otherwise.
  */
  int cdio_read_mode1_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
			       bool b_form2, unsigned int i_sectors);
  
  /*!
    Reads a mode 2 sector

    @param p_cdio object to read from
    @param buf place to read data into
    @param lsn sector to read
    @param b_form2 true for reading mode 2 form 2 sectors or false for 
    mode 2 form 1 sectors.

    @return 0 if no error, nonzero otherwise.
  */
  int cdio_read_mode2_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
			      bool b_form2);
  
  /*!
    Reads mode 2 sectors

    @param p_cdio object to read from
    @param buf place to read data into
    @param lsn sector to read
    @param b_form2 true for reading mode2 form 2 sectors or false for 
    mode 2  form 1 sectors.
    @param i_sectors number of sectors to read

    @return 0 if no error, nonzero otherwise.
  */
  int cdio_read_mode2_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
			       bool b_form2, unsigned int i_sectors);
  
  /*!
    Set the arg "key" with "value" in "p_cdio".

    @param p_cdio the CD object to set
    @param key the key to set
    @param value the value to assocaiate with key
    @return 0 if no error was found, and nonzero otherwise.
  */
  int cdio_set_arg (CdIo_t *p_cdio, const char key[], const char value[]);
  
  /*!
    Get the size of the CD in logical block address (LBA) units.

    @param p_cdio the CD object queried
    @return the size
  */
  uint32_t cdio_stat_size (const CdIo_t *p_cdio);
  
  /*!
    Initialize CD Reading and control routines. Should be called first.
  */
  bool cdio_init(void);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* CD-Text-related functions. */
#include <cdio/cdtext.h>

/* Track-related functions. */
#include <cdio/track.h>

/* Disc-related functions. */
#include <cdio/disc.h>

/* Drive(r)/Device-related functions. Perhaps we should break out 
   Driver from device?
*/
#include <cdio/device.h>

#endif /* __CDIO_H__ */
