/*
  $Id: sample2.c,v 1.7 2004/04/23 22:10:52 rocky Exp $

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

/* Simple program to show drivers installed and what the default 
   CD-ROM drive is. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <cdio/cdio.h>
int
main(int argc, const char *argv[])
{
  CdIo *cdio = cdio_open (NULL, DRIVER_UNKNOWN);
  driver_id_t driver_id;
  
  if (NULL != cdio) {
    char *default_device = cdio_get_default_device(cdio);

    printf("The driver selected is %s\n", cdio_get_driver_name(cdio));

    if (default_device) {
      cdio_drive_cap_t i_drive_cap =  cdio_get_drive_cap(cdio);
      printf("The default device for this driver is %s\n", default_device);
      printf("drive capability in hex: %x\n", i_drive_cap);
      if (CDIO_DRIVE_ERROR == i_drive_cap) {
	printf("Error in getting drive properties\n");
      } else if (CDIO_DRIVE_UNKNOWN == i_drive_cap) {
	printf("Can't determine drive properties\n");
      } else if (CDIO_DRIVE_FILE == i_drive_cap) {
	printf("Disc-image file\n");
      } else {
	if (i_drive_cap & CDIO_DRIVE_CD_R) 
	  printf("Drive can read CD-ROM\n");
	if (i_drive_cap & CDIO_DRIVE_CD_RW) 
	  printf("Drive can write CD-ROM\n");
	if (i_drive_cap & CDIO_DRIVE_DVD) 
	  printf("Drive can read DVD\n");
	if (i_drive_cap & CDIO_DRIVE_DVD_R) 
	  printf("Drive can write DVD-R\n");
	if (i_drive_cap & CDIO_DRIVE_DVD_RAM) 
	  printf("Drive can write DVD-RAM\n");
      }
    }
      
    free(default_device);
    cdio_destroy(cdio);
    printf("\n");
  } else {
    printf("Problem in trying to find a driver.\n\n");
  }

  for (driver_id=CDIO_MIN_DRIVER; driver_id<=CDIO_MAX_DRIVER; driver_id++)
    if (cdio_have_driver(driver_id))
      printf("We have: %s\n", cdio_driver_describe(driver_id));
    else
      printf("We don't have: %s\n", cdio_driver_describe(driver_id));
  return 0;
}
