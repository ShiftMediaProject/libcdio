/*
  Copyright (C) 2013 Rocky Bernstein
  <rocky@gnu.org>

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

/* Tests reading ISO 9660 info from an ISO 9660 image.  */

#ifndef DATA_DIR
#define DATA_DIR "./data"
#endif
/* Set up a CD-DA image to test on which is in the libcdio distribution. */
#define ISO9660_IMAGE_PATH DATA_DIR "/"
#define ISO9660_IMAGE    ISO9660_IMAGE_PATH "copying.iso"
#define ISO9660_IMAGE_RR ISO9660_IMAGE_PATH "copying-rr.iso"

#define SKIP_TEST_RC 77

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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <cdio/cdio.h>
#include <cdio/iso9660.h>

int
main(int argc, const char *argv[])
{
  char const *psz_fname;
  iso9660_t *p_iso;

  psz_fname = ISO9660_IMAGE;
  p_iso = iso9660_open_ext(psz_fname, ISO_EXTENSION_ROCK_RIDGE);

  if (NULL == p_iso) {
    fprintf(stderr, "Sorry, couldn't open %s as an ISO-9660 image\n",
	    psz_fname);
    return 1;
  }

  if (iso9660_have_rr(p_iso, 0) != nope) {
    fprintf(stderr, "-- Should not find Rock Ridge for %s\n", psz_fname);
    return 2;
  } else {
    printf("-- Good! Did not find Rock Ridge in %s\n", psz_fname);
  }

  iso9660_close(p_iso);

  psz_fname = ISO9660_IMAGE_RR;
  p_iso = iso9660_open_ext(psz_fname, ISO_EXTENSION_ROCK_RIDGE);

  if (NULL == p_iso) {
    fprintf(stderr, "Sorry, couldn't open %s as an ISO-9660 image\n",
	    psz_fname);
    return 3;
  }

  if (iso9660_have_rr(p_iso, 0) == yep) {
      printf("-- Good! found Rock Ridge in %s\n", psz_fname);
  } else {
    fprintf(stderr, "-- Should have found Rock Ridge for %s\n", psz_fname);
    return 2;
  }

  if (iso9660_have_rr(p_iso, 3) == yep) {
      printf("-- Good! Found Rock Ridge again in %s\n", psz_fname);
  } else {
    fprintf(stderr, "-- Should have found Rock Ridge for %s\n", psz_fname);
    return 3;
  }

  iso9660_close(p_iso);

  return 0;
}
