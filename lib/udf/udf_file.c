/*
    $Id: udf_file.c,v 1.5 2005/11/02 03:49:15 rocky Exp $

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
/* Access routines */

#include <cdio/bytesex.h>
#include "udf_private.h"

#ifdef HAVE_STRING_H
# include <string.h>
#endif

const char *
udf_get_filename(const udf_dirent_t *p_udf_dirent)
{
  if (!p_udf_dirent) return NULL;
  if (!p_udf_dirent->psz_name) return ".";
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
