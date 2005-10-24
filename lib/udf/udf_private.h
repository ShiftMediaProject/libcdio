/*
    $Id: udf_private.h,v 1.1 2005/10/24 10:14:58 rocky Exp $

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

#include <cdio/types.h>
#include <cdio/ecma_167.h>
#include <cdio/udf.h>
#include "_cdio_stdio.h"

/* Implementation of opaque types */

struct udf_s {
  bool          b_stream;         /* Use stream pointer, else use 
				    p_cdio.
				  */
  CdioDataSource_t      *stream;  /* Stream pointer if stream */
  CdIo_t                *cdio;    /* Cdio pointer if read device */
  anchor_vol_desc_ptr_t anchor_vol_desc_ptr;
  uint32_t              pvd_lba;  /* sector of Primary Volume Descriptor */
  partition_num_t       i_partition;  /* partition number */
  uint32_t              i_part_start; /* start of Partition Descriptor */
  uint32_t              lvd_lba;      /* sector of Logical Volume Descriptor */
  uint32_t              fsd_offset;   /* lba of fileset descriptor */
};

struct udf_file_s
{
  char	            *psz_name;
  bool	             b_dir;    /* true if this entry is a directory. */
  bool               b_parent; /* True if has parent directory (e.g. not root
				  directory). If not set b_dir will probably
				  be true. */

  uint32_t           i_part_start;
  uint32_t           dir_lba, dir_end_lba;
  uint64_t           dir_left;
  uint8_t           *sector;
  udf_fileid_desc_t *fid;
};

