/*
  $Id: sample1.c,v 1.2 2003/08/09 11:52:00 rocky Exp $

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

/* Simple program to list track numbers and logical sector numbers of
   a Compact Disc using libcdio. */
#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
int
main(int argc, const char *argv[])
{
  CdIo *cdio = cdio_open ("/dev/cdrom", DRIVER_UNKNOWN);
  track_t first_track_num = cdio_get_first_track_num(cdio);
  track_t num_tracks      = cdio_get_num_tracks(cdio);
  int j, i=first_track_num;

  printf("CD-ROM Track List (%i - %i)\n", first_track_num, num_tracks);

  printf("  #:  LSN\n");
  
  for (j = 0; j < num_tracks; i++, j++) {
    lsn_t lsn = cdio_get_track_lsn(cdio, i);
    if (CDIO_INVALID_LSN != lsn)
	printf("%3d: %06d\n", (int) i, lsn);
  }
  printf("%3X: %06d  leadout\n", CDIO_CDROM_LEADOUT_TRACK, 
	 cdio_get_track_lsn(cdio, CDIO_CDROM_LEADOUT_TRACK));
  cdio_destroy(cdio);
  return 0;
}
