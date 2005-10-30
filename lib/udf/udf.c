/*
    $Id: udf.c,v 1.6 2005/10/30 05:43:01 rocky Exp $

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

#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif

/** The below variables are trickery to force enum symbol values to be
    recorded in debug symbol tables. They are used to allow one to refer
    to the enumeration value names in the typedefs above in a debugger
    and debugger expressions
*/
tag_id_t debug_tagid;
file_characteristics_t debug_file_characteristics;
udf_enum1_t debug_udf_enum1;
icbtag_file_type_enum_t debug_icbtag_file_type_enum;
ecma_167_enum1_t ecma167_enum1;
ecma_167_timezone_enum_t debug_ecma_167_timezone_enum;

/*!
  Returns POSIX mode bitstring for a given file.
*/
mode_t 
udf_get_posix_filemode(const udf_file_t *p_udf_file) 
{
  udf_file_entry_t udf_fe;
  mode_t mode = 0;

  if (udf_get_file_entry(p_udf_file, &udf_fe)) {
    uint16_t i_flags;
    uint32_t i_perms;

    i_perms = uint32_from_le(udf_fe.permissions);
    i_flags = uint16_from_le(udf_fe.icb_tag.flags);

    if (i_perms & FE_PERM_U_READ)  mode |= S_IRUSR;
    if (i_perms & FE_PERM_U_WRITE) mode |= S_IWUSR;
    if (i_perms & FE_PERM_U_EXEC)  mode |= S_IXUSR;
    
    if (i_perms & FE_PERM_G_READ)  mode |= S_IRGRP;
    if (i_perms & FE_PERM_G_WRITE) mode |= S_IWGRP;
    if (i_perms & FE_PERM_G_EXEC)  mode |= S_IXGRP;
    
    if (i_perms & FE_PERM_O_READ)  mode |= S_IROTH;
    if (i_perms & FE_PERM_O_WRITE) mode |= S_IWOTH;
    if (i_perms & FE_PERM_O_EXEC)  mode |= S_IXOTH;

    switch (udf_fe.icb_tag.file_type) {
    case ICBTAG_FILE_TYPE_DIRECTORY: 
      mode |= S_IFDIR;
      break;
    case ICBTAG_FILE_TYPE_REGULAR:
      mode |= S_IFREG;
      break;
    case ICBTAG_FILE_TYPE_SYMLINK:
      mode |= S_IFLNK;
      break;
    case ICBTAG_FILE_TYPE_CHAR:
      mode |= S_IFCHR;
      break;
    case ICBTAG_FILE_TYPE_SOCKET:
      mode |= S_IFSOCK;
      break;
    case ICBTAG_FILE_TYPE_BLOCK:
      mode |= S_IFBLK;
      break;
    default: ;
    };
  
    if (i_flags & ICBTAG_FLAG_SETUID) mode |= S_ISUID;
    if (i_flags & ICBTAG_FLAG_SETGID) mode |= S_ISGID;
    if (i_flags & ICBTAG_FLAG_STICKY) mode |= S_ISVTX;
  }
  
  return mode;
  
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
