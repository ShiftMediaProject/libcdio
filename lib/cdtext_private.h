/*
    $Id: cdtext_private.h,v 1.4 2004/08/30 00:26:59 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>

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

#ifndef __CDIO_CDTEXT_PRIVATE_H__
#define __CDIO_CDTEXT_PRIVATE_H__

#include <cdio/cdio.h>
#include <cdio/cdtext.h>

#define CDIO_CDTEXT_MAX_PACK_DATA  255
#define CDIO_CDTEXT_MAX_TEXT_DATA  12

#define CDIO_CDTEXT_TITLE      0x80
#define CDIO_CDTEXT_PERFORMER  0x81
#define CDIO_CDTEXT_SONGWRITER 0x82
#define CDIO_CDTEXT_COMPOSER   0x83
#define CDIO_CDTEXT_ARRANGER   0x84
#define CDIO_CDTEXT_MESSAGE    0x85
#define CDIO_CDTEXT_DISCID     0x86
#define CDIO_CDTEXT_GENRE      0x87

PRAGMA_BEGIN_PACKED

struct CDText_data
{
  uint8_t  type;
  track_t  i_track;
  uint8_t  seq;
#ifdef WORDS_BIGENDIAN
  uint8_t  bDBC:             1;	 /* double byte character */
  uint8_t  block:            3;  /* block number 0..7 */
  uint8_t  characterPosition:4;  /* character position */
#else
  uint8_t  characterPosition:4;  /* character position */
  uint8_t  block            :3;	 /* block number 0..7 */
  uint8_t  bDBC             :1;	 /* double byte character */
#endif
  char     text[CDIO_CDTEXT_MAX_TEXT_DATA];
  uint8_t  crc[2];
} GNUC_PACKED;

PRAGMA_END_PACKED

typedef struct CDText_data CDText_data_t;

typedef void (*set_cdtext_field_fn_t) (void *user_data, track_t i_track,
                                       track_t i_first_track,
                                       cdtext_field_t field, 
                                       const char *buffer);

/* 
   Internal routine to parse all CD-TEXT data retrieved.
*/       
bool cdtext_data_init(void *user_data, track_t i_first_track, 
                      const unsigned char *wdata, 
                      set_cdtext_field_fn_t set_cdtext_field_fn);


#endif /* __CDIO_CDTEXT_PRIVATE_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
