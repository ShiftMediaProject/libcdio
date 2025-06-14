/*
  Copyright (C) 2004-2008, 2011, 2017
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

/* Simple program to show using libiso9660 to list files in a directory of
   an ISO-9660 image and give some iso9660 information. See the code
   to iso-info for a more complete example.

   If a single argument is given, it is used as the ISO 9660 image to
   use in the listing. Otherwise a compiled-in default ISO 9660 image
   name (that comes with the libcdio distribution) will be used.
 */

/* Set up a CD-DA image to test on which is in the libcdio distribution. */
#define ISO9660_IMAGE_PATH "../test/"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "copying.iso"

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

#define print_vd_info(title, fn)	  \
  if (fn(p_iso, &psz_str)) {		  \
    printf(title ": %s\n", psz_str);	  \
  }					  \
  free(psz_str);			  \
  psz_str = NULL;


int
main(int argc, const char *argv[])
{
  CdioISO9660FileList_t *p_entlist;
  CdioListNode_t *p_entnode;
  char const *psz_fname;
  iso9660_t *p_iso;
  const char *psz_path = "/";

  if (argc > 1)
    psz_fname = argv[1];
  else
    psz_fname = ISO9660_IMAGE;

  p_iso = iso9660_open (psz_fname);

  if (NULL == p_iso) {
    fprintf(stderr, "Sorry, couldn't open %s as an ISO-9660 image\n",
	    psz_fname);
    return 1;
  }

  /* Show basic CD info from the Primary Volume Descriptor. */
  {
    char *psz_str = NULL;
    print_vd_info("Application", iso9660_ifs_get_application_id);
    print_vd_info("Preparer   ", iso9660_ifs_get_preparer_id);
    print_vd_info("Publisher  ", iso9660_ifs_get_publisher_id);
    print_vd_info("System     ", iso9660_ifs_get_system_id);
    print_vd_info("Volume     ", iso9660_ifs_get_volume_id);
    print_vd_info("Volume Set ", iso9660_ifs_get_volumeset_id);
  }

  p_entlist = iso9660_ifs_readdir (p_iso, psz_path);

  /* Iterate over the list of nodes that iso9660_ifs_readdir gives  */

  if (p_entlist) {
    _CDIO_LIST_FOREACH (p_entnode, p_entlist)
    {
      char filename[4096];
      double total_size;
      iso9660_stat_t *p_statbuf =
	(iso9660_stat_t *) _cdio_list_node_data (p_entnode);
      iso9660_name_translate(p_statbuf->filename, filename);

      total_size = (double) p_statbuf->total_size;
      printf("%s [LSN %8d] %12.f %s%s\n",
	     _STAT_DIR == p_statbuf->type ? "d" : "-",
	     p_statbuf->lsn, total_size,  psz_path, filename);
    }

    iso9660_filelist_free(p_entlist);
  }

  iso9660_close(p_iso);
  return 0;
}
