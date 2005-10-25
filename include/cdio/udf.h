/*  
    $Id: udf.h,v 1.8 2005/10/25 03:13:13 rocky Exp $
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
 * \brief The top-level interface header for libudf: UDF filesystem
 * library; applications include this.
 *
*/

#ifndef UDF_H
#define UDF_H 

#include <cdio/cdio.h>
#include <cdio/ecma_167.h>

typedef uint16_t partition_num_t;

/* FIXME: these probably don't go here. */
typedef uint16_t unicode16_t;
typedef uint8_t  ubyte;


/** Opaque structures. */
typedef struct udf_s udf_t; 
typedef struct udf_file_s udf_file_t;

/**
   Imagine the below a #define'd value rather than distinct values of
   an enum.
*/
typedef enum {
  UDF_BLOCKSIZE       = 2048
} udf_enum1_t; 

/** This variable is trickery to force the above enum symbol value to
    be recorded in debug symbol tables. It is used to allow one refer
    to above enumeration values in a debugger and debugger
    expressions */
extern udf_enum1_t debug_udf_enums1;

/*!
  Seek to a position i_start and then read i_blocks. Number of blocks read is 
  returned. One normally expects the return to be equal to i_blocks.
*/
driver_return_code_t udf_read_sectors (const udf_t *p_udf, void *ptr, 
				       lsn_t i_start,  long int i_blocks);

/*!
  Open an UDF for reading. Maybe in the future we will have
  a mode. NULL is returned on error.

  Caller must free result - use udf_close for that.
*/
udf_t *udf_open (const char *psz_path);

/*!
  Get the root in p_udf. If b_any_partition is false then
  the root must be in the given partition.
  NULL is returned if the partition is not found or a root is not found or
  there is on error.

  Caller must free result - use udf_file_free for that.
*/
udf_file_t *udf_get_root (udf_t *p_udf, bool b_any_partition, 
			  partition_num_t i_partition);

/**
 * Gets the Volume Identifier string, in 8bit unicode (latin-1)
 * psz_volid, place to put the string
 * i_volid_size, size of the buffer volid points to
 * returns the size of buffer needed for all data
 */
int udf_get_volume_id(udf_t *p_udf, /*out*/ char *psz_volid,  
		      unsigned int i_volid);

/*!
  Return a file pointer matching pzz_name. If b_any_partition is false then
  the root must be in the given partition. 
 */
udf_file_t *udf_find_file(udf_t *p_udf, const char *psz_name,
			  bool b_any_partition,
			  partition_num_t i_partition);

/*!
  Return the next subdirectory. 
 */
udf_file_t *udf_get_sub(const udf_t *p_udf, const udf_file_t *p_udf_file);

/*!
  Return the name of the file
 */
const char *udf_get_name(const udf_file_t *p_udf_file);

/*!
  Return the next file.
 */
udf_file_t *udf_get_next(const udf_t *p_udf, udf_file_t *p_udf_file);

/*!
  Close UDF and free resources associated with p_udf.
*/
bool udf_close (udf_t *p_udf);

/*!
  free free resources associated with p_fe.
*/
bool udf_file_free(udf_file_t *p_udf_file);

/*!
  Return true if the file is a directory.
 */
bool udf_is_dir(const udf_file_t *p_udf_file);


#endif /*UDF_H*/
