/* -*- c -*-
    $Id: cdio.h,v 1.75 2005/01/09 16:07:46 rocky Exp $

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
#define CDIO_API_VERSION 4

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
    Get the value associatied with key. 

    @param p_cdio the CD object queried
    @param key the key to retrieve
    @return the value associatd with "key" or NULL if p_cdio is NULL
    or "key" does not exist.
  */
  const char * cdio_get_arg (const CdIo_t *p_cdio,  const char key[]);

  /*!
    Set the arg "key" with "value" in "p_cdio".

    @param p_cdio the CD object to set
    @param key the key to set
    @param value the value to assocaiate with key
    @return 0 if no error was found, and nonzero otherwise.
  */
  int cdio_set_arg (CdIo_t *p_cdio, const char key[], const char value[]);
  
  /*!
    Initialize CD Reading and control routines. Should be called first.
  */
  bool cdio_init(void);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

/* Sector (frame, or block)-related functions. */
#include <cdio/read.h>

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
