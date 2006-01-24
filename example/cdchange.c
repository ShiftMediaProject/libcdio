/*
  $Id: cdchange.c,v 1.1 2006/01/24 00:15:33 rocky Exp $

  Copyright (C) 2005, 2006 Rocky Bernstein <rocky@panix.com>
  
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

/* Test media changed */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <sys/types.h>
#include <cdio/cdio.h>
int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio;

  if (argc > 1)
    p_cdio = cdio_open (argv[1], DRIVER_DEVICE);
  else {
    p_cdio = cdio_open (NULL, DRIVER_DEVICE);
    if (NULL == p_cdio) {
      printf("Couldn't find a driver.. leaving.\n");
      return 1;
    }
  }

  if (cdio_get_media_changed(p_cdio))
    printf("Initial media status: changed\n");
  else 
    printf("Initial media status: not changed\n");

  printf("Giving you 30 seconds to change CD if you want to do so.\n");
  sleep(30);
  if (cdio_get_media_changed(p_cdio))
    printf("Media status: changed\n");
  else 
    printf("Media status: not changed\n");

  cdio_destroy(p_cdio);
  return 0;
}
