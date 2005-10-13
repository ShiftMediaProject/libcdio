/*  
    $Id: udf.h,v 1.1 2005/10/13 02:39:43 rocky Exp $
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

/*!
 * \file udf.h 
 *
 * \brief The top-level interface header for libudf: the ISO-9660
 * filesystem library; applications include this.
 *
*/

#ifndef UDF_H
#define UDF__H 

#include <cdio/types.h>
typedef uint16_t unicode16_t;
typedef uint8_t  ubyte;

/** This is an opaque structure. */
typedef struct udf_s udf_t; 

extern enum udf_enum1_s {
  UDF_BLOCKSIZE       = 2048
} udf_enums1;

/*!
  Seek to a position and then read n blocks. Size read is returned.
*/
long int udf_seek_read (const udf_t *p_udf, void *ptr, lsn_t start, 
			long int size);

/*!
  Open an UDF image for reading. Maybe in the future we will have
  a mode. NULL is returned on error.
*/
udf_t *udf_open (const char *psz_path);

#endif /*UDF_H*/
