/*
    $Id: iso9660.c,v 1.2 2003/08/31 01:01:40 rocky Exp $

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>

/* Public headers */
#include <cdio/iso9660.h>
#include <cdio/util.h>

/* Private headers */
#include "iso9660_private.h"
#include "cdio_assert.h"
#include "bytesex.h"

static const char _rcsid[] = "$Id: iso9660.c,v 1.2 2003/08/31 01:01:40 rocky Exp $";

/* some parameters... */
#define SYSTEM_ID         "CD-RTOS CD-BRIDGE"
#define VOLUME_SET_ID     ""

static void
pathtable_get_size_and_entries(const void *pt, unsigned *size, 
                               unsigned int *entries);

static void
_idr_set_time (uint8_t _idr_date[], const struct tm *_tm)
{
  memset (_idr_date, 0, 7);

  if (!_tm)
    return;

  _idr_date[0] = _tm->tm_year;
  _idr_date[1] = _tm->tm_mon + 1;
  _idr_date[2] = _tm->tm_mday;
  _idr_date[3] = _tm->tm_hour;
  _idr_date[4] = _tm->tm_min;
  _idr_date[5] = _tm->tm_sec;
  _idr_date[6] = 0x00; /* tz, GMT -48 +52 in 15min intervals */
}

static void
_pvd_set_time (char _pvd_date[], const struct tm *_tm)
{
  memset (_pvd_date, '0', 16);
  _pvd_date[16] = (int8_t) 0; /* tz */

  if (!_tm)
    return;

  snprintf(_pvd_date, 17, 
           "%4.4d%2.2d%2.2d" "%2.2d%2.2d%2.2d" "%2.2d",
           _tm->tm_year + 1900, _tm->tm_mon + 1, _tm->tm_mday,
           _tm->tm_hour, _tm->tm_min, _tm->tm_sec,
           0 /* 1/100 secs */ );
  
  _pvd_date[16] = (int8_t) 0; /* tz */
}

char *
iso9660_strncpy_pad(char dst[], const char src[], size_t len,
                    enum strncpy_pad_check _check)
{
  size_t rlen;

  cdio_assert (dst != NULL);
  cdio_assert (src != NULL);
  cdio_assert (len > 0);

  switch (_check)
    {
      int idx;
    case ISO9660_NOCHECK:
      break;

    case ISO9660_7BIT:
      for (idx = 0; src[idx]; idx++)
        if ((int8_t) src[idx] < 0)
          {
            cdio_warn ("string '%s' fails 7bit constraint (pos = %d)", 
                      src, idx);
            break;
          }
      break;

    case ISO9660_ACHARS:
      for (idx = 0; src[idx]; idx++)
        if (!iso9660_isachar (src[idx]))
          {
            cdio_warn ("string '%s' fails a-character constraint (pos = %d)",
                      src, idx);
            break;
          }
      break;

    case ISO9660_DCHARS:
      for (idx = 0; src[idx]; idx++)
        if (!iso9660_isdchar (src[idx]))
          {
            cdio_warn ("string '%s' fails d-character constraint (pos = %d)",
                      src, idx);
            break;
          }
      break;

    default:
      cdio_assert_not_reached ();
      break;
    }

  rlen = strlen (src);

  if (rlen > len)
    cdio_warn ("string '%s' is getting truncated to %d characters",  
              src, (unsigned) len);

  strncpy (dst, src, len);
  if (rlen < len)
    memset(dst+rlen, ' ', len-rlen);
  return dst;
}

int
iso9660_isdchar (int c)
{
  if (!IN (c, 0x30, 0x5f)
      || IN (c, 0x3a, 0x40)
      || IN (c, 0x5b, 0x5e))
    return false;

  return true;
}

int
iso9660_isachar (int c)
{
  if (!IN (c, 0x20, 0x5f)
      || IN (c, 0x23, 0x24)
      || c == 0x40
      || IN (c, 0x5b, 0x5e))
    return false;

  return true;
}

void 
iso9660_set_evd(void *pd)
{
  struct iso_volume_descriptor ied;

  cdio_assert (sizeof(struct iso_volume_descriptor) == ISO_BLOCKSIZE);

  cdio_assert (pd != NULL);
  
  memset(&ied, 0, sizeof(ied));

  ied.type = to_711(ISO_VD_END);
  iso9660_strncpy_pad (ied.id, ISO_STANDARD_ID, sizeof(ied.id), ISO9660_DCHARS);
  ied.version = to_711(ISO_VERSION);

  memcpy(pd, &ied, sizeof(ied));
}

/* important date to celebrate (for me at least =)
   -- until user customization is implemented... */
static const time_t _vcd_time = 269222400L;
                                       
void
iso9660_set_pvd(void *pd,
            const char volume_id[],
            const char publisher_id[],
            const char preparer_id[],
            const char application_id[],
            uint32_t iso_size,
            const void *root_dir,
            uint32_t path_table_l_extent,
            uint32_t path_table_m_extent,
            uint32_t path_table_size)
{
  pvd_t ipd;

  cdio_assert (sizeof(pvd_t) == ISO_BLOCKSIZE);

  cdio_assert (pd != NULL);
  cdio_assert (volume_id != NULL);
  cdio_assert (application_id != NULL);

  memset(&ipd,0,sizeof(ipd)); /* paranoia? */

  /* magic stuff ... thatis CD XA marker... */
  strcpy(((char*)&ipd)+ISO_XA_MARKER_OFFSET, ISO_XA_MARKER_STRING);

  ipd.type = to_711(ISO_VD_PRIMARY);
  iso9660_strncpy_pad (ipd.id, ISO_STANDARD_ID, 5, ISO9660_DCHARS);
  ipd.version = to_711(ISO_VERSION);

  iso9660_strncpy_pad (ipd.system_id, SYSTEM_ID, 32, ISO9660_ACHARS);
  iso9660_strncpy_pad (ipd.volume_id, volume_id, 32, ISO9660_DCHARS);

  ipd.volume_space_size = to_733(iso_size);

  ipd.volume_set_size = to_723(1);
  ipd.volume_sequence_number = to_723(1);
  ipd.logical_block_size = to_723(ISO_BLOCKSIZE);

  ipd.path_table_size = to_733(path_table_size);
  ipd.type_l_path_table = to_731(path_table_l_extent); 
  ipd.type_m_path_table = to_732(path_table_m_extent); 
  
  cdio_assert (sizeof(ipd.root_directory_record) == 34);
  memcpy(ipd.root_directory_record, root_dir, sizeof(ipd.root_directory_record));
  ipd.root_directory_record[0] = 34;

  iso9660_strncpy_pad (ipd.volume_set_id, VOLUME_SET_ID, 128, ISO9660_DCHARS);

  iso9660_strncpy_pad (ipd.publisher_id, publisher_id, 128, ISO9660_ACHARS);
  iso9660_strncpy_pad (ipd.preparer_id, preparer_id, 128, ISO9660_ACHARS);
  iso9660_strncpy_pad (ipd.application_id, application_id, 128, ISO9660_ACHARS); 

  iso9660_strncpy_pad (ipd.copyright_file_id    , "", 37, ISO9660_DCHARS);
  iso9660_strncpy_pad (ipd.abstract_file_id     , "", 37, ISO9660_DCHARS);
  iso9660_strncpy_pad (ipd.bibliographic_file_id, "", 37, ISO9660_DCHARS);

  _pvd_set_time (ipd.creation_date, gmtime (&_vcd_time));
  _pvd_set_time (ipd.modification_date, gmtime (&_vcd_time));
  _pvd_set_time (ipd.expiration_date, NULL);
  _pvd_set_time (ipd.effective_date, NULL);

  ipd.file_structure_version = to_711(1);

  /* we leave ipd.application_data = 0 */

  memcpy(pd, &ipd, sizeof(ipd)); /* copy stuff to arg ptr */
}

unsigned
iso9660_dir_calc_record_size(unsigned namelen, unsigned int su_len)
{
  unsigned length;

  length = sizeof(struct iso_directory_record);
  length += namelen;
  if (length % 2) /* pad to word boundary */
    length++;
  length += su_len;
  if (length % 2) /* pad to word boundary again */
    length++;

  return length;
}

void
iso9660_dir_add_entry_su(void *dir,
                         const char name[], 
                         uint32_t extent,
                         uint32_t size,
                         uint8_t flags,
                         const void *su_data,
                         unsigned su_size)
{
  struct iso_directory_record *idr = dir;
  uint8_t *dir8 = dir;
  unsigned offset = 0;
  uint32_t dsize = from_733(idr->size);
  int length, su_offset;

  cdio_assert (sizeof(struct iso_directory_record) == 33);

  if (!dsize && !idr->length)
    dsize = ISO_BLOCKSIZE; /* for when dir lacks '.' entry */
  
  cdio_assert (dsize > 0 && !(dsize % ISO_BLOCKSIZE));
  cdio_assert (dir != NULL);
  cdio_assert (extent > 17);
  cdio_assert (name != NULL);
  cdio_assert (strlen(name) <= MAX_ISOPATHNAME);

  length = sizeof(struct iso_directory_record);
  length += strlen(name);
  length = _cdio_ceil2block (length, 2); /* pad to word boundary */
  su_offset = length;
  length += su_size;
  length = _cdio_ceil2block (length, 2); /* pad to word boundary again */

  /* find the last entry's end */
  {
    unsigned ofs_last_rec = 0;

    offset = 0;
    while (offset < dsize)
      {
        if (!dir8[offset])
          {
            offset++;
            continue;
          }

        offset += dir8[offset];
        ofs_last_rec = offset;
      }

    cdio_assert (offset == dsize);

    offset = ofs_last_rec;
  }

  /* be sure we don't cross sectors boundaries */
  offset = _cdio_ofs_add (offset, length, ISO_BLOCKSIZE);
  offset -= length;

  cdio_assert (offset + length <= dsize);

  idr = (struct iso_directory_record*) &dir8[offset];
  
  cdio_assert (offset+length < dsize); 
  
  memset(idr, 0, length);

  idr->length = to_711(length);
  idr->extent = to_733(extent);
  idr->size = to_733(size);
  
  _idr_set_time (idr->date, gmtime (&_vcd_time));
  
  idr->flags = to_711(flags);

  idr->volume_sequence_number = to_723(1);

  idr->name_len = to_711(strlen(name) ? strlen(name) : 1); /* working hack! */

  memcpy(idr->name, name, from_711(idr->name_len));
  memcpy(&dir8[offset] + su_offset, su_data, su_size);
}

void
iso9660_dir_init_new (void *dir,
                      uint32_t self,
                      uint32_t ssize,
                      uint32_t parent,
                      uint32_t psize)
{
  iso9660_dir_init_new_su (dir, self, ssize, NULL, 0, parent, psize, NULL, 0);
}

void 
iso9660_dir_init_new_su (void *dir,
                         uint32_t self,
                         uint32_t ssize,
                         const void *ssu_data,
                         unsigned ssu_size,
                         uint32_t parent,
                         uint32_t psize,
                         const void *psu_data,
                         unsigned psu_size)
{
  cdio_assert (ssize > 0 && !(ssize % ISO_BLOCKSIZE));
  cdio_assert (psize > 0 && !(psize % ISO_BLOCKSIZE));
  cdio_assert (dir != NULL);

  memset (dir, 0, ssize);

  /* "\0" -- working hack due to padding  */
  iso9660_dir_add_entry_su (dir, "\0", self, ssize, ISO_DIRECTORY, ssu_data, 
                            ssu_size); 

  iso9660_dir_add_entry_su (dir, "\1", parent, psize, ISO_DIRECTORY, psu_data, 
                            psu_size);
}

void 
iso9660_pathtable_init (void *pt)
{
  cdio_assert (sizeof (struct iso_path_table) == 8);

  cdio_assert (pt != NULL);
  
  memset (pt, 0, ISO_BLOCKSIZE); /* fixme */
}

static const struct iso_path_table*
pathtable_get_entry (const void *pt, unsigned entrynum)
{
  const uint8_t *tmp = pt;
  unsigned offset = 0;
  unsigned count = 0;

  cdio_assert (pt != NULL);

  while (from_711 (*tmp)) 
    {
      if (count == entrynum)
        break;

      cdio_assert (count < entrynum);

      offset += sizeof (struct iso_path_table);
      offset += from_711 (*tmp);
      if (offset % 2)
        offset++;
      tmp = (uint8_t *)pt + offset;
      count++;
    }

  if (!from_711 (*tmp))
    return NULL;

  return (const void *) tmp;
}

void
pathtable_get_size_and_entries (const void *pt, 
                                unsigned *size,
                                unsigned *entries)
{
  const uint8_t *tmp = pt;
  unsigned offset = 0;
  unsigned count = 0;

  cdio_assert (pt != NULL);

  while (from_711 (*tmp)) 
    {
      offset += sizeof (struct iso_path_table);
      offset += from_711 (*tmp);
      if (offset % 2)
        offset++;
      tmp = (uint8_t *)pt + offset;
      count++;
    }

  if (size)
    *size = offset;

  if (entries)
    *entries = count;
}

unsigned int
iso9660_pathtable_get_size (const void *pt)
{
  unsigned size = 0;
  pathtable_get_size_and_entries (pt, &size, NULL);
  return size;
}

uint16_t 
iso9660_pathtable_l_add_entry (void *pt, 
                               const char name[], 
                               uint32_t extent, 
                               uint16_t parent)
{
  struct iso_path_table *ipt = 
    (struct iso_path_table*)((char *)pt + iso9660_pathtable_get_size (pt));
  size_t name_len = strlen (name) ? strlen (name) : 1;
  unsigned entrynum = 0;

  cdio_assert (iso9660_pathtable_get_size (pt) < ISO_BLOCKSIZE); /*fixme */

  memset (ipt, 0, sizeof (struct iso_path_table) + name_len); /* paranoia */

  ipt->name_len = to_711 (name_len);
  ipt->extent = to_731 (extent);
  ipt->parent = to_721 (parent);
  memcpy (ipt->name, name, name_len);

  pathtable_get_size_and_entries (pt, NULL, &entrynum);

  if (entrynum > 1)
    {
      const struct iso_path_table *ipt2 
        = pathtable_get_entry (pt, entrynum - 2);

      cdio_assert (ipt2 != NULL);

      cdio_assert (from_721 (ipt2->parent) <= parent);
    }
  
  return entrynum;
}

uint16_t 
iso9660_pathtable_m_add_entry (void *pt, 
                               const char name[], 
                               uint32_t extent, 
                               uint16_t parent)
{
  struct iso_path_table *ipt =
    (struct iso_path_table*)((char *)pt + iso9660_pathtable_get_size (pt));
  size_t name_len = strlen (name) ? strlen (name) : 1;
  unsigned entrynum = 0;

  cdio_assert (iso9660_pathtable_get_size(pt) < ISO_BLOCKSIZE); /* fixme */

  memset(ipt, 0, sizeof (struct iso_path_table) + name_len); /* paranoia */

  ipt->name_len = to_711 (name_len);
  ipt->extent = to_732 (extent);
  ipt->parent = to_722 (parent);
  memcpy (ipt->name, name, name_len);

  pathtable_get_size_and_entries (pt, NULL, &entrynum);

  if (entrynum > 1)
    {
      const struct iso_path_table *ipt2 
        = pathtable_get_entry (pt, entrynum - 2);

      cdio_assert (ipt2 != NULL);

      cdio_assert (from_722 (ipt2->parent) <= parent);
    }

  return entrynum;
}

bool
iso9660_dirname_valid_p (const char pathname[])
{
  const char *p = pathname;
  int len;

  cdio_assert (pathname != NULL);

  if (*p == '/' || *p == '.' || *p == '\0')
    return false;

  if (strlen (pathname) > MAX_ISOPATHNAME)
    return false;
  
  len = 0;
  for (; *p; p++)
    if (iso9660_isdchar (*p))
      {
        len++;
        if (len > 8)
          return false;
      }
    else if (*p == '/')
      {
        if (!len)
          return false;
        len = 0;
      }
    else
      return false; /* unexpected char */

  if (!len)
    return false; /* last char may not be '/' */

  return true;
}

bool
iso9660_pathname_valid_p (const char pathname[])
{
  const char *p = NULL;

  cdio_assert (pathname != NULL);

  if ((p = strrchr (pathname, '/')))
    {
      bool rc;
      char *_tmp = strdup (pathname);
      
      *strrchr (_tmp, '/') = '\0';

      rc = iso9660_dirname_valid_p (_tmp);

      free (_tmp);

      if (!rc)
        return false;

      p++;
    }
  else
    p = pathname;

  if (strlen (pathname) > (MAX_ISOPATHNAME - 6))
    return false;

  {
    int len = 0;
    int dots = 0;

    for (; *p; p++)
      if (iso9660_isdchar (*p))
        {
          len++;
          if (dots == 0 ? len > 8 : len > 3)
            return false;
        }
      else if (*p == '.')
        {
          dots++;
          if (dots > 1)
            return false;
          if (!len)
            return false;
          len = 0;
        }
      else
        return false;

    if (dots != 1)
      return false;
  }

  return true;
}

char *
iso9660_pathname_isofy (const char pathname[], uint16_t version)
{
  char tmpbuf[1024] = { 0, };
    
  cdio_assert (strlen (pathname) < (sizeof (tmpbuf) - sizeof (";65535")));

  snprintf (tmpbuf, sizeof(tmpbuf), "%s;%d", pathname, version);

  return strdup (tmpbuf);
}

uint8_t
iso9660_get_dir_extent(const iso_directory_record_t *idr) 
{
  if (NULL == idr) return 0;
  return from_733(idr->extent);
}

uint8_t
iso9660_get_dir_len(const iso_directory_record_t *idr) 
{
  if (NULL == idr) return 0;
  return idr->length;
}

uint8_t
iso9660_get_dir_size(const iso_directory_record_t *idr) 
{
  if (NULL == idr) return 0;
  return from_733(idr->size);
}

uint8_t
iso9660_get_pvd_type(const pvd_t *pvd) 
{
  if (NULL == pvd) return 255;
  return(pvd->type);
}

const char *
iso9660_get_pvd_id(const pvd_t *pvd) 
{
  if (NULL == pvd) return "ERR";
  return(pvd->id);
}

int
iso9660_get_pvd_space_size(const pvd_t *pvd) 
{
  if (NULL == pvd) return 0;
  return from_733(pvd->volume_space_size);
}

int
iso9660_get_pvd_block_size(const pvd_t *pvd) 
{
  if (NULL == pvd) return 0;
  return from_723(pvd->logical_block_size);
}

int
iso9660_get_pvd_version(const pvd_t *pvd) 
{
  if (NULL == pvd) return 0;
  return pvd->version;
}

lsn_t
iso9660_get_root_lsn(const pvd_t *pvd) 
{
  if (NULL == pvd) 
    return CDIO_INVALID_LSN;
  else {
    iso_directory_record_t *idr = (void *) pvd->root_directory_record;
    if (NULL == idr) return CDIO_INVALID_LSN;
    return(from_733 (idr->extent));
  }
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
