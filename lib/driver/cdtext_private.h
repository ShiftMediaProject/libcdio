/*
  Copyright (C) 2004, 2005, 2008, 2011 Rocky Bernstein <rocky@gnu.org>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __CDIO_CDTEXT_PRIVATE_H__
#define __CDIO_CDTEXT_PRIVATE_H__

#include <cdio/cdio.h>
#include <cdio/cdtext.h>

#define CDTEXT_GET_LEN16(p) (p[0]<<8) + p[1]

/*! An enumeration for some of the CDIO_CDTEXT_* #defines below. This isn't
    really an enumeration one would really use in a program it is here
    to be helpful in debuggers where wants just to refer to the
    ISO_*_ names and get something.
  */
extern const enum cdtext_enum1_s {
  CDIO_CDTEXT_MAX_PACK_DATA = 255,
  CDIO_CDTEXT_MAX_TEXT_DATA = 12,
  CDIO_CDTEXT_TITLE         = 0x80,
  CDIO_CDTEXT_PERFORMER     = 0x81,
  CDIO_CDTEXT_SONGWRITER    = 0x82,
  CDIO_CDTEXT_COMPOSER      = 0x83,
  CDIO_CDTEXT_ARRANGER      = 0x84,
  CDIO_CDTEXT_MESSAGE       = 0x85,
  CDIO_CDTEXT_DISCID        = 0x86,
  CDIO_CDTEXT_GENRE         = 0x87,
  CDIO_CDTEXT_TOC           = 0x88,
  CDIO_CDTEXT_TOC2          = 0x89,
  CDIO_CDTEXT_UPC           = 0x8E,
  CDIO_CDTEXT_BLOCKSIZE     = 0x8F
} cdtext_enums1;


/*!
 * CD-Text character codes 
 */
extern const enum cdtext_charcode_enum_s {
  CDIO_CDTEXT_CHARCODE_ISO_8859_1 = 0x00, /**< ISO-8859-1 */
  CDIO_CDTEXT_CHARCODE_ASCII      = 0x01, /**< ASCII - 7 bit */
  CDIO_CDTEXT_CHARCODE_KANJI      = 0x80, /**< Music Shift-JIS Kanji - 
                                               double byte */
  CDIO_CDTEXT_CHARCODE_KOREAN     = 0x81, /**< Korean */
  CDIO_CDTEXT_CHARCODE_CHINESE    = 0x82, /**< Mandarin Chinese */
  CDIO_CDTEXT_CHARCODE_UNDEFINED  = 0xFF, /**< everything else */
} cdtext_charcode_enum;

/*!
 * The language code is encoded as specified in ANNEX 1 to part 5 of EBU
 * Tech 32 58 -E (1991).
 */
extern const enum cdtext_lang_enum_s 
{
  CDIO_CDTEXT_LANG_CZECH        = 0x06,
  CDIO_CDTEXT_LANG_DANISH       = 0x07,
  CDIO_CDTEXT_LANG_GERMAN       = 0x08,
  CDIO_CDTEXT_LANG_ENGLISH      = 0x09,
  CDIO_CDTEXT_LANG_SPANISH      = 0x0A,
  CDIO_CDTEXT_LANG_FRENCH       = 0x0F,
  CDIO_CDTEXT_LANG_ITALIAN      = 0x15,
  CDIO_CDTEXT_LANG_HUNGARIAN    = 0x1b,
  CDIO_CDTEXT_LANG_DUTCH        = 0x1d,
  CDIO_CDTEXT_LANG_NORWEGIAN    = 0x1e,
  CDIO_CDTEXT_LANG_POLISH       = 0x20,
  CDIO_CDTEXT_LANG_PORTUGUESE   = 0x21,
  CDIO_CDTEXT_LANG_SLOVENE      = 0x26,
  CDIO_CDTEXT_LANG_FINNISH      = 0x27,
  CDIO_CDTEXT_LANG_SWEDISH      = 0x28,
  CDIO_CDTEXT_LANG_RUSSIAN      = 0x56,
  CDIO_CDTEXT_LANG_KOREAN       = 0x65,
  CDIO_CDTEXT_LANG_JAPANESE     = 0x69,
  CDIO_CDTEXT_LANG_GREEK        = 0x70,
  CDIO_CDTEXT_LANG_CHINESE      = 0x75,
} cdtext_lang_enum;
  

#define CDIO_CDTEXT_MAX_PACK_DATA  255
#define CDIO_CDTEXT_MAX_TEXT_DATA  12

/* From table J.2 - Pack Type Indicator Definitions from 
   Working Draft NCITS XXX T10/1364-D Revision 10G. November 12, 2001.
*/
/* Title of Album name (ID=0) or Track Titles (ID != 0) */
#define CDIO_CDTEXT_TITLE      0x80 

/* Name(s) of the performer(s) in ASCII */
#define CDIO_CDTEXT_PERFORMER  0x81

/* Name(s) of the songwriter(s) in ASCII */
#define CDIO_CDTEXT_SONGWRITER 0x82

/* Name(s) of the Composers in ASCII */
#define CDIO_CDTEXT_COMPOSER   0x83

/* Name(s) of the Arrangers in ASCII */
#define CDIO_CDTEXT_ARRANGER   0x84

/* Message(s) from content provider and/or artist in ASCII */
#define CDIO_CDTEXT_MESSAGE    0x85

/* Disc Identificatin information */
#define CDIO_CDTEXT_DISCID     0x86

/* Genre Identification and Genre Information */
#define CDIO_CDTEXT_GENRE      0x87

/* Table of Content Information */
#define CDIO_CDTEXT_TOC        0x88

/* Second Table of Content Information */
#define CDIO_CDTEXT_TOC2       0x89

/* 0x8A, 0x8B, 0x8C are reserved
   0x8D Reserved for content provider only.
 */

/* UPC/EAN code of the album and ISRC code of each track */
#define CDIO_CDTEXT_UPC        0x8E

/* Size information of the Block */
#define CDIO_CDTEXT_BLOCKSIZE  0x8F

PRAGMA_BEGIN_PACKED

struct CDText_data
{
  uint8_t  type;
  track_t  i_track;
  uint8_t  seq;
#ifdef WORDS_BIGENDIAN
  uint8_t  bDBC:             1;  /* double byte character */
  uint8_t  block:            3;  /* block number 0..7 */
  uint8_t  characterPosition:4;  /* character position */
#else
  uint8_t  characterPosition:4;  /* character position */
  uint8_t  block            :3;  /* block number 0..7 */
  uint8_t  bDBC             :1;  /* double byte character */
#endif
  char     text[CDIO_CDTEXT_MAX_TEXT_DATA];
  uint8_t  crc[2];
} GNUC_PACKED;

PRAGMA_END_PACKED

/*
 * content of BLOCKSIZE packs
 */
 
PRAGMA_BEGIN_PACKED

struct CDText_blocksize
{
  uint8_t charcode;     /* character code */
  uint8_t i_first_track; /* first track number */
  uint8_t i_last_track;  /* last track number */
  uint8_t copyright;    /* cd-text information copyright byte */
  uint8_t i_packs[16];/* number of packs of each type 
                         * 0 TITLE; 1 PERFORMER; 2 SONGWRITER; 3 COMPOSER; 
                         * 4 ARRANGER; 5 MESSAGE; 6 DISCID; 7 GENRE; 
                         * 8 TOC; 9 TOC2; 10-12 RESERVED; 13 CLOSED; 
                         * 14 UPC_ISRC; 15 BLOCKSIZE */
  uint8_t lastseq[8];   /* last sequence for block 0..7 */
  uint8_t langcode[8];  /* language code for block 0..7 */
} GNUC_PACKED;

PRAGMA_END_PACKED;


typedef struct CDText_blocksize CDText_blocksize_t;
typedef struct CDText_data CDText_data_t;


#endif /* __CDIO_CDTEXT_PRIVATE_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
