/*
  $Id: sample2.c,v 1.1 2003/08/03 20:02:04 rocky Exp $

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
   CD-ROM drive is. */
#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
int
main(int argc, const char *argv[])
{
  CdIo *cdio = cdio_open (NULL, DRIVER_UNKNOWN);
  driver_id_t driver_id;
  
  if (NULL != cdio) {
    printf("The driver selected is %s\n", cdio_get_driver_name(cdio));
    printf("The default device for this driver is %s\n\n", 
	   cdio_get_default_device(cdio));
    cdio_destroy(cdio);
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


