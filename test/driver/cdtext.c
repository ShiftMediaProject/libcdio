/* -*- C -*-
  Copyright (C) 2018
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
  Performs some regression tests concerning:
   * the association of numerical language codes and language names, and
   * the handling of all language codes, valid and invalid.
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

static int
language_code_tests(void) {
  cdtext_lang_t num, re_num;
  char *text;
  int i;
  int rc=0;
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
      rc++;
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
	rc++;
      }
      if (CDTEXT_LANGUAGE_INVALID != re_num) {
        fprintf(stderr,
                "cdtext_str2lang(cdtext_lang2str()) test: invalid code 0x%2.2X yields 0x%2.2X, expected 0x%2.2X\n",
                (unsigned int) i, (unsigned int) re_num,
                (unsigned int) CDTEXT_LANGUAGE_INVALID);
	rc++;
      }
    } else {
      if (re_num != num) {
        fprintf(stderr,
                "cdtext_str2lang(cdtext_lang2str()) test: code 0x%2.2X yields '%s' yields 0x%2.2X\n",
                (unsigned int) i, text, (unsigned int) re_num);
	rc++;
      }
    }
  }
  return rc;
}

int
main(int argc, const char *argv[]) {
  int rc = language_code_tests();
  return rc;
}
