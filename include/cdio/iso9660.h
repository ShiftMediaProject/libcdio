/*
    $Id: iso9660.h,v 1.13 2003/08/31 14:26:06 rocky Exp $

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

#ifndef __CDIO_ISO9660_H__
#define __CDIO_ISO9660_H__

#include <cdio/cdio.h>
#include <cdio/xa.h>

#define MIN_TRACK_SIZE 4*75

#define MIN_ISO_SIZE MIN_TRACK_SIZE


#define LEN_ISONAME     31
#define MAX_ISONAME     37

#define MAX_ISOPATHNAME 255

#define ISO_FILE        0       
#define ISO_VD_PRIMARY  1
#define ISO_DIRECTORY   2
#define ISO_PVD_SECTOR  16
#define ISO_EVD_SECTOR  17

#define ISO_STANDARD_ID         "CD001"
#define ISO_BLOCKSIZE           2048

enum strncpy_pad_check {
  ISO9660_NOCHECK = 0,
  ISO9660_7BIT,
  ISO9660_ACHARS,
  ISO9660_DCHARS
};

typedef struct iso9660_pvd  iso9660_pvd_t;
typedef struct iso9660_dir  iso9660_dir_t;
typedef struct iso9660_stat iso9660_stat_t;

PRAGMA_BEGIN_PACKED

struct iso9660_pvd {
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

#ifndef  EMPTY_ARRAY_SIZE
#define EMPTY_ARRAY_SIZE 0
#endif

struct iso9660_dir {
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

struct iso9660_stat { /* big endian!! */
  enum { _STAT_FILE = 1, _STAT_DIR = 2 } type;
  lsn_t        lsn;     /* start logical sector number */
  uint32_t     size;    /* total size in bytes */
  uint32_t     secsize; /* number of sectors allocated */
  iso9660_xa_t xa;      /* XA attributes */
};

PRAGMA_END_PACKED

char *
iso9660_strncpy_pad(char dst[], const char src[], size_t len, 
                    enum strncpy_pad_check _check);

int
iso9660_isdchar (int c);

int
iso9660_isachar (int c);

/* file/dirname's */
bool
iso9660_pathname_valid_p (const char pathname[]);

bool
iso9660_dirname_valid_p (const char pathname[]);

char *
iso9660_pathname_isofy (const char pathname[], uint16_t version);

/* volume descriptors */

void
iso9660_set_pvd (void *pd, const char volume_id[], const char application_id[],
                 const char publisher_id[], const char preparer_id[],
                 uint32_t iso_size, const void *root_dir, 
                 uint32_t path_table_l_extent, uint32_t path_table_m_extent,
                 uint32_t path_table_size);

void 
iso9660_set_evd (void *pd);

/* directory tree */

void
iso9660_dir_init_new (void *dir, uint32_t self, uint32_t ssize, 
                      uint32_t parent, uint32_t psize);

void
iso9660_dir_init_new_su (void *dir, uint32_t self, uint32_t ssize, 
                         const void *ssu_data, unsigned ssu_size, 
                         uint32_t parent, uint32_t psize, 
                         const void *psu_data, unsigned psu_size);

void
iso9660_dir_add_entry_su (void *dir, const char name[], uint32_t extent,
                          uint32_t size, uint8_t flags, const void *su_data,
                          unsigned su_size);

unsigned
iso9660_dir_calc_record_size (unsigned namelen, unsigned int su_len);

int
iso9660_fs_stat (CdIo *obj,const char pathname[], iso9660_stat_t *buf,
                 bool is_mode2);

void * /* list of char* -- caller must free it */
iso9660_fs_readdir (CdIo *obj, const char pathname[], bool mode2);

uint8_t
iso9660_get_dir_len(const iso9660_dir_t *idr);

#if FIXME
uint8_t
iso9660_get_dir_size(const iso9660_dir_t *idr);

lsn_t
iso9660_get_dir_extent(const iso9660_dir_t *idr);
#endif

uint8_t
iso9660_get_pvd_type(const iso9660_pvd_t *pvd);

const char *
iso9660_get_pvd_id(const iso9660_pvd_t *pvd);

int
iso9660_get_pvd_space_size(const iso9660_pvd_t *pvd);

int
iso9660_get_pvd_block_size(const iso9660_pvd_t *pvd) ;

int
iso9660_get_pvd_version(const iso9660_pvd_t *pvd) ;

lsn_t
iso9660_get_root_lsn(const iso9660_pvd_t *pvd);

/* pathtable */

void
iso9660_pathtable_init (void *pt);

unsigned int
iso9660_pathtable_get_size (const void *pt);

uint16_t
iso9660_pathtable_l_add_entry (void *pt, const char name[], uint32_t extent,
                               uint16_t parent);

uint16_t
iso9660_pathtable_m_add_entry (void *pt, const char name[], uint32_t extent,
                       uint16_t parent);

#endif /* __CDIO_ISO9660_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
