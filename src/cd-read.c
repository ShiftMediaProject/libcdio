/*
  $Id: cd-read.c,v 1.1 2003/09/17 12:13:07 rocky Exp $

  Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
  
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

/* Program to debug read routines audio, mode1, mode2 forms 1 & 2. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
#include <stdlib.h>
#include <ctype.h>

static void
hexdump (uint8_t * buffer, unsigned int len)
{
  unsigned int i;
  for (i = 0; i < len; i++, buffer++)
    {
      if (i % 16 == 0)
	printf ("0x%04x: ", i);
      printf ("%02x", *buffer);
      if (i % 2 == 1)
	printf (" ");
      if (i % 16 == 15) {
	printf ("  ");
	uint8_t *p; 
	for (p=buffer-15; p <= buffer; p++) {
	  printf("%c", isprint(*p) ?  *p : '.');
	}
	printf ("\n");
      }
    }
  printf ("\n");
}

int
main(int argc, const char *argv[])
{
  uint8_t buffer[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  lsn_t lsn;
  
  if (argc != 4) {
    printf("need to give 3 things: device/filename, type of read, & lsn\n");
    return 10;
  }
  
  CdIo *cdio = cdio_open (argv[1], DRIVER_UNKNOWN);

  lsn = atoi(argv[3]);
  
  switch (argv[2][0]) {
  case '1':
    cdio_read_audio_sector(cdio, &buffer, lsn);
    break;
  case '2':
    cdio_read_mode1_sector(cdio, &buffer, lsn, false);
    break;
  case '3':
    cdio_read_mode1_sector(cdio, &buffer, 16, true);
    break;
  case '4':
    cdio_read_mode2_sector(cdio, &buffer, 0, false);
    break;
  case '5':
    cdio_read_mode2_sector(cdio, &buffer, 16, true);
    break;
  default:
    printf("Expecting 1..5 as the second argument.\n");
    return 20;
  }

  hexdump(buffer, CDIO_CD_FRAMESIZE_RAW);
  cdio_destroy(cdio);
  return 0;
}
