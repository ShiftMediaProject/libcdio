/*
  $Id: sample7.c,v 1.5 2004/03/11 01:31:32 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  
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

/* Simple program to show using libiso9660 to extract a file from an
   ISO-9660 image.
 */

#if defined ( WIN32 )
#define ftruncate chsize
#endif

/* This is the ISO 9660 image. */
#define ISO9660_IMAGE_PATH "../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/copying.iso"

#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/iso9660.h>

#include <stdio.h>

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

int
main(int argc, const char *argv[])
{
  iso9660_stat_t *statbuf;
  FILE *outfd;
  int i;
  
  iso9660_t *iso = iso9660_open (ISO9660_IMAGE);
  
  if (NULL == iso) {
    fprintf(stderr, "Sorry, couldn't open ISO 9660 image %s\n", ISO9660_IMAGE);
    return 1;
  }

  statbuf = iso9660_ifs_stat_translate (iso, LOCAL_FILENAME);

  if (NULL == statbuf) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      LOCAL_FILENAME);
      return 2;
    }

  if (!(outfd = fopen (LOCAL_FILENAME, "wb")))
    {
      perror ("fopen()");
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */
  for (i = 0; i < statbuf->size; i += ISO_BLOCKSIZE)
    {
      char buf[ISO_BLOCKSIZE];

      memset (buf, 0, ISO_BLOCKSIZE);
      
      if ( ISO_BLOCKSIZE != iso9660_iso_seek_read (iso, buf, statbuf->lsn 
						   + (i / ISO_BLOCKSIZE),
						   1) )
      {
	fprintf(stderr, "Error reading ISO 9660 file at lsn %lu\n",
		(long unsigned int) statbuf->lsn + (i / ISO_BLOCKSIZE));
	return 4;
      }
      
      
      fwrite (buf, ISO_BLOCKSIZE, 1, outfd);
      
      if (ferror (outfd))
	{
	  perror ("fwrite()");
	  return 5;
	}
    }
  
  fflush (outfd);

  /* Make sure the file size has the exact same byte size. Without the
     truncate below, the file will a multiple of ISO_BLOCKSIZE.
   */
  if (ftruncate (fileno (outfd), statbuf->size))
    perror ("ftruncate()");

  fclose (outfd);
  iso9660_close(iso);
  return 0;
}
