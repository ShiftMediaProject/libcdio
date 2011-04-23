/*
  Copyright (C) 2004, 2005, 2006, 2008, 2009, 2011
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

/* Simple program to list CD-Text info of a Compact Disc using
   libcdio.  See also corresponding C++ programs of similar names. */

#include <cdio/cdio.h>
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <cdio/cdtext.h>


static void 
print_cdtext_track_info(CdIo_t *p_cdio, track_t i_track, const char *psz_msg) {
  const cdtext_t *cdtext = cdio_get_cdtext(p_cdio, i_track);
  if (NULL != cdtext) {
    cdtext_field_t i;
    
    printf("%s\n", psz_msg);
    
    for (i=0; i < MAX_CDTEXT_FIELDS; i++) {
      if (cdtext->field[i]) {
	printf("\t%s: %s\n", cdtext_field2str(i), cdtext->field[i]);
      }
    }
  }
  
}
    
static void 
print_disc_info(CdIo_t *p_cdio, track_t i_tracks, track_t i_first_track) {
  track_t i_last_track = i_first_track+i_tracks;
  discmode_t cd_discmode = cdio_get_discmode(p_cdio);

  printf("%s\n", discmode2str[cd_discmode]);
  
  print_cdtext_track_info(p_cdio, 0, "\nCD-Text for Disc:");
  for ( ; i_first_track < i_last_track; i_first_track++ ) {
    char psz_msg[50];
    snprintf(psz_msg, sizeof(psz_msg), "CD-Text for Track %d:", i_first_track);
    print_cdtext_track_info(p_cdio, i_first_track, psz_msg);
  }
}

int
main(int argc, const char *argv[])
{
  track_t i_first_track;
  track_t i_tracks;
  CdIo_t *p_cdio       = cdio_open ("../test/cdda.cue", DRIVER_BINCUE);


  if (NULL == p_cdio) {
    printf("Couldn't open ../test/cdda.cue with BIN/CUE driver.\n");
  } else {
    i_first_track = cdio_get_first_track_num(p_cdio);
    i_tracks      = cdio_get_num_tracks(p_cdio);
    print_disc_info(p_cdio, i_tracks, i_first_track);
    cdio_destroy(p_cdio);
  }

  p_cdio = cdio_open (NULL, DRIVER_DEVICE);
  i_first_track = cdio_get_first_track_num(p_cdio);
  i_tracks      = cdio_get_num_tracks(p_cdio);

  if (NULL == p_cdio) {
    printf("Couldn't find CD\n");
    return 77;
  } else {
    print_disc_info(p_cdio, i_tracks, i_first_track);
  }

  cdio_destroy(p_cdio);
  
  return 0;
}
