/*
    $Id: iso9660.h,v 1.3 2003/08/31 01:32:05 rocky Exp $

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

/* Will eventually need/use glib.h */
#include <cdio/types.h>

#define MIN_TRACK_SIZE 4*75

#define MIN_ISO_SIZE MIN_TRACK_SIZE

#define ISO_BLOCKSIZE 2048

#define LEN_ISONAME     31
#define MAX_ISONAME     37

#define MAX_ISOPATHNAME 255

#define ISO_FILE        0       
#define ISO_VD_PRIMARY  1
#define ISO_DIRECTORY   2
#define ISO_STANDARD_ID         "CD001"


#define ISO_PVD_SECTOR  16
#define ISO_EVD_SECTOR  17

enum strncpy_pad_check {
  ISO9660_NOCHECK = 0,
  ISO9660_7BIT,
  ISO9660_ACHARS,
  ISO9660_DCHARS
};

  /* Opaque types ... */

  /* Defined fully in iso9660_private.h */
  typedef struct iso_primary_descriptor pvd_t;
  typedef struct iso_directory_record iso_directory_record_t;

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

uint8_t
iso9660_get_dir_len(const iso_directory_record_t *idr);

#if FIXME
uint8_t
iso9660_get_dir_size(const iso_directory_record_t *idr);

lsn_t
iso9660_get_dir_extent(const iso_directory_record_t *idr);#
#endif

uint8_t
iso9660_get_pvd_type(const pvd_t *pvd);

const char *
iso9660_get_pvd_id(const pvd_t *pvd);

int
iso9660_get_pvd_space_size(const pvd_t *pvd);

int
iso9660_get_pvd_block_size(const pvd_t *pvd) ;

int
iso9660_get_pvd_version(const pvd_t *pvd) ;

lsn_t
iso9660_get_root_lsn(const pvd_t *pvd);


#endif /* __CDIO_ISO9660_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
