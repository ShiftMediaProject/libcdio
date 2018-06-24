/* -*- C -*-
  Copyright (C) 2004, 2006, 2008, 2010, 2011, 2012, 2017
  Rocky Bernstein <rocky@gnu.org>

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

/*
   Example demonstrating the parsing of raw CD-Text files
   Output adapted from cd-info.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#define __CDIO_CONFIG_H__ 1
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <cdio/cdio.h>

/* Maximum CD-TEXT payload: 8 text blocks with 256 packs of 18 bytes each */
#define CDTEXT_LEN_BINARY_MAX (8 * 256 * 18)

static void
print_cdtext_track_info(cdtext_t *p_cdtext, track_t i_track, const char *psz_msg) {

  if (NULL != p_cdtext) {
    cdtext_field_t i;

    printf("%s\n", psz_msg);

    for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
      if (cdtext_get_const(p_cdtext, i, i_track)) {
        printf("\t%s: %s\n", cdtext_field2str(i), cdtext_get_const(p_cdtext, i, i_track));
      }
    }
  }
}

static void
print_cdtext_info(cdtext_t *p_cdtext) {
  track_t i_first_track = 0;
  track_t i_last_track = 0;
  cdtext_lang_t *languages;
  cdtext_genre_t genre;

  int i, j;

  languages = cdtext_list_languages_v2(p_cdtext);
  if (NULL == languages)
    return;
  for(i=0; i<8; i++)
    if ( CDTEXT_LANGUAGE_BLOCK_UNUSED != languages[i]
         && cdtext_set_language_index(p_cdtext, i))
    {
      printf("\nLanguage %d '%s':\n", i, cdtext_lang2str(languages[i]));

      print_cdtext_track_info(p_cdtext, 0, "CD-TEXT for Disc:");
      genre = cdtext_get_genre(p_cdtext);
      if ( CDTEXT_GENRE_UNUSED != genre)
        printf("\tGENRE_CODE: %d (%s)\n", genre, cdtext_genre2str(genre));

      i_first_track = cdtext_get_first_track(p_cdtext);
      i_last_track = cdtext_get_last_track(p_cdtext);

      for ( j = i_first_track ; j <= i_last_track; j++ ) {
        char msg[50];
        sprintf(msg, "CD-TEXT for Track %2d:", j);
        print_cdtext_track_info(p_cdtext, j, msg);
      }
    }
}

static cdtext_t *
read_cdtext(const char *path) {
  FILE *fp;
  size_t size;
  uint8_t *cdt_data = NULL;
  cdtext_t *cdt;

  cdt_data = calloc(CDTEXT_LEN_BINARY_MAX + 4, 1);
  if (NULL == cdt_data) {
    fprintf(stderr, "could not allocate memory for cdt_data buffer\n");
    exit(4);
  }
  fp = fopen(path, "rb");
  if (NULL == fp) {
    fprintf(stderr, "could not open file `%s'\n", path);
    exit(3);
  }

  size = fread(cdt_data, 1, CDTEXT_LEN_BINARY_MAX + 4, fp);
  fclose(fp);

  if (size < 5) {
    fprintf(stderr, "file `%s' is too small to contain CD-TEXT\n", path);
    exit(1);
  }

  /* Truncate header when it is too large. The standard is ambiguous here*/
  if (cdt_data[0] > 0x80) {
    size -= 4;
  }

  /* ignore trailing 0 */
  if (1 == size % 18)
    size -= 1;

  /* init cdtext */
  cdt = cdtext_init ();
  if(0 != cdtext_data_init(cdt, cdt_data, size)) {
    fprintf(stderr, "failed to parse CD-Text file `%s'\n", path);
    free(cdt);
    exit(2);
  }

  free(cdt_data);
  return cdt;
}

/*
  This function performs regression tests concerning:
    * the association of numerical language codes and language names, and
    * the handling of all language codes, valid and invalid 
*/
static void
language_code_tests(void) {
  cdtext_lang_t num, re_num;
  char *text;
  int i;
  static cdtext_lang_t spot_codes[] = {
    CDTEXT_LANGUAGE_ENGLISH, CDTEXT_LANGUAGE_WALLON , CDTEXT_LANGUAGE_ZULU,
    CDTEXT_LANGUAGE_MALAGASAY, CDTEXT_LANGUAGE_AMHARIC
  };
  static const char *spot_names[] = {
    "English", "Wallon", "Zulu", "Malagasay", "Amharic"
  };
  static int spots = 5;

  for (i = 0; i < spots; i++) {
    if (0 != strcmp(cdtext_lang2str(spot_codes[i]), spot_names[i])) {
      fprintf(stderr,
  "cdtext_lang2str() test: idx %d , code 0x%2.2X : expected '%s' , got '%s'\n",
              i, (unsigned int) spot_codes[i], spot_names[i],
              cdtext_lang2str(spot_codes[i]));
    }
  }

  for (i = 0; i <= 0xFF; i++) {
    num = i;
    text = (char *) cdtext_lang2str(num);
    re_num = cdtext_str2lang(text);
    if ((i > CDTEXT_LANGUAGE_WALLON && i < CDTEXT_LANGUAGE_ZULU)
        || i > CDTEXT_LANGUAGE_AMHARIC) {
      if (0 != strcmp(text, "INVALID")) {
        fprintf(stderr,
                "cdtext_lang2str() test: invalid code 0x%2.2X yields '%s'\n",
                (unsigned int) i, text);
      }
      if (CDTEXT_LANGUAGE_INVALID != re_num) {
        fprintf(stderr,
                "cdtext_str2lang(cdtext_lang2str()) test: invalid code 0x%2.2X yields 0x%2.2X, expected 0x%2.2X\n",
                (unsigned int) i, (unsigned int) re_num,
                (unsigned int) CDTEXT_LANGUAGE_INVALID);
      }
    } else {
      if (re_num != num) {
        fprintf(stderr,
                "cdtext_str2lang(cdtext_lang2str()) test: code 0x%2.2X yields '%s' yields 0x%2.2X\n",
                (unsigned int) i, text, (unsigned int) re_num);
      }
    }
  }

}

int
main(int argc, const char *argv[]) {
  cdtext_t *cdt;

  char *cdt_path = NULL;

  if (argc > 1) {
    cdt_path = (char *) argv[1];
  }else{
    fprintf(stderr, "no CD-Text file given\n");
    exit(77);
  }

  cdt = read_cdtext(cdt_path);
  print_cdtext_info(cdt);
  language_code_tests();
  free(cdt);

  return 0;
}
