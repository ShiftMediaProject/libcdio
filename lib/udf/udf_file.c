/*
    $Id: udf_file.c,v 1.11 2006/04/16 02:34:10 rocky Exp $

    Copyright (C) 2005, 2006 Rocky Bernstein <rockyb@users.sourceforge.net>

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
/* Access routines */

#include <cdio/bytesex.h>
#include "udf_private.h"
#include "udf_fs.h"

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#include <stdio.h>  /* Remove when adding cdio/logging.h */

#define MIN(a, b) (a<b) ? (a) : (b)

const char *
udf_get_filename(const udf_dirent_t *p_udf_dirent)
{
  if (!p_udf_dirent) return NULL;
  if (!p_udf_dirent->psz_name) return "..";
  return p_udf_dirent->psz_name;
}

bool
udf_get_file_entry(const udf_dirent_t *p_udf_dirent, 
		   /*out*/ udf_file_entry_t *p_udf_fe)
{
  if (!p_udf_dirent) return false;
  memcpy(p_udf_fe, &p_udf_dirent->fe, sizeof(udf_file_entry_t));
  return true;
}

/*!
  Return the file id descriptor of the given file.
*/
bool udf_get_fileid_descriptor(const udf_dirent_t *p_udf_dirent, 
			       /*out*/ udf_fileid_desc_t *p_udf_fid)
{
  
  if (!p_udf_dirent) return false;
  if (!p_udf_dirent->fid) {
    /* FIXME do something about trying to get the descriptor. */
    return false;
  }
  memcpy(p_udf_fid, p_udf_dirent->fid, sizeof(udf_fileid_desc_t));
  return true;
}


/*!
  Return the number of hard links of the file. Return 0 if error.
*/
uint16_t udf_get_link_count(const udf_dirent_t *p_udf_dirent) 
{
  if (p_udf_dirent) {
    return uint16_from_le(p_udf_dirent->fe.link_count);
  }
  return 0; /* Error. Non-error case handled above. */
}

/*!
  Return the file length the file. Return 2147483647L if error.
*/
uint64_t udf_get_file_length(const udf_dirent_t *p_udf_dirent) 
{
  if (p_udf_dirent) {
    return uint64_from_le(p_udf_dirent->fe.info_len);
  }
  return 2147483647L; /* Error. Non-error case handled above. */
}

/*!
  Return true if the file is a directory.
*/
bool
udf_is_dir(const udf_dirent_t *p_udf_dirent)
{
  return p_udf_dirent->b_dir;
}

/**
  Attempts to read up to count bytes from UDF directory entry
  p_udf_dirent into the buffer starting at buf. buf should be a
  multiple of UDF_BLOCKSIZE bytes. Reading continues after the point
  at which we last read or from the beginning the first time.

  If count is zero, read() returns zero and has no other results. If
  count is greater than SSIZE_MAX, the result is unspecified.

  If there is an error, cast the result to driver_return_code_t for 
  the specific error code.
*/
ssize_t
udf_read_block(const udf_dirent_t *p_udf_dirent, void * buf, size_t count)
{
  if (count == 0) return 0;
  else {
    /* FIXME this code seems a bit convoluted. */
    udf_t *p_udf = p_udf_dirent->p_udf;
    const udf_file_entry_t *p_udf_fe = (udf_file_entry_t *) p_udf_dirent->data;
    driver_return_code_t ret;
    const unsigned long int i_file_length = udf_get_file_length(p_udf_dirent);

    if (0 == p_udf->i_position) {
      ret = udf_read_sectors(p_udf, p_udf_dirent->data, 
			     p_udf_dirent->fe.unique_ID, 1);
      if (ret != DRIVER_OP_SUCCESS) return DRIVER_OP_ERROR;
    } 
    {
      if (!udf_checktag(&p_udf_fe->tag, TAGID_FILE_ENTRY)) {
	uint32_t i_lba_start, i_lba_end;
	udf_get_lba( p_udf_fe, &i_lba_start, &i_lba_end);

	/* set i_lba_start to position of where we last left off. */
	i_lba_start += (p_udf->i_position / UDF_BLOCKSIZE);
	
	if ( (i_lba_end - i_lba_start+1) < count ) {
	  printf("Warning: don't know how to handle yet\n" );
	  count = i_lba_end - i_lba_start+1;
	} else {
	  const uint32_t i_lba = p_udf->i_part_start+i_lba_start;
	  ret = udf_read_sectors(p_udf, buf, i_lba, count);
	  if (DRIVER_OP_SUCCESS == ret) {
	    ssize_t i_read_len = MIN(i_file_length, count * UDF_BLOCKSIZE);
	    p_udf->i_position += i_read_len;
	    return i_read_len;
	  }
	}
      }
    }
    return ret;
  }
}
