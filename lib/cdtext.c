/*
    $Id: cdtext.c,v 1.4 2004/07/11 14:25:07 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
    toc reading routine adapted from cuetools
    Copyright (C) 2003 Svend Sanjay Sorensen <ssorensen@fastmail.fm>

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

#include <cdio/cdtext.h>
#include <cdio/logging.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*! Free memory assocated with cdtext*/
void 
cdtext_destroy (cdtext_t *cdtext)
{
  cdtext_field_t i;

  for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
    if (cdtext->field[i]) free(cdtext->field[i]);
  }
}

/*! 
  returns the CDTEXT value associated with key. NULL is returned
  if key is CDTEXT_INVALID or the field is not set.
 */
const char *
cdtext_get (cdtext_field_t key, const cdtext_t *cdtext)
{
  if (key == CDTEXT_INVALID) return NULL;
  return cdtext->field[key];
}

/*! Initialize a new cdtext structure.
  When the structure is no longer needed, release the 
  resources using cdtext_delete.
*/
void 
cdtext_init (cdtext_t *cdtext)
{
  cdtext_field_t i;

  for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
    cdtext->field[i] = NULL;
  }
}

/*!
  returns 0 if field is a CD-TEXT keyword, returns non-zero otherwise 
*/
cdtext_field_t
cdtext_is_keyword (const char *key)
{
  /* Note: the order and number items (except CDTEXT_INVALID) should
     match the cdtext_field_t enumeration. */
  const char *cdtext_keywords[] = 
    {
      "ARRANGER",     /* name(s) of the arranger(s) */
      "COMPOSER",     /* name(s) of the composer(s) */
      "DISC_ID",      /* disc identification information */
      "GENRE",        /* genre identification and genre information */
      "ISRC",         /* ISRC code of each track */
      "MESSAGE",      /* message(s) from the content provider and/or artist */
      "PERFORMER",    /* name(s) of the performer(s) */
      "SIZE_INFO",    /* size information of the block */
      "SONGWRITER",   /* name(s) of the songwriter(s) */
      "TITLE",        /* title of album name or track titles */
      "TOC_INFO",     /* table of contents information */
      "TOC_INFO2",    /* second table of contents information */
      "UPC_EAN",
    };

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
void cdtext_set (cdtext_field_t key, const char *value, cdtext_t *cdtext)
{
  if (NULL == value || key == CDTEXT_INVALID) return;
  
  if (cdtext->field[key]) free (cdtext->field[key]);
  cdtext->field[key] = strdup (value);
  
}

