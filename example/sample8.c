/*
  $Id: sample8.c,v 1.7 2004/07/24 06:11:30 rocky Exp $

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

/* Simple program to list CDTEXT info of  a Compact Disc using libcdio. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/cdtext.h>


static void 
print_cdtext_track_info(CdIo *cdio, track_t i_track, const char *message) {
  const cdtext_t *cdtext    = cdio_get_cdtext(cdio, 0);
  if (NULL != cdtext) {
    cdtext_field_t i;
    
    printf("%s\n", message);
    
    for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
      if (cdtext->field[i]) {
	printf("\t%s: %s\n", cdtext_field2str(i), cdtext->field[i]);
      }
    }
  }
  
}
    
static void 
print_disc_info(CdIo *cdio, track_t i_tracks, track_t i_first_track) {
  track_t i_last_track = i_first_track+i_tracks;
  discmode_t cd_discmode = cdio_get_discmode(cdio);

  switch (cd_discmode) {
  case CDIO_DISC_MODE_CD_DA: 
    printf("Disc is CD-DA.\n");
    break;
  case CDIO_DISC_MODE_CD_DATA_1: 
    printf("Disc is CD-ROM mode 1.\n");
    break;
  case CDIO_DISC_MODE_CD_DATA_2: 
    printf("Disc is CD-ROM mode 2.\n");
    break;
  case CDIO_DISC_MODE_CD_XA_2_1: 
    printf("Disc is CD-XA form2 mode 1.\n");
    break;
  case CDIO_DISC_MODE_CD_XA_2_2: 
    printf("Disc is CD-XA form2 mode 2.\n");
    break;
  case CDIO_DISC_MODE_CD_MIXED: 
    printf("Disc is mixed-mode.\n");
    break;
  case CDIO_DISC_MODE_DVD: 
    printf("Disc is some sort of DVD.\n");
    break;
  case CDIO_DISC_MODE_NO_INFO: 
    printf("Don't now what disc is. Perhaps driver doesn't implement.\n");
    break;
  case CDIO_DISC_MODE_ERROR: 
    printf("Error getting CD info or request not supported by drive.\n");
    break;
  }
  
  print_cdtext_track_info(cdio, 0, "\nCD-TEXT for Disc:");
  for ( ; i_first_track < i_last_track; i_first_track++ ) {
    char msg[50];
    sprintf(msg, "CD-TEXT for Track %d:", i_first_track);
    print_cdtext_track_info(cdio, i_first_track, msg);
  }
}

int
main(int argc, const char *argv[])
{
  track_t i_first_track;
  track_t i_tracks;
  CdIo *cdio       = cdio_open ("../test/cdda.cue", DRIVER_BINCUE);


  if (NULL == cdio) {
    printf("Couldn't open ../test/cdda.cue with BIN/CUE driver.\n");
  } else {
    i_first_track = cdio_get_first_track_num(cdio);
    i_tracks      = cdio_get_num_tracks(cdio);
    print_disc_info(cdio, i_tracks, i_first_track);
    cdio_destroy(cdio);
  }

  cdio = cdio_open (NULL, DRIVER_UNKNOWN);
  i_first_track = cdio_get_first_track_num(cdio);
  i_tracks      = cdio_get_num_tracks(cdio);

  if (NULL == cdio) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    print_disc_info(cdio, i_tracks, i_first_track);
  }

  cdio_destroy(cdio);
  
  return 0;
}
