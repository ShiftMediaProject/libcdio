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

#include <stdio.h>

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
} ;

/*! Return string representation of the enum values above */
const char *
cdtext_field2str (cdtext_field_t i)
{
  if (i >= MAX_CDTEXT_FIELDS)
    return "Invalid CDTEXT field index";
  else 
    return cdtext_keywords[i];
}

/*! Free memory assocated with cdtext*/
void 
cdtext_destroy (cdtext_t *p_cdtext)
{
  cdtext_field_t i;

  if (!p_cdtext) return; 
  for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
    if (p_cdtext->field[i]) {
      free(p_cdtext->field[i]);
      p_cdtext->field[i] = NULL;
    }
    
  }
}

/*! 
  returns the CDTEXT value associated with key. NULL is returned
  if key is CDTEXT_INVALID or the field is not set.
 */
char *
cdtext_get (cdtext_field_t key, const cdtext_t *p_cdtext)
{
  if ((key == CDTEXT_INVALID) || !p_cdtext 
      || (!p_cdtext->field[key])) return NULL;
  return strdup(p_cdtext->field[key]);
}

const char *
cdtext_get_const (cdtext_field_t key, const cdtext_t *p_cdtext)
{
  if (key == CDTEXT_INVALID) return NULL;
  return p_cdtext->field[key];
}


/*! Initialize a new cdtext structure.
  When the structure is no longer needed, release the 
  resources using cdtext_delete.
*/
void 
cdtext_init (cdtext_t *p_cdtext)
{
  cdtext_field_t i;

  for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
    p_cdtext->field[i] = NULL;
  }
}

/*!
  returns 0 if field is a CD-TEXT keyword, returns non-zero otherwise 
*/
cdtext_field_t
cdtext_is_keyword (const char *key)
{
#if 0  
  char *item;
  
  item = bsearch(key, 
                 cdtext_keywords, 12,
                 sizeof (char *), 
                 (int (*)(const void *, const void *))
                 strcmp);
  return (NULL != item) ? 0 : 1;
#else 
  unsigned int i;
  
  for (i = 0; i < 13 ; i++)
    if (0 == strcmp (cdtext_keywords[i], key)) {
      return i;
    }
  return CDTEXT_INVALID;
#endif
}

/*! sets cdtext's keyword entry to field.
 */
void 
cdtext_set (cdtext_field_t key, const char *p_value, cdtext_t *p_cdtext)
{
  if (NULL == p_value || key == CDTEXT_INVALID) return;
  
  if (p_cdtext->field[key]) free (p_cdtext->field[key]);
  p_cdtext->field[key] = strdup (p_value);
  
}

#define SET_CDTEXT_FIELD(FIELD) \
  (*set_cdtext_field_fn)(p_user_data, i_track, i_first_track, FIELD, buffer);

/* 
  parse all CD-TEXT data retrieved.
*/       
bool
cdtext_data_init(void *p_user_data, track_t i_first_track, 
                 unsigned char *wdata, int i_data,
                 set_cdtext_field_fn_t set_cdtext_field_fn) 
{
  CDText_data_t *p_data;
  int           i = -1;
  int           j;
  char          buffer[256];
  int           idx;
  int           i_track;
  bool          b_ret = false;
  char          block = 0;
  char    encoding[16];
  CDText_blocksize_t p_blocksize;
  
  memset( buffer, 0x00, sizeof(buffer) );
  idx = 0;
  
  bzero(encoding,16);
  bzero(&p_blocksize, sizeof(CDText_blocksize_t));

  p_data = (CDText_data_t *) (&wdata[4]);

  for( ; i_data > 0; 
       i_data -= sizeof(CDText_data_t), p_data++ ) {
    
#if TESTED
    if ( p_data->bDBC ) {
      cdio_warn("Double-byte characters not supported");
      return false;
    }
#endif
    
    if ( p_data->seq != ++i || p_data->block != block ) break;

    /* only handle character packs */
    if ( ((p_data->type >= 0x80) && (p_data->type <= 0x86)) || 
        (p_data->type == 0x8E)) {

      i_track = p_data->i_track;

      for( j=0; j < CDIO_CDTEXT_MAX_TEXT_DATA; (p_data->bDBC ? j+=2 : j++) ) {
        if( p_data->text[j] == 0x00 && (!p_data->bDBC || p_data->text[j+1] == 0x00)) {
          
          /* omit empty strings */
          if((buffer[0] != 0x00) && (!p_data->bDBC || buffer[1] != 0x00)) {
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
            }
            
            b_ret = true;
            i_track++;
            idx = 0;
          }
        } else {
          buffer[idx++] = p_data->text[j];
          if(p_data->bDBC)
             buffer[idx++] = p_data->text[j+1];
        }

        buffer[idx] = 0x00;
        if(p_data->bDBC)
           buffer[idx+1] = 0x00;
      }
    } else {
      /* not a character pack */
      if (p_data->type == CDIO_CDTEXT_GENRE) {
        i_track = p_data->i_track;
        /* seems like it is a uint_16 in the first 2 bytes */
        if((p_data->text[0] << 8) + p_data->text[1] != CDIO_CDTEXT_GENRE_UNUSED) {
          sprintf(buffer,"%s",cdtext_genre[(p_data->text[0] << 8) + p_data->text[1]]);
          SET_CDTEXT_FIELD(CDTEXT_GENRE);
    }
#ifdef _DEBUG_CDTEXT
        printf("GENRE information present: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	       p_data->text[0],p_data->text[1],p_data->text[2],p_data->text[3],
	       p_data->text[4],p_data->text[5],p_data->text[6],p_data->text[7], 
	       p_data->text[8],p_data->text[9],p_data->text[10],p_data->text[11]);
#endif
      }

      if(p_data->type == CDIO_CDTEXT_TOC) {
#ifdef _DEBUG_CDTEXT
        printf("TOC information present: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
          p_data->text[0],p_data->text[1],p_data->text[2],p_data->text[3],
          p_data->text[4],p_data->text[5],p_data->text[6],p_data->text[7], 
          p_data->text[8],p_data->text[9],p_data->text[10],p_data->text[11]);
#endif
      }

      if (p_data->type == CDIO_CDTEXT_TOC2) {
#ifdef _DEBUG_CDTEXT
        printf("TOC2 information present: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
          p_data->text[0],p_data->text[1],p_data->text[2],p_data->text[3],
          p_data->text[4],p_data->text[5],p_data->text[6],p_data->text[7], 
          p_data->text[8],p_data->text[9],p_data->text[10],p_data->text[11]);
#endif
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
  
  if (p_blocksize.packcount[15] >= 3) {
    /* BLOCKSIZE packs present */
    switch (p_blocksize.charcode){
    case CDIO_CDTEXT_CHARCODE_ISO_8859_1:
      sprintf(encoding,"ISO-8859-1");
      break;
    case CDIO_CDTEXT_CHARCODE_ASCII:
      sprintf(encoding,"ASCII");
      break;
    case CDIO_CDTEXT_CHARCODE_KANJI:
      sprintf(encoding,"Shift-JIS");
      break;
    }
    
  }
  
  return b_ret;
}
