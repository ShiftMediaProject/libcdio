/*
    $Id: udf_file.c,v 1.1 2005/10/30 07:35:37 rocky Exp $

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
udf_get_filename(const udf_file_t *p_udf_file)
{
  if (!p_udf_file) return NULL;
  return p_udf_file->psz_name;
}

bool
udf_get_file_entry(const udf_file_t *p_udf_file, 
		   /*out*/ udf_file_entry_t *p_udf_fe)
{
  if (!p_udf_file) return false;
  memcpy(p_udf_fe, &p_udf_file->fe, sizeof(udf_file_entry_t));
  return true;
}

/*!
  Return the file id descriptor of the given file.
*/
bool udf_get_fileid_descriptor(const udf_file_t *p_udf_file, 
			       /*out*/ udf_fileid_desc_t *p_udf_fid)
{
  
  if (!p_udf_file) return false;
  if (!p_udf_file->fid) {
    /* FIXME do something about trying to get the descriptor. */
    return false;
  }
  memcpy(p_udf_fid, p_udf_file->fid, sizeof(udf_fileid_desc_t));
  return true;
}


/*!
  Return the number of hard links of the file. Return 0 if error.
*/
uint16_t udf_get_link_count(const udf_file_t *p_udf_file) 
{
  if (p_udf_file) {
    udf_file_entry_t udf_fe;
    if (udf_get_file_entry(p_udf_file, &udf_fe)) {
      return uint16_from_le(udf_fe.link_count);
    }
  }
  return 0; /* Error. Non-error case handled above. */
}

/*!
  Return true if the file is a directory.
*/
bool
udf_is_dir(const udf_file_t *p_udf_file)
{
  return p_udf_file->b_dir;
}
