/*
  $Id: testtoc.c,v 1.2 2004/05/08 20:36:02 rocky Exp $

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
   Regression test for cdio_tocfile.
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

#define NUM_GOOD_TOCS 14
#define NUM_BAD_TOCS 6
int
main(int argc, const char *argv[])
{
  const char *toc_file[NUM_GOOD_TOCS] = {
    "t9.toc", 
    "t2.toc", 
    "t3.toc", 
    "t4.toc", 
    "t5.toc", 
    "t6.toc", 
    "t7.toc", 
    "t8.toc", 
    "t9.toc",
    "data1.toc",
    "data2.toc",
    "data5.toc",
    "data6.toc",
    "data7.toc"
  };

  const char *badtoc_file[NUM_BAD_TOCS] = {
    "bad-msf-1.toc", 
    "bad-msf-2.toc",
    "bad-cat1.toc", 
    "bad-cat2.toc",
    "bad-cat3.toc",
    "bad-mode1.toc"
  };
  int ret=0;
  unsigned int i;
  
  cdio_loglevel_default = CDIO_LOG_INFO;
  for (i=0; i<NUM_GOOD_TOCS; i++) {
    if (!cdio_is_tocfile(toc_file[i])) {
      printf("Incorrect: %s doesn't parse as a cdrdao TOC file.\n", 
	     toc_file[i]);
      ret=i+1;
    } else {
      printf("Correct: %s parses as a cdrdao TOC file.\n", 
	     toc_file[i]);
    }
  }

  for (i=0; i<NUM_BAD_TOCS; i++) {
    if (!cdio_is_tocfile(badtoc_file[i])) {
      printf("Correct: %s doesn't parse as a cdrdao TOC file.\n", 
	     badtoc_file[i]);
    } else {
      printf("Incorrect: %s parses as a cdrdao TOC file.\n", 
	     badtoc_file[i]);
      ret+=50*i+1;
      break;
    }
  }
  
  return ret;
}
