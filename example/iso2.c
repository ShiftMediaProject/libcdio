/*
  $Id: iso2.c,v 1.9 2005/10/30 14:26:11 rocky Exp $

  Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
  
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

/* Simple program to show using libiso9660 to extract a file from a
   cue/bin CD-IMAGE.

   This program can be compiled with either a C or C++ compiler. In 
   the distribution we prefer C++ just to make sure we haven't broken
   things on the C++ side.
 */

/* This is the CD-image with an ISO-9660 filesystem */
#define ISO9660_IMAGE_PATH "../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/isofs-m1.cue"

#define ISO9660_PATH "/"
#define ISO9660_FILENAME "COPYING"
#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "portable.h"

#include <cdio/cdio.h>
#include <cdio/iso9660.h>

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
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#define my_exit(rc)				\
  fclose (p_outfd);				\
  free(p_statbuf);				\
  cdio_destroy(p_cdio);				\
  return rc;					\
  

int
main(int argc, const char *argv[])
{
  iso9660_stat_t *p_statbuf;
  FILE *p_outfd;
  int i;
  char const *psz_image;
  char const *psz_fname;
  char translated_name[256];
  char untranslated_name[256] = ISO9660_PATH;
  CdIo_t *p_cdio;
  
  if (argc > 1) 
    psz_image = argv[1];
  else 
    psz_image = ISO9660_IMAGE;

  if (argc > 2) 
    psz_fname = argv[2];
  else 
    psz_fname = ISO9660_FILENAME;

  strcat(untranslated_name, psz_fname);

  p_cdio = cdio_open (psz_image, DRIVER_UNKNOWN);
  if (!p_cdio) {
    fprintf(stderr, "Sorry, couldn't open %s\n", psz_image);
    return 1;
  }

  p_statbuf = iso9660_fs_stat (p_cdio, untranslated_name);

  if (NULL == p_statbuf) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      untranslated_name);
      cdio_destroy(p_cdio);
      return 2;
    }

  iso9660_name_translate(psz_fname, translated_name);
  
  if (!(p_outfd = fopen (translated_name, "wb")))
    {
      perror ("fopen()");
      cdio_destroy(p_cdio);
      free(p_statbuf);
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */
  for (i = 0; i < p_statbuf->size; i += ISO_BLOCKSIZE)
    {
      char buf[ISO_BLOCKSIZE];

      memset (buf, 0, ISO_BLOCKSIZE);
      
      if ( 0 != cdio_read_data_sectors (p_cdio, buf, 
					p_statbuf->lsn + (i / ISO_BLOCKSIZE),
					ISO_BLOCKSIZE, 1) )
      {
	fprintf(stderr, "Error reading ISO 9660 file at lsn %lu\n",
		(long unsigned int) p_statbuf->lsn + (i / ISO_BLOCKSIZE));
	my_exit(4);
      }
      
      
      fwrite (buf, ISO_BLOCKSIZE, 1, p_outfd);
      
      if (ferror (p_outfd))
	{
	  perror ("fwrite()");
	  my_exit(5);
	}
    }
  
  fflush (p_outfd);

  /* Make sure the file size has the exact same byte size. Without the
     truncate below, the file will a multiple of ISO_BLOCKSIZE.
   */
  if (ftruncate (fileno (p_outfd), p_statbuf->size))
    perror ("ftruncate()");

  printf("Extraction of file '%s' from '%s' successful.\n", 
	 translated_name, untranslated_name);

  my_exit(0);
}
