/*
    $Id: util.h,v 1.7 2005/02/28 02:05:19 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>

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

#ifndef __CDIO_UTIL_H__
#define __CDIO_UTIL_H__

/*!
   \file util.h 
   \brief Miscellaneous utility functions. 

   Warning: this will probably get removed/replaced by using glib.h
*/
#include <stdlib.h>

#undef  MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))

#undef  MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))

#undef  IN
#define IN(x, low, high) ((x) >= (low) && (x) <= (high))

#undef  CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static inline uint16_t
_cdio_len2blocks (unsigned int i_len, uint16_t i_blocksize)
{
  uint16_t i_blocks;

  i_blocks = i_len / i_blocksize;
  if (i_len % i_blocksize)
    i_blocks++;

  return i_blocks;
}

/* round up to next block boundary */
static inline unsigned 
_cdio_ceil2block (unsigned offset, uint16_t i_blocksize)
{
  return _cdio_len2blocks (offset, i_blocksize) * i_blocksize;
}

static inline unsigned int
_cdio_ofs_add (unsigned offset, unsigned length, uint16_t i_blocksize)
{
  if (i_blocksize - (offset % i_blocksize) < length)
    offset = _cdio_ceil2block (offset, i_blocksize);

  offset += length;

  return offset;
}

static inline const char *
_cdio_bool_str (bool b)
{
  return b ? "yes" : "no";
}

#ifdef __cplusplus
extern "C" {
#endif

void *
_cdio_memdup (const void *mem, size_t count);

char *
_cdio_strdup_upper (const char str[]);

void
_cdio_strfreev(char **strv);

char *
_cdio_strjoin (char *strv[], unsigned count, const char delim[]);

size_t
_cdio_strlenv(char **str_array);

char **
_cdio_strsplit(const char str[], char delim);

uint8_t cdio_to_bcd8(uint8_t n);
uint8_t cdio_from_bcd8(uint8_t p);

#if defined(__GNUC__) && __GNUC__ >= 3
static inline __attribute__((deprecated))
uint8_t to_bcd8(uint8_t n) {
  return cdio_to_bcd8(n);
}
static inline __attribute__((deprecated))
uint8_t from_bcd8(uint8_t p) {
  return cdio_from_bcd8(p);
}
#else
#define to_bcd8 cdio_to_bcd8
#define from_bcd8 cdio_from_bcd8
#endif

#ifdef __cplusplus
}
#endif

#endif /* __CDIO_UTIL_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
