/*
  $Id: sample8.c,v 1.2 2004/07/11 14:25:07 rocky Exp $

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
int
main(int argc, const char *argv[])
{
  CdIo *cdio = cdio_open ("../test/cdda.cue", DRIVER_BINCUE);

  if (NULL == cdio) {
    printf("Couldn't open ../test/cdda.cue with BIN/CUE driver \n");
    return 1;
  } else {
    const cdtext_t *cdtext = cdio_get_cdtext(cdio);

    if (NULL != cdtext) {
      printf("CD-TEXT Title: %s\n", 
	     cdtext->field[CDTEXT_TITLE]     ? 
	     cdtext->field[CDTEXT_TITLE]     : "not set");
      printf("CD-TEXT Performer: %s\n", 
	     cdtext->field[CDTEXT_PERFORMER] ? 
	     cdtext->field[CDTEXT_PERFORMER] : "not set"
	     );
    } else {
      printf("Didn't get CD-TEXT info.\n");
    }
  }

  cdio_destroy(cdio);
  
  return 0;
}
