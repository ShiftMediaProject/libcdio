/*
    $Id: iso9660.h,v 1.21 2003/09/10 08:39:00 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

    See also iso9660.h by Eric Youngdale (1993).

    Copyright 1993 Yggdrasil Computing, Incorporated
    Copyright (c) 1999,2000 J. Schilling

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
/*
 * Header file iso9660.h - assorted structure definitions and typecasts.
  specific to iso9660 filesystem.
*/


#ifndef __CDIO_ISO9660_H__
#define __CDIO_ISO9660_H__

#include <cdio/cdio.h>
#include <cdio/xa.h>

#include <time.h>

#define MIN_TRACK_SIZE 4*75
#define MIN_ISO_SIZE MIN_TRACK_SIZE

/*
   A ISO filename is: "abcde.eee;1" -> <filename> '.' <ext> ';' <version #>
  
    The maximum needed string length is:
  	 30 chars (filename + ext)
    +	  2 chars ('.' + ';')
    +	   strlen("32767")
    +	   null byte
   ================================
    =	38 chars
*/
#define LEN_ISONAME     31
#define MAX_ISONAME     37

#define MAX_ISOPATHNAME 255

/*
 * ISO 9660 directory flags.
 */
#define	ISO_FILE	  0	/* Not really a flag...			*/
#define	ISO_EXISTENCE	  1	/* Do not make existence known (hidden)	*/
#define	ISO_DIRECTORY	  2	/* This file is a directory		*/
#define	ISO_ASSOCIATED	  4	/* This file is an assiciated file	*/
#define	ISO_RECORD	  8	/* Record format in extended attr. != 0	*/
#define	ISO_PROTECTION	 16	/* No read/execute perm. in ext. attr.	*/
#define	ISO_DRESERVED1	 32	/* Reserved bit 5			*/
#define	ISO_DRESERVED2	 64	/* Reserved bit 6			*/
#define	ISO_MULTIEXTENT	128	/* Not final entry of a mult. ext. file	*/

/* Volume descriptor types */
#define ISO_VD_PRIMARY             1
#define ISO_VD_SUPPLEMENTARY	   2  /* Used by Joliet */
#define ISO_VD_END	         255

#define ISO_PVD_SECTOR  16
#define ISO_EVD_SECTOR  17

#define ISO_STANDARD_ID      "CD001"
#define ISO_BLOCKSIZE           2048

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum strncpy_pad_check {
  ISO9660_NOCHECK = 0,
  ISO9660_7BIT,
  ISO9660_ACHARS,
  ISO9660_DCHARS
};

PRAGMA_BEGIN_PACKED

/* ISO-9660 Primary Volume Descriptor.
 */
struct iso9660_pvd {
  uint8_t  type;                      /* 711 */
  char     id[5];
  uint8_t  version;                   /* 711 */
  char     unused1[1];
  char     system_id[32];             /* achars */
  char     volume_id[32];             /* dchars */
  char     unused2[8];
  uint64_t volume_space_size;         /* 733 */
  char     escape_sequences[32];
  uint32_t volume_set_size;           /* 723 */
  uint32_t volume_sequence_number;    /* 723 */
  uint32_t logical_block_size;        /* 723 */
  uint64_t path_table_size;           /* 733 */
  uint32_t type_l_path_table;         /* 731 */
  uint32_t opt_type_l_path_table;     /* 731 */
  uint32_t type_m_path_table;         /* 732 */
  uint32_t opt_type_m_path_table;     /* 732 */
  char     root_directory_record[34]; /* 9.1 */
  char     volume_set_id[128];        /* dchars */
  char     publisher_id[128];         /* achars */
  char     preparer_id[128];          /* achars */
  char     application_id[128];       /* achars */
  char     copyright_file_id[37];     /* 7.5 dchars */
  char     abstract_file_id[37];      /* 7.5 dchars */
  char     bibliographic_file_id[37]; /* 7.5 dchars */
  char     creation_date[17];         /* 8.4.26.1 */
  char     modification_date[17];     /* 8.4.26.1 */
  char     expiration_date[17];       /* 8.4.26.1 */
  char     effective_date[17];        /* 8.4.26.1 */
  uint8_t  file_structure_version;    /* 711 */
  char     unused4[1];
  char     application_data[512];
  char     unused5[653];
} GNUC_PACKED;

typedef struct iso9660_dir  iso9660_dir_t;
typedef struct iso9660_pvd  iso9660_pvd_t;
typedef struct iso9660_stat iso9660_stat_t;

#ifndef  EMPTY_ARRAY_SIZE
#define EMPTY_ARRAY_SIZE 0
#endif

/*
 * XXX JS: The next structure may have an odd length!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See mkisofs.h
 */

/* Format of an ISO-9660 directory record */
struct iso9660_dir {
  uint8_t  length;                    /* 711 */
  uint8_t  ext_attr_length;           /* 711 */
  uint64_t extent;                    /* 733 */
  uint64_t size;                      /* 733 */
  uint8_t  date[7];                   /* 7 by 711 */
  uint8_t  flags;
  uint8_t  file_unit_size;            /* 711 */
  uint8_t  interleave;                /* 711 */
  uint32_t volume_sequence_number;    /* 723 */
  uint8_t  name_len;                  /* 711 */
  char     name[EMPTY_ARRAY_SIZE];
} GNUC_PACKED;

struct iso9660_stat { /* big endian!! */
  enum { _STAT_FILE = 1, _STAT_DIR = 2 } type;
  lsn_t        lsn;                   /* start logical sector number */
  uint32_t     size;                  /* total size in bytes */
  uint32_t     secsize;               /* number of sectors allocated */
  iso9660_xa_t xa;                    /* XA attributes */
  struct tm    tm;                    /* time on entry */
} ;

PRAGMA_END_PACKED

/*====================================
  Character file/dirname's 
=====================================*/

/*!
   Return true if c is a DCHAR - a character that can appear in an an
   ISO-9600 level 1 directory name. These are the ASCII capital
   letters A-Z, the digits 0-9 and an underscore.
*/
bool iso9660_isdchar (int c);

/*!
   Return true if c is an ACHAR - 
   These are the DCHAR's plus some ASCII symbols including the space 
   symbol.   
*/
bool iso9660_isachar (int c);

/*!
   Convert ISO-9660 file name that stored in a directory entry into 
   what's usually listed as the file name in a listing.
   Lowercase name, and drop deal with trailing ;1's or .;1's or 
   ; version numbers.

   The length of the translated string is returned.
*/
int iso9660_name_translate(const char *old, char *new);

/*!  
  Pad string src with spaces to size len and copy this to dst. If
  len is less than the length of src, dst will be truncated to the
  first len characters of src.

  src can also be scanned to see if it contains only ACHARs, DCHARs, 
  7-bit ASCII chars depending on the enumeration _check.

  In addition to getting changed, dst is the return value.
  Note: this string might not be NULL terminated.
 */
char *iso9660_strncpy_pad(char dst[], const char src[], size_t len, 
                          enum strncpy_pad_check _check);

/*!
  Set time in format used in ISO 9660 directory index record
  from a Unix time structure. */
void iso9660_set_time (const struct tm *tm, /*out*/ uint8_t idr_date[]);


/*!
  Get time structure from structure in an ISO 9660 directory index 
  record. Even though tm_wday and tm_yday fields are not explicitly in
  idr_date, the are calculated from the other fields.
*/
void iso9660_get_time (const uint8_t idr_date[], /*out*/ struct tm *tm);


/*=====================================================================
  file/dirname's 
======================================================================*/

/*!
  Check that pathname is a valid ISO-9660 directory name.

  A valid directory name should not start out with a slash (/), 
  dot (.) or null byte, should be less than 37 characters long, 
  have no more than 8 characters in a directory component 
  which is separated by a /, and consist of only DCHARs. 

  True is returned if pathname is valid.
 */
bool iso9660_dirname_valid_p (const char pathname[]);

/*!  
  Take pathname and a version number and turn that into a ISO-9660
  pathname.  (That's just the pathname followd by ";" and the version
  number. For example, mydir/file.ext -> mydir/file.ext;1 for version
  1. The resulting ISO-9660 pathname is returned.
*/
char *iso9660_pathname_isofy (const char pathname[], uint16_t version);

/*!
  Check that pathname is a valid ISO-9660 pathname.  

  A valid pathname contains a valid directory name, if one appears and
  the filename portion should be no more than 8 characters for the
  file prefix and 3 characters in the extension (or portion after a
  dot). There should be exactly one dot somewhere in the filename
  portion and the filename should be composed of only DCHARs.
  
  True is returned if pathname is valid.
 */
bool iso9660_pathname_valid_p (const char pathname[]);

/*=====================================================================
  directory tree 
======================================================================*/

void
iso9660_dir_init_new (void *dir, uint32_t self, uint32_t ssize, 
                      uint32_t parent, uint32_t psize, 
                      const time_t *dir_time);

void
iso9660_dir_init_new_su (void *dir, uint32_t self, uint32_t ssize, 
                         const void *ssu_data, unsigned int ssu_size, 
                         uint32_t parent, uint32_t psize, 
                         const void *psu_data, unsigned int psu_size,
                         const time_t *dir_time);

void
iso9660_dir_add_entry_su (void *dir, const char name[], uint32_t extent,
                          uint32_t size, uint8_t flags, const void *su_data,
                          unsigned int su_size, const time_t *entry_time);

unsigned int 
iso9660_dir_calc_record_size (unsigned int namelen, unsigned int su_len);

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it in stat. 

   Returns true if we found an entry with the lsn and false if not.
 */
bool iso9660_find_fs_lsn(const CdIo *cdio, lsn_t lsn, 
                         /*out*/ iso9660_stat_t *stat);

/*!
  Get file status for pathname into stat. As with libc's stat, 0 is returned
  if no error and -1 on error.
 */
int iso9660_fs_stat (const CdIo *obj, const char pathname[], 
                     /*out*/ iso9660_stat_t *stat, bool is_mode2);

void * /* list of char* -- caller must free it */
iso9660_fs_readdir (const CdIo *obj, const char pathname[], bool mode2);

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

/* volume descriptors */

void
iso9660_set_pvd (void *pd, const char volume_id[], const char application_id[],
                 const char publisher_id[], const char preparer_id[],
                 uint32_t iso_size, const void *root_dir, 
                 uint32_t path_table_l_extent, uint32_t path_table_m_extent,
                 uint32_t path_table_size, const time_t *pvd_time);

void 
iso9660_set_evd (void *pd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_ISO9660_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
