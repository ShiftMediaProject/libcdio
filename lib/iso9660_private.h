/*
    $Id: iso9660_private.h,v 1.1 2003/08/17 05:31:19 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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

#ifndef __CDIO_ISO9660_PRIVATE_H__
#define __CDIO_ISO9660_PRIVATE_H__

#include <libvcd/types.h>

#define ISO_VD_END              255

#define ISO_VERSION             1

#define ISO_XA_MARKER_STRING    "CD-XA001"
#define ISO_XA_MARKER_OFFSET    1024

PRAGMA_BEGIN_PACKED

struct iso_volume_descriptor {
  uint8_t  type; /* 711 */
  char     id[5];
  uint8_t  version; /* 711 */
  char     data[2041];
} GNUC_PACKED;

#define struct_iso_volume_descriptor_SIZEOF ISO_BLOCKSIZE

struct iso_primary_descriptor {
  uint8_t  type; /* 711 */
  char     id[5];
  uint8_t  version; /* 711 */
  char     unused1[1];
  char     system_id[32]; /* achars */
  char     volume_id[32]; /* dchars */
  char     unused2[8];
  uint64_t volume_space_size; /* 733 */
  char     escape_sequences[32];
  uint32_t volume_set_size; /* 723 */
  uint32_t volume_sequence_number; /* 723 */
  uint32_t logical_block_size; /* 723 */
  uint64_t path_table_size; /* 733 */
  uint32_t type_l_path_table; /* 731 */
  uint32_t opt_type_l_path_table; /* 731 */
  uint32_t type_m_path_table; /* 732 */
  uint32_t opt_type_m_path_table; /* 732 */
  char     root_directory_record[34]; /* 9.1 */
  char     volume_set_id[128]; /* dchars */
  char     publisher_id[128]; /* achars */
  char     preparer_id[128]; /* achars */
  char     application_id[128]; /* achars */
  char     copyright_file_id[37]; /* 7.5 dchars */
  char     abstract_file_id[37]; /* 7.5 dchars */
  char     bibliographic_file_id[37]; /* 7.5 dchars */
  char     creation_date[17]; /* 8.4.26.1 */
  char     modification_date[17]; /* 8.4.26.1 */
  char     expiration_date[17]; /* 8.4.26.1 */
  char     effective_date[17]; /* 8.4.26.1 */
  uint8_t  file_structure_version; /* 711 */
  char     unused4[1];
  char     application_data[512];
  char     unused5[653];
} GNUC_PACKED;

#define struct_iso_primary_descriptor_SIZEOF ISO_BLOCKSIZE

struct iso_path_table {
  uint8_t  name_len; /* 711 */
  uint8_t  xa_len; /* 711 */
  uint32_t extent; /* 731/732 */
  uint16_t parent; /* 721/722 */
  char     name[EMPTY_ARRAY_SIZE];
} GNUC_PACKED;

#define struct_iso_path_table_SIZEOF 8

struct iso_directory_record {
  uint8_t  length; /* 711 */
  uint8_t  ext_attr_length; /* 711 */
  uint64_t extent; /* 733 */
  uint64_t size; /* 733 */
  uint8_t  date[7]; /* 7 by 711 */
  uint8_t  flags;
  uint8_t  file_unit_size; /* 711 */
  uint8_t  interleave; /* 711 */
  uint32_t volume_sequence_number; /* 723 */
  uint8_t  name_len; /* 711 */
  char     name[EMPTY_ARRAY_SIZE];
} GNUC_PACKED;

#define struct_iso_directory_record_SIZEOF 33

PRAGMA_END_PACKED

#endif /* __CDIO_ISO9660_PRIVATE_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
