/*
    $Id: cdtext.h,v 1.4 2004/07/11 14:25:07 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
    adapted from cuetools
    Copyright (C) 2003 Svend Sanjay Sorensen <ssorensen@fastmail.fm>

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
/*!
 * \file cdtext.h 
 * \brief Header CDTEXT information
*/


#ifndef __CDIO_CDTEXT_H__
#define __CDIO_CDTEXT_H__

#include <cdio/cdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MAX_CDTEXT_FIELDS 13
  
  struct cdtext {
    char *field[MAX_CDTEXT_FIELDS];
  };
  
  typedef enum {
    CDTEXT_ARRANGER   =  0,
    CDTEXT_COMPOSER   =  1,
    CDTEXT_DISCID     =  2,
    CDTEXT_GENRE      =  3,
    CDTEXT_MESSAGE    =  4,
    CDTEXT_ISRC       =  5,
    CDTEXT_PERFORMER  =  6,
    CDTEXT_SIZE_INFO  =  7,
    CDTEXT_SONGWRITER =  8,
    CDTEXT_TITLE      =  9,
    CDTEXT_TOC_INFO   = 10,
    CDTEXT_TOC_INFO2  = 10,
    CDTEXT_UPC_EAN    = 12,
    CDTEXT_INVALID    = MAX_CDTEXT_FIELDS
  } cdtext_field_t;


/*! Initialize a new cdtext structure.
  When the structure is no longer needed, release the 
  resources using cdtext_delete.
*/
void cdtext_init (cdtext_t *cdtext);

/*! Free memory assocated with cdtext*/
void cdtext_destroy (cdtext_t *cdtext);

/*! 
  returns the CDTEXT value associated with key. NULL is returned
  if key is CDTEXT_INVALID or the field is not set.
 */
const char *cdtext_get (cdtext_field_t key, const cdtext_t *cdtext);

/*!
  returns enum of keyword if key is a CD-TEXT keyword, 
  returns MAX_CDTEXT_FIELDS non-zero otherwise.
*/
cdtext_field_t cdtext_is_keyword (const char *key);

/*! 
  sets cdtext's keyword entry to field 
 */
void cdtext_set (cdtext_field_t key, const char *value, cdtext_t *cdtext);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_CDTEXT_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
