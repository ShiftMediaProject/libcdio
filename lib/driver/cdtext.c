/*
  Copyright (C) 2004, 2005, 2008, 2011 Rocky Bernstein <rocky@gnu.org>
  toc reading routine adapted from cuetools
  Copyright (C) 2003 Svend Sanjay Sorensen <ssorensen@fastmail.fm>

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

#ifdef HAVE_CONFIG_H
# include "config.h"
# define __CDIO_CONFIG_H__ 1
#endif

#include <cdio/cdtext.h>
#include <cdio/logging.h>
#include "cdtext_private.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#define _CDTEXT_DBCC

/*! Note: the order and number items (except CDTEXT_INVALID) should
  match the cdtext_field_t enumeration. */
static const char cdtext_keywords[][16] = 
{
  "ARRANGER",
  "COMPOSER",
  "DISC_ID",
  "GENRE",
  "MESSAGE",
  "ISRC",
  "PERFORMER",
  "SIZE_INFO",
  "SONGWRITER",
  "TITLE",
  "TOC_INFO",
  "TOC_INFO2",
  "UPC_EAN",
};

static const char cdtext_genre[][30] =
{
  "Not Used",
  "Not Defined",
  "Adult Contemporary",
  "Alternative Rock",
  "Childrens Music",
  "Classical",
  "Contemporary Christian",
  "Country",
  "Dance",
  "Easy Listening",
  "Erotic",
  "Folk",
  "Gospel",
  "Hip Hop",
  "Jazz",
  "Latin",
  "Musical",
  "New Age",
  "Opera",
  "Operetta",
  "Pop Music",
  "Rap",
  "Reggae",
  "Rock Music",
  "Rhythm & Blues",
  "Sound Effects",
  "Spoken Word",
  "World Music"
};

/*! Return string representation of the enum values above */
const char *
cdtext_field2str (cdtext_field_t i)
{
  if (i >= MAX_CDTEXT_FIELDS)
    return "Invalid CDTEXT field index";
  else 
    return cdtext_keywords[i];
}

/*! Return string representation of the given genre oode */
const char *
cdtext_genre2str (cdtext_genre_t i)
{
  if (i >= MAX_CDTEXT_GENRE_CODE)
    return "Invalid genre code";
  else
    return cdtext_genre[i];
}

/*! Free memory assocated with cdtext*/
void 
cdtext_destroy (cdtext_t *p_cdtext)
{
  cdtext_field_t i;
  track_t i2;

  if (!p_cdtext) return; 
  for (i2=0; i2<=99; i2++) {
    for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
      if (p_cdtext->track[i2].field[i]) {
        free(p_cdtext->track[i2].field[i]);
        p_cdtext->track[i2].field[i] = NULL;
      }
    }
  }
}

/*! 
  returns the CDTEXT value associated with key. NULL is returned
  if key is CDTEXT_INVALID or the field is not set.
 */
char *
cdtext_get (cdtext_field_t key, track_t track, const cdtext_t *p_cdtext)
{
  const char *ret = cdtext_get_const(key, track, p_cdtext);
  if (NULL == ret)
    return NULL;
  else
    return strdup(ret);
}

const char *
cdtext_get_const (cdtext_field_t key, track_t track, const cdtext_t *p_cdtext)
{
  if (CDTEXT_INVALID == key
      || NULL == p_cdtext
      || 0 > track
      || 99 < track)
    return NULL;

  return p_cdtext->track[track].field[key];
}


/*! Initialize a new cdtext structure.
  When the structure is no longer needed, release the 
  resources using cdtext_delete.
*/
void 
cdtext_init (cdtext_t *p_cdtext)
{
  cdtext_field_t i;
  track_t i2;

  for (i2=0; i2<=99; i2++) {
    for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
      p_cdtext->track[i2].field[i] = NULL;
    }
  }

  p_cdtext->genre_code = CDIO_CDTEXT_GENRE_UNUSED;
  p_cdtext->language[0] = '\0';
  p_cdtext->encoding[0] = '\0';
  p_cdtext->block       = 0;
}

/*!
  returns associated cdtext_field_t if field is a CD-TEXT keyword, returns non-zero otherwise 
*/
cdtext_field_t
cdtext_is_keyword (const char *key)
{
  unsigned int i;
  
  for (i = 0; i < 13 ; i++)
    if (0 == strcmp (cdtext_keywords[i], key)) {
      return i;
    }
  return CDTEXT_INVALID;
}

/*! sets cdtext's keyword entry to field.
 */
void 
cdtext_set (cdtext_field_t key, track_t track, const char *p_value, cdtext_t *p_cdtext)
{
  if (NULL == p_value || key == CDTEXT_INVALID || 0 > track || 99 < track) return;

  if (p_cdtext->track[track].field[key]) 
    free (p_cdtext->track[track].field[key]);
  p_cdtext->track[track].field[key] = strdup (p_value);
  
}

#define SET_CDTEXT_FIELD(FIELD) \
  cdtext_set(FIELD, i_track, buffer, p_cdtext);

/* 
  parse all CD-TEXT data retrieved.
*/       
bool
cdtext_data_init(cdtext_t *p_cdtext, uint8_t *wdata, size_t i_data) 
{
  CDText_data_t *p_data;
  int           i;
  int           j;
  char          buffer[256];
  int           idx;
  int           i_track;
  bool          b_ret = false;
  int           block = 0;
  CDText_blocksize_t p_blocksize;
  
  memset( buffer, 0x00, sizeof(buffer) );
  idx = 0;
  
  memset( &p_blocksize, 0x00, sizeof(CDText_blocksize_t) );

  p_data = (CDText_data_t *) wdata;
  if (0 != i_data % sizeof(CDText_data_t)) {
    cdio_warn("CD-Text size is not multiple of pack size");
    return false;
  }

  for( i = -1; i_data > 0; i_data -= sizeof(CDText_data_t), p_data++ ) {
    
#ifndef _CDTEXT_DBCC
    if ( p_data->bDBC ) {
      cdio_warn("Double-byte characters not supported");
      return false;
    }
#endif

    if ( p_data->seq != ++i || p_data->block != block ) break;

    /* only handle character packs */
    if ( ((p_data->type >= 0x80) && (p_data->type <= 0x87)) || 
        (p_data->type == 0x8E)) 
    {
      i_track = p_data->i_track;

      j = 0;
      if (CDIO_CDTEXT_GENRE == p_data->type) {
        j = 2;
        if (CDIO_CDTEXT_GENRE_UNUSED == p_cdtext->genre_code) {
          p_cdtext->genre_code = CDTEXT_GET_LEN16(p_data->text);
        }
      }

#ifdef _CDTEXT_DBCC
      for( ; j < CDIO_CDTEXT_MAX_TEXT_DATA; (p_data->bDBC ? j+=2 : j++) ) {
        if( p_data->text[j] == 0x00 && (!p_data->bDBC || p_data->text[j+1] == 0x00)) {
          if((buffer[0] != 0x00) && (!p_data->bDBC || buffer[1] != 0x00)) {
#else
      for( ; j < CDIO_CDTEXT_MAX_TEXT_DATA; j++) {
        if( p_data->text[j] == 0x00) {
          if(buffer[0] != 0x00) {
#endif
          
          /* omit empty strings */
            switch( p_data->type) {
            case CDIO_CDTEXT_TITLE: 
              SET_CDTEXT_FIELD(CDTEXT_TITLE);
              break;
            case CDIO_CDTEXT_PERFORMER:  
              SET_CDTEXT_FIELD(CDTEXT_PERFORMER);
              break;
            case CDIO_CDTEXT_SONGWRITER:
              SET_CDTEXT_FIELD(CDTEXT_SONGWRITER);
              break;
            case CDIO_CDTEXT_COMPOSER:
              SET_CDTEXT_FIELD(CDTEXT_COMPOSER);
              break;
            case CDIO_CDTEXT_ARRANGER:
              SET_CDTEXT_FIELD(CDTEXT_ARRANGER);
              break;
            case CDIO_CDTEXT_MESSAGE:
              SET_CDTEXT_FIELD(CDTEXT_MESSAGE);
              break;
            case CDIO_CDTEXT_DISCID:
              if(i_track == 0) {
                SET_CDTEXT_FIELD(CDTEXT_DISCID);
              }
              break;
            case CDIO_CDTEXT_UPC:
              if(i_track == 0) {
                SET_CDTEXT_FIELD(CDTEXT_UPC_EAN);
              }
              else {
                SET_CDTEXT_FIELD(CDTEXT_ISRC);
              }
              break;
            case CDIO_CDTEXT_GENRE:
              SET_CDTEXT_FIELD(CDTEXT_GENRE);
              break;
            }
            
            b_ret = true;
            i_track++;
            idx = 0;
          }
        } else {
          buffer[idx++] = p_data->text[j];
#ifdef _CDTEXT_DBCC
          if(p_data->bDBC)
             buffer[idx++] = p_data->text[j+1];
#endif
        }

        buffer[idx] = 0x00;
#ifdef _CDTEXT_DBCC
        if(p_data->bDBC)
           buffer[idx+1] = 0x00;
#endif
      }
    } else {
      /* not a character pack */

      if(p_data->type == CDIO_CDTEXT_TOC) {
        /* no idea what this is */
      }

      if (p_data->type == CDIO_CDTEXT_TOC2) {
        /* no idea what this is */
      }

      /* i got this info from cdrtools' cdda2wav; all the tests i ran so far were successful */
      if (p_data->type == CDIO_CDTEXT_BLOCKSIZE) {
	      /* i_track is the pack element number in this case */
        switch(p_data->i_track){
	        case 0:
	          memcpy((char*)&p_blocksize,p_data->text,0x0c);
	          break;
	        case 1:
	          memcpy(((char*)&p_blocksize)+0x0c,p_data->text,0x0c);
	          break;
	        case 2:
	          memcpy(((char*)&p_blocksize)+0x18,p_data->text,0x0c);
	          break;
        }
      }
    }
  }
  
  if (p_blocksize.i_packs[15] >= 3) {
    /* if there were more than 3 BLOCKSIZE packs */
    switch (p_blocksize.charcode){
      case CDIO_CDTEXT_CHARCODE_ISO_8859_1:
        strcpy(p_cdtext->encoding, "ISO-8859-1");
        break;
      case CDIO_CDTEXT_CHARCODE_ASCII:
        strcpy(p_cdtext->encoding, "ASCII");
        break;
      case CDIO_CDTEXT_CHARCODE_KANJI:
        strcpy(p_cdtext->encoding, "Shift-JIS");
        break;
    }
 
    /* set ISO 639-1 code */
    switch (p_blocksize.langcode[block]) {
      case CDIO_CDTEXT_LANG_CZECH        :
        strcpy(p_cdtext->language, "cs");
        break;
      case CDIO_CDTEXT_LANG_DANISH       :
        strcpy(p_cdtext->language, "da");
        break;
      case CDIO_CDTEXT_LANG_GERMAN       :
        strcpy(p_cdtext->language, "de");
        break;
      case CDIO_CDTEXT_LANG_ENGLISH      :
        strcpy(p_cdtext->language, "en");
        break;
      case CDIO_CDTEXT_LANG_SPANISH      :
        strcpy(p_cdtext->language, "es");
        break;
      case CDIO_CDTEXT_LANG_FRENCH       :
        strcpy(p_cdtext->language, "fr");
        break;
      case CDIO_CDTEXT_LANG_ITALIAN      :
        strcpy(p_cdtext->language, "it");
        break;
      case CDIO_CDTEXT_LANG_HUNGARIAN    :
        strcpy(p_cdtext->language, "hu");
        break;
      case CDIO_CDTEXT_LANG_DUTCH        :
        strcpy(p_cdtext->language, "nl");
        break;
      case CDIO_CDTEXT_LANG_NORWEGIAN    :
        strcpy(p_cdtext->language, "no");
        break;
      case CDIO_CDTEXT_LANG_POLISH       :
        strcpy(p_cdtext->language, "pl");
        break;
      case CDIO_CDTEXT_LANG_PORTUGUESE   :
        strcpy(p_cdtext->language, "pt");
        break;
      case CDIO_CDTEXT_LANG_SLOVENE      :
        strcpy(p_cdtext->language, "sl");
        break;
      case CDIO_CDTEXT_LANG_FINNISH      :
        strcpy(p_cdtext->language, "fi");
        break;
      case CDIO_CDTEXT_LANG_SWEDISH      :
        strcpy(p_cdtext->language, "sv");
        break;
      case CDIO_CDTEXT_LANG_RUSSIAN      :
        strcpy(p_cdtext->language, "ru");
        break;
      case CDIO_CDTEXT_LANG_KOREAN       :
        strcpy(p_cdtext->language, "ko");
        break;
      case CDIO_CDTEXT_LANG_JAPANESE     :
        strcpy(p_cdtext->language, "ja");
        break;
      case CDIO_CDTEXT_LANG_GREEK        :
        strcpy(p_cdtext->language, "el");
        break;
      case CDIO_CDTEXT_LANG_CHINESE      :
        strcpy(p_cdtext->language, "zh");
        break;
    }
    
  }
  return b_ret;
}
