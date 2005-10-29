/*
    $Id: udf.c,v 1.5 2005/10/29 14:52:47 rocky Exp $

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

/** The below variables are trickery to force enum symbol values to be
    recorded in debug symbol tables. They are used to allow one to refer
    to the enumeration value names in the typedefs above in a debugger
    and debugger expressions
*/
tag_id_t debug_tagid;
file_characteristics_t debug_file_characteristics;
udf_enum1_t debug_udf_enum1;
ecma_167_enum1_t ecma167_enum1;
ecma_167_timezone_enum_t debug_ecma_167_timezone_enum;

/*!
  Returns a string which interpreting the extended attribute permissions
*/
const char *
udf_get_attr_str(udf_Uint32_t permissions, char *result) 
{
  uint32_t i_perms = uint32_from_le(permissions);

  result[ 0] = (i_perms & FE_PERM_U_READ)  ? 'r' : '-';
  result[ 1] = (i_perms & FE_PERM_U_WRITE) ? 'w' : '-';
  result[ 2] = (i_perms & FE_PERM_U_EXEC)  ? 'x' : '-';

  result[ 3] = (i_perms & FE_PERM_G_READ)  ? 'r' : '-';
  result[ 4] = (i_perms & FE_PERM_G_WRITE) ? 'w' : '-';
  result[ 5] = (i_perms & FE_PERM_G_EXEC)  ? 'x' : '-';

  result[ 6] = (i_perms & FE_PERM_O_READ)  ? 'r' : '-';
  result[ 7] = (i_perms & FE_PERM_O_WRITE) ? 'w' : '-';
  result[ 8] = (i_perms & FE_PERM_O_EXEC)  ? 'x' : '-';

  result[ 9] = '\0';
  return result;
}

const char *
udf_get_filename(const udf_file_t *p_udf_file)
{
  if (!p_udf_file) return NULL;
  return p_udf_file->psz_name;
}

/*!  
  Returns a POSIX filemode string for a given p_udf_file.
*/
const char *
udf_get_posix_filemode_str(const udf_file_t *p_udf_file, char perms[]) 
{
  udf_file_entry_t udf_fe;

  if (p_udf_file->b_dir) {
    perms[0] = 'd';
  } else {
    perms[0] = '-';
  }
  
  if (udf_get_file_entry(p_udf_file, &udf_fe)) {
    /* Print directory attributes*/
    udf_get_attr_str (udf_fe.permissions, &perms[1]);
    return perms;
  }
  return "";
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
  Return the partition number of the file
*/
int16_t udf_get_part_number(const udf_t *p_udf)
{
  if (!p_udf) return -1;
  return p_udf->i_partition;
}

bool
udf_is_dir(const udf_file_t *p_udf_file)
{
  return p_udf_file->b_dir;
}
