/*
    $Id: udf.c,v 1.2 2005/10/27 01:23:48 rocky Exp $

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

  result[ 3] = (i_perms & FE_PERM_G_READ) ?  'r' : '-';
  result[ 4] = (i_perms & FE_PERM_G_WRITE) ? 'w' : '-';
  result[ 5] = (i_perms & FE_PERM_G_EXEC) ?  'x' : '-';

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

bool
udf_get_file_entry(const udf_file_t *p_udf_file, 
		   /*out*/ udf_file_entry_t *p_udf_fe)
{
  if (!p_udf_file) return false;
  memcpy(p_udf_fe, &p_udf_file->fe, sizeof(udf_file_entry_t));
  return true;
}

bool
udf_is_dir(const udf_file_t *p_udf_file)
{
  return p_udf_file->b_dir;
}


