/*
    $Id: cdtext.c,v 1.1 2004/07/09 01:05:32 rocky Exp $

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

/* Private type */
struct cdtext {
	const char *key;
	char *value;
};

const cdtext_t cdtext[] = {
    {"TITLE",      NULL},
    {"PERFORMER",  NULL},
    {"SONGWRITER", NULL},
    {"COMPOSER",   NULL},
    {"ARRANGER",   NULL},
    {"MESSAGE",    NULL},
    {"DISC_ID",    NULL},
    {"GENRE",      NULL},
    {"TOC_INFO",   NULL},
    {"TOC_INFO2",  NULL},
    {"UPC_EAN",    NULL},
    {"ISRC",       NULL},
    {"SIZE_INFO",  NULL},
    {NULL,         NULL}
  };
  
cdtext_t *cdtext_init ()
{
  cdtext_t *new_cdtext = NULL;
  
  new_cdtext = (cdtext_t *) calloc (sizeof (cdtext) / sizeof (cdtext_t), 
				    sizeof (cdtext_t));
  if (NULL == new_cdtext)
    cdio_warn("problem allocating memory");
  else
    memcpy (new_cdtext, cdtext, sizeof(cdtext));
  
  return (new_cdtext);
}


/*! Free memory assocated with cdtext*/
void cdtext_delete (cdtext_t *cdtext)
{
  int i;
  
  if (NULL != cdtext) {
    for (i = 0; NULL != cdtext[i].key; i++)
      free (cdtext[i].value);
    free (cdtext);
  }
}

/*!
  returns 0 if field is a CD-TEXT keyword, returns non-zero otherwise 
*/
int 
cdtext_is_keyword (char *key)
{
  const char *cdtext_keywords[] = 
    {
      "ARRANGER",     /* name(s) of the arranger(s) */
      "COMPOSER",     /* name(s) of the composer(s) */
      "DISC ID",      /* disc identification information */
      "GENRE",        /* genre identification and genre information */
      "ISRC",         /* ISRC code of each track */
      "MESSAGE",      /* message(s) from the content provider and/or artist */
      "PERFORMER",    /* name(s) of the performer(s) */
      "SIZE_INFO",    /* size information of the block */
      "SONGWRITER",   /* name(s) of the songwriter(s) */
      "TITLE",        /* title of album name or track titles */
      "TOC_INFO",     /* table of contents information */
      "TOC_INFO2"     /* second table of contents information */
      "UPC_EAN",
    };
  
  char *item;
  
  item = bsearch(key, 
		 cdtext_keywords, 12,
		 sizeof (char *), 
		 (int (*)(const void *, const void *))
		 strcmp);
  return (NULL != item) ? 0 : 1;
}

/*! sets cdtext's keyword entry to field.
 */
void cdtext_set (char *key, char *value, cdtext_t *cdtext)
{
  if (NULL != value)	/* don't pass NULL to strdup! */
    for (; NULL != cdtext->key; cdtext++)
      if (0 == strcmp (cdtext->key, key)) {
	free (cdtext->value);
	cdtext->value = strdup (value);
      }
}

