/*  
    $Id: udf.h,v 1.3 2005/10/19 06:55:55 rocky Exp $
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
#define UDF_H 

#include <cdio/ecma_167.h>

/* FIXME: these probably don't go here. */
typedef uint16_t unicode16_t;
typedef uint8_t  ubyte;

typedef struct
{
  char	           *psz_name;
  bool	            b_dir;   /* true if this entry is a directory. */
  bool              b_root;  /* True if root directory, so b_dir should be
			        true if this is true. */

  uint32_t          i_part_start;
  uint32_t          dir_lba, dir_end_lba;
} udf_file_t;

/** This is an opaque structure. */
typedef struct udf_s udf_t; 

extern enum udf_enum1_s {
  UDF_BLOCKSIZE       = 2048
} udf_enums1;

/*!
  Seek to a position i_start and then read i_blocks. Number of blocks read is 
  returned. One normally expects the return to be equal to i_blocks.
*/
long int udf_read_sectors (const udf_t *p_udf, void *ptr, lsn_t i_start, 
			   long int i_blocks);

/*!
  Open an UDF for reading. Maybe in the future we will have
  a mode. NULL is returned on error.

  Caller must free result - use udf_close for that.
*/
udf_t *udf_open (const char *psz_path);

/*!
  Close UDF and free resources associated with p_udf.
*/
bool udf_close (udf_t *p_udf);

/*!
  free free resources associated with p_fe.
*/
bool udf_file_free(udf_file_t * p_fe);


#endif /*UDF_H*/
