/*  
    $Id: udf_file.h,v 1.1 2005/10/30 07:36:15 rocky Exp $
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
 * \file udf_file.h 
 *
 * \brief Routines involving UDF file operations
 *
*/

#ifndef UDF_FILE_H
#define UDF_FILE_H 

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /*!
    Return the file id descriptor of the given file.
  */
  bool udf_get_fileid_descriptor(const udf_file_t *p_udf_file, 
				 /*out*/ udf_fileid_desc_t *p_udf_fid);

  /*!
    Return the name of the file
  */
  const char *udf_get_filename(const udf_file_t *p_udf_file);
  
  /*!
    Return the name of the file
  */
  bool udf_get_file_entry(const udf_file_t *p_udf_file, 
			  /*out*/ udf_file_entry_t *p_udf_fe);

  /*!
    Return the number of hard links of the file. Return 0 if error.
  */
  uint16_t udf_get_link_count(const udf_file_t *p_udf_file);

  /*!  
    Returns a POSIX mode for a given p_udf_file.
  */
  mode_t udf_get_posix_filemode(const udf_file_t *p_udf_file);

  /*!
    Return the next subdirectory. 
  */
  udf_file_t *udf_get_sub(const udf_t *p_udf, const udf_file_t *p_udf_file);
  
  /*!
    Return the next file.
  */
  udf_file_t *udf_get_next(const udf_t *p_udf, udf_file_t *p_udf_file);
  
  /*!
    free free resources associated with p_udf_file.
  */
  bool udf_file_free(udf_file_t *p_udf_file);
  
  /*!
    Return true if the file is a directory.
  */
  bool udf_is_dir(const udf_file_t *p_udf_file);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*UDF_FILE_H*/
