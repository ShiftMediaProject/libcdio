/*
  $Id: sample5.c,v 1.1 2003/09/28 17:14:20 rocky Exp $

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

/* Simple program to show drivers installed and what the default 
   CD-ROM drive is and what CD drives are available. */
#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/cd_types.h>
#include <cdio/logging.h>

static void 
log_handler (cdio_log_level_t level, const char message[])
{
  switch(level) {
  case CDIO_LOG_DEBUG:
  case CDIO_LOG_INFO:
    return;
  default:
    printf("cdio %d message: %s\n", level, message);
  }
}

int
main(int argc, const char *argv[])
{
  char **cd_drives=NULL, **c;
  
  cdio_log_set_handler (log_handler);

  /* Print out a list of CD-drives */
  cd_drives = cdio_get_devices(NULL);
  for( c = cd_drives; *c != NULL; *c++ ) {
    printf("Drive %s\n", *c);
  }

  cdio_free_device_list(cd_drives);
  printf("-----\n");
  
  cd_drives = NULL;
  /* Print out a list of CD-drives the harder way. */
  cd_drives = cdio_get_devices_with_cap(NULL, CDIO_FS_MATCH_ALL, false);

  if (NULL != cd_drives) {
    for( c = cd_drives; *c != NULL; *c++ ) {
      printf("Drive %s\n", *c);
    }
    
    cdio_free_device_list(cd_drives);
  }
  
  
  printf("-----\n");
  printf("CD-DA drives...\n");
  cd_drives = NULL;
  /* Print out a list of CD-drives with CD-DA's in them. */
  cd_drives = cdio_get_devices_with_cap(NULL,  CDIO_FS_AUDIO, true);

  if (NULL != cd_drives) {
    for( c = cd_drives; *c != NULL; *c++ ) {
      printf("drive: %s\n", *c);
    }
    
    cdio_free_device_list(cd_drives);
  }
  
  printf("-----\n");
  cd_drives = NULL;
  printf("VCD drives...\n");
  /* Print out a list of CD-drives with VCD's in them. */
  cd_drives = cdio_get_devices_with_cap(NULL, 
(CDIO_FS_ANAL_SVCD|CDIO_FS_ANAL_CVD|CDIO_FS_ANAL_VIDEOCD|CDIO_FS_UNKNOWN),
					true);
  if (NULL != cd_drives) {
    for( c = cd_drives; *c != NULL; *c++ ) {
      printf("drive: %s\n", *c);
    }
  }

  cdio_free_device_list(cd_drives);
  return 0;
  
}
