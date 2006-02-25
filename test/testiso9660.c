/*
    $Id: testiso9660.c,v 1.6 2006/02/25 12:02:02 rocky Exp $

    Copyright (C) 2003, 2006 Rocky Bernstein <rocky@panix.com>

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
/* Tests ISO9660 library routines. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#include <cdio/iso9660.h>

int
main (int argc, const char *argv[])
{
  int c;
  int i;
  int i_bad = 0;
  char dst[100];
  char *dst_p;
  int achars[] = {'!', '"', '%', '&', '(', ')', '*', '+', ',', '-', '.',
                  '/', '?', '<', '=', '>'};
  for (c='A'; c<='Z'; c++ ) {
    if (!iso9660_isdchar(c)) {
      printf("Failed iso9660_isdchar test on %c\n", c);
      i_bad++;
    }
    if (!iso9660_isachar(c)) {
      printf("Failed iso9660_isachar test on %c\n", c);
      i_bad++;
    }
  }

  if (i_bad) return i_bad;
  
  for (c='0'; c<='9'; c++ ) {
    if (!iso9660_isdchar(c)) {
      printf("Failed iso9660_isdchar test on %c\n", c);
      i_bad++;
    }
    if (!iso9660_isachar(c)) {
      printf("Failed iso9660_isachar test on %c\n", c);
      i_bad++;
    }
  }

  if (i_bad) return i_bad;

  for (i=0; i<=13; i++ ) {
    c=achars[i];
    if (iso9660_isdchar(c)) {
      printf("Should not pass iso9660_isdchar test on %c\n", c);
      i_bad++;
    }
    if (!iso9660_isachar(c)) {
      printf("Failed iso9660_isachar test on symbol %c\n", c);
      i_bad++;
    }
  }

  if (i_bad) return i_bad;

  /* Test iso9660_strncpy_pad */
  dst_p = iso9660_strncpy_pad(dst, "1_3", 5, ISO9660_DCHARS);
  if ( 0 != strncmp(dst, "1_3  ", 5) ) {
    printf("Failed iso9660_strncpy_pad DCHARS\n");
    return 31;
  }
  dst_p = iso9660_strncpy_pad(dst, "ABC!123", 2, ISO9660_ACHARS);
  if ( 0 != strncmp(dst, "AB", 2) ) {
    printf("Failed iso9660_strncpy_pad ACHARS truncation\n");
    return 32;
  }

  /* Test iso9660_dirname_valid_p */
  if ( iso9660_dirname_valid_p("/NOGOOD") ) {
    printf("/NOGOOD should fail iso9660_dirname_valid_p\n");
    return 33;
  }
  if ( iso9660_dirname_valid_p("LONGDIRECTORY/NOGOOD") ) {
    printf("LONGDIRECTORY/NOGOOD should fail iso9660_dirname_valid_p\n");
    return 34;
  }
  if ( !iso9660_dirname_valid_p("OKAY/DIR") ) {
    printf("OKAY/DIR should pass iso9660_dirname_valid_p\n");
    return 35;
  }
  if ( iso9660_dirname_valid_p("OKAY/FILE.EXT") ) {
    printf("OKAY/FILENAME.EXT should fail iso9660_dirname_valid_p\n");
    return 36;
  }

  /* Test iso9660_pathname_valid_p */
  if ( !iso9660_pathname_valid_p("OKAY/FILE.EXT") ) {
    printf("OKAY/FILE.EXT should pass iso9660_dirname_valid_p\n");
    return 37;
  }
  if ( iso9660_pathname_valid_p("OKAY/FILENAMETOOLONG.EXT") ) {
    printf("OKAY/FILENAMETOOLONG.EXT should fail iso9660_dirname_valid_p\n");
    return 38;
  }
  if ( iso9660_pathname_valid_p("OKAY/FILE.LONGEXT") ) {
    printf("OKAY/FILE.LONGEXT should fail iso9660_dirname_valid_p\n");
    return 39;
  }

  dst_p = iso9660_pathname_isofy ("this/file.ext", 1);
  if ( 0 != strncmp(dst_p, "this/file.ext;1", 16) ) {
    printf("Failed iso9660_pathname_isofy\n");
    free(dst_p);
    return 40;
  }
  free(dst_p);

  /* Test get/set date */
  {
    struct tm *p_tm, tm;
    iso9660_dtime_t dtime;
    time_t now = time(NULL);

    memset(&dtime, 0, sizeof(dtime));
    p_tm = localtime(&now);
    iso9660_set_dtime(p_tm, &dtime);
    iso9660_get_dtime(&dtime, true, &tm);
    if ( memcmp(p_tm, &tm, sizeof(tm)) != 0 ) {
      printf("Local time retrieved with iso9660_get_dtime not same as\n");
      printf("that set with iso9660_set_dtime().\n");
      return 41;
    }
    p_tm = gmtime(&now);
    iso9660_set_dtime(p_tm, &dtime);
    iso9660_get_dtime(&dtime, false, &tm);
    if ( memcmp(p_tm, &tm, sizeof(tm)) != 0 ) {
      printf("GMT time retrieved with iso9660_get_dtime() not same as that\n");
      printf("set with iso9660_set_dtime().\n");
      return 42;
    }
  }

  return 0;
}
