/*
  $Id: testbincue.c,v 1.1 2004/07/09 20:47:08 rocky Exp $

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

/* 
   Regression test for cdio_binfile().
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <cdio/cdio.h>
#include <cdio/logging.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#define NUM_GOOD_CUES 2
#define NUM_BAD_CUES 3
int
main(int argc, const char *argv[])
{
  const char *cue_file[NUM_GOOD_CUES] = {
    "cdda.cue", 
    "isofs-m1.cue", 
  };

  const char *badcue_file[NUM_BAD_CUES] = {
    "bad-cat1.cue", 
    "bad-cat2.cue", 
    "bad-cat3.cue", 
  };
  int ret=0;
  unsigned int i;

  cdio_loglevel_default = (argc > 1) ? CDIO_LOG_DEBUG : CDIO_LOG_INFO;
  for (i=0; i<NUM_GOOD_CUES; i++) {
    if (!cdio_is_cuefile(cue_file[i])) {
      printf("Incorrect: %s doesn't parse as a CDRWin CUE file.\n", 
	     cue_file[i]);
      ret=i+1;
    } else {
      printf("Correct: %s parses as a CDRWin CUE file.\n", 
	     cue_file[i]);
    }
  }

  for (i=0; i<NUM_BAD_CUES; i++) {
    if (!cdio_is_cuefile(badcue_file[i])) {
      printf("Correct: %s doesn't parse as a CDRWin CUE file.\n", 
	     badcue_file[i]);
    } else {
      printf("Incorrect: %s parses as a CDRWin CUE file.\n", 
	     badcue_file[i]);
      ret+=50*i+1;
      break;
    }
  }
  
  return ret;
}
