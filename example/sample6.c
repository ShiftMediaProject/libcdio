/*
  $Id: sample6.c,v 1.3 2004/01/29 04:22:49 rocky Exp $

  Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
  
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
 */
/* This is the CD-image with an ISO-9660 filesystem */
#define ISO9660_IMAGE_PATH "../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/isofs-m1.cue"

#define ISO9660_FILENAME "/COPYING.;1"
#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/iso9660.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int
main(int argc, const char *argv[])
{
  iso9660_stat_t *statbuf;
  FILE *outfd;
  int i;
  
  CdIo *cdio = cdio_open (ISO9660_IMAGE, DRIVER_BINCUE);
  
  if (NULL == cdio) {
    fprintf(stderr, "Sorry, couldn't open BIN/CUE image %s\n", ISO9660_IMAGE);
    return 1;
  }

  statbuf = iso9660_fs_stat (cdio, ISO9660_FILENAME, false);

  if (NULL == statbuf) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      ISO9660_FILENAME);
      return 2;
    }

  if (!(outfd = fopen ("copying", "wb")))
    {
      perror ("fopen()");
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */
  for (i = 0; i < statbuf->size; i += ISO_BLOCKSIZE)
    {
      char buf[ISO_BLOCKSIZE];

      memset (buf, 0, ISO_BLOCKSIZE);
      
      if ( 0 != cdio_read_mode1_sector (cdio, buf, 
					statbuf->lsn + (i / ISO_BLOCKSIZE),
					false) )
      {
	fprintf(stderr, "Error reading ISO 9660 file at lsn %d\n",
		statbuf->lsn + (i / ISO_BLOCKSIZE));
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
  cdio_destroy(cdio);
  return 0;
}
