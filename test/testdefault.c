/*
  $Id: testdefault.c,v 1.2 2003/10/02 02:59:58 rocky Exp $

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

/* 
   Regression test for cdio_get_devices, cdio_get_devices_with_cap(),
   and cdio_free_device_list()
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <cdio/cdio.h>
#include <cdio/cd_types.h>
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

static bool 
is_in(char **file_list, const char *file) 
{
  char **p;
  for (p = file_list; p != NULL && *p != NULL; *p++) {
    if (strcmp(*p, file) == 0) {
      printf("File %s found as expected\n", file);
      return true;
    }
  }
  printf("Can't find file %s in list\n", file);
  return false;
}

int
main(int argc, const char *argv[])
{
  char **nrg_images=NULL;
  char **bincue_images=NULL;
  char **imgs;
  char **c;
  unsigned int i;

  const char *cue_files[2] = {"cdda.cue", "isofs-m1.cue"};
  const char *nrg_files[1] = {"videocd.nrg"};
  
  cdio_log_set_handler (log_handler);

  if (! (cdio_have_driver(DRIVER_NRG) && cdio_have_driver(DRIVER_BINCUE)) )  {
    printf("You don't have enough drivers for this test\n");
    exit(77);
  }

  nrg_images = cdio_get_devices(DRIVER_NRG);

  for (imgs=nrg_images; *imgs != NULL; imgs++) {
    printf("NRG image %s\n", *imgs);
  }

  if (!is_in(nrg_images, nrg_files[0])) {
    return 1;
  }
      
  bincue_images = cdio_get_devices(DRIVER_BINCUE);
  
  for (imgs=bincue_images; *imgs != NULL; imgs++) {
    printf("bincue image %s\n", *imgs);
  }
  
  for (i=0; i<2; i++) {
    if (!is_in(bincue_images, cue_files[i])) {
      return 2;
    }
  }
    
  printf("-----\n");
  printf("ISO 9660 images...\n");
  imgs = NULL;
  /* Print out a list of CDDA-drives. */
  imgs = cdio_get_devices_with_cap(bincue_images, CDIO_FS_ISO_9660, false);

  if (NULL == imgs || *imgs == NULL) {
    printf("Failed to find an ISO 9660 image\n");
    return 3;
  }
    
  for( c = imgs; *c != NULL; c++ ) {
    printf("%s\n", *c);
  }
    
  cdio_free_device_list(imgs);
  
  
  printf("-----\n");
  printf("CD-DA images...\n");
  imgs = NULL;
  /* Print out a list of CDDA-drives. */
  imgs = cdio_get_devices_with_cap(bincue_images, CDIO_FS_AUDIO, false);

  if (NULL == imgs || *imgs == NULL) {
    printf("Failed to find CDDA image\n");
    return 4;
  }
    
  for( c = imgs; *c != NULL; *c++ ) {
    printf("%s\n", *c);
  }
    
  cdio_free_device_list(imgs);
  
  
  printf("-----\n");
  imgs = NULL;
  printf("VCD images...\n");
  /* Print out a list of CD-drives with VCD's in them. */
  imgs = cdio_get_devices_with_cap(nrg_images, 
(CDIO_FS_ANAL_SVCD|CDIO_FS_ANAL_CVD|CDIO_FS_ANAL_VIDEOCD|CDIO_FS_UNKNOWN),
					true);
  if (NULL == imgs || *imgs == NULL) {
    printf("Failed to find VCD image\n");
    return 5;
  }
    
  for( c = imgs; *c != NULL; *c++ ) {
    printf("image: %s\n", *c);
  }

  cdio_free_device_list(imgs);

  imgs = NULL;
  /* Print out a list of CDDA-drives. */
  imgs = cdio_get_devices_with_cap(bincue_images, CDIO_FS_HIGH_SIERRA, false);

  if (NULL != imgs && *imgs != NULL) {
    printf("Found erroneous High Sierra image\n");
    return 5;
  }
    
  imgs = NULL;
  /* Print out a list of CDDA-drives. */
  imgs = cdio_get_devices_with_cap(bincue_images, CDIO_FS_UFS, true);

  if (NULL != imgs && *imgs != NULL) {
    printf("Found erroneous UFS image\n");
    return 6;
  }
    
  cdio_free_device_list(imgs);

  return 0;
  
}
