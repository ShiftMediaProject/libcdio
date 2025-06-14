/*
  Copyright (C) 2003-2008, 2012-2013, 2017 Rocky Bernstein
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
#include "portable.h"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <cdio/cd_types.h>
#include "filemode.h"

#ifndef DATA_DIR
#define DATA_DIR "./data"
#endif
/* Set up a CD image to test on which is in the libcdio distribution. */
#define ISO9660_IMAGE_PATH DATA_DIR "/"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "copying.iso"

#define SKIP_TEST_RC 77

int
main(int argc, const char *argv[])
{
  iso9660_t *p_iso;
  iso9660_stat_t *p_statbuf;

  p_iso = iso9660_open(ISO9660_IMAGE);
  if (!p_iso) {
    fprintf(stderr, "Sorry, couldn't open ISO9660 image %s\n",
	    ISO9660_IMAGE);
    return 1;
  } else {
    unsigned int rc=0;
    uint8_t joliet_level = iso9660_ifs_get_joliet_level(p_iso);
    if (joliet_level != 0) {
      printf("Expecting joliet level to be 0, got %u\n", joliet_level);
      exit(10);
    } else {
      printf("-- No joliet, as expected.\n");
    }
    iso9660_close(p_iso);
    p_iso = iso9660_open_ext(ISO9660_IMAGE, ISO_EXTENSION_ALL);

    joliet_level = iso9660_ifs_get_joliet_level(p_iso);
    if ( joliet_level != 0) {
      printf("Expecting joliet level to still be 0, got %d\n", joliet_level);
      exit(11);
    } else {
      printf("-- joliet level 0 again, as expected.\n");
    }

    /* You make get different results looking up "/" versus "/." and the
       latter may give more complete information. "/" will take information
       from the PVD only, whereas "/." will force a directory read of "/" and
       find "." and in that Rock-Ridge information might be found which fills
       in more stat information that iso9660_fs_find_lsn also will find.
       . Ideally iso9660_fs_stat should be fixed. */
    p_statbuf = iso9660_ifs_stat (p_iso, "/.");

    if (NULL == p_statbuf) {
      fprintf(stderr,
	      "Could not get ISO-9660 file information for file /.\n");
      iso9660_close(p_iso);
      iso9660_stat_free(p_statbuf);
      exit(2);
    } else {
      /* Now try getting the statbuf another way */
      char buf[ISO_BLOCKSIZE];
      char *psz_path = NULL;
      const lsn_t i_lsn = p_statbuf->lsn;
      iso9660_stat_t *p_statbuf2 = iso9660_ifs_find_lsn (p_iso, i_lsn);
      iso9660_stat_t *p_statbuf3 =
	iso9660_ifs_find_lsn_with_path (p_iso, i_lsn, &psz_path);

      /* Compare the two statbufs. */
      if (p_statbuf->lsn != p_statbuf2->lsn ||
	  p_statbuf->total_size != p_statbuf2->total_size ||
	  p_statbuf->type != p_statbuf2->type) {

	  fprintf(stderr, "File stat information between fs_stat and "
		  "iso9660_ifs_find_lsn isn't the same\n");
	  rc = 3;
	  goto exit;
      }

      if (p_statbuf3->lsn != p_statbuf2->lsn ||
	  p_statbuf3->total_size != p_statbuf2->total_size ||
	  p_statbuf3->type != p_statbuf2->type) {
	rc = 4;
	goto exit;
      }

      if (psz_path != NULL) {
	if (0 != strncmp("/./", psz_path, strlen("/./"))) {
	  fprintf(stderr, "Path returned for ifs_find_lsn_with_path "
		  "is not correct should be /./, is %s\n", psz_path);
	  rc = 5;
	  goto exit;
	}
      } else {
	fprintf(stderr, "Path returned for fs_find_lsn_with_path is NULL\n");
	rc = 6;
	goto exit;
	exit(6);
      }

      /* Try reading from the directory. */
      memset (buf, 0, ISO_BLOCKSIZE);
      if ( ISO_BLOCKSIZE != iso9660_iso_seek_read (p_iso, buf, i_lsn, 1) )
	{
	  fprintf(stderr, "Error reading ISO 9660 file at lsn %lu\n",
		  (long unsigned int) p_statbuf->lsn);
	  rc = 7;
	  goto exit;
	}
  exit:
      if (psz_path != NULL)
      	free(psz_path);
      iso9660_stat_free(p_statbuf2);
      iso9660_stat_free(p_statbuf3);
      iso9660_stat_free(p_statbuf);
      iso9660_close(p_iso);
      exit(rc);
    }
  }

  exit(0);
}
