/*
  $Id: util.c,v 1.8 2004/06/19 19:15:15 rocky Exp $

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

/* Miscellaneous things common to standalone programs. */

#include "util.h"

cdio_log_handler_t gl_default_cdio_log_handler = NULL;
char *source_name = NULL;
char *program_name;

void 
myexit(CdIo *cdio, int rc) 
{
  if (NULL != cdio) cdio_destroy(cdio);
  if (NULL != program_name) free(program_name);
  if (NULL != source_name)  free(source_name);
  exit(rc);
}

void
print_version (char *program_name, const char *version, 
	       int no_header, bool version_only)
{
  
  driver_id_t driver_id;

  if (no_header == 0)
    printf( "%s version %s\nCopyright (c) 2003, 2004 R. Bernstein\n",
	    program_name, version);
  printf( _("This is free software; see the source for copying conditions.\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
"));

  if (version_only) {
    char *default_device;
    for (driver_id=DRIVER_UNKNOWN+1; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
      if (cdio_have_driver(driver_id)) {
	printf("Have driver: %s\n", cdio_driver_describe(driver_id));
      }
    }
    default_device=cdio_get_default_device(NULL);
    if (default_device)
      printf("Default CD-ROM device: %s\n", default_device);
    else
      printf("No CD-ROM device found.\n");
    free(program_name);
    exit(100);
  }
  
}

#define DEV_PREFIX "/dev/"
char *
fillout_device_name(const char *device_name) 
{
#if defined(HAVE_WIN32_CDROM)
  return strdup(device_name);
#else
  unsigned int prefix_len = strlen(DEV_PREFIX);
  if (0 == strncmp(device_name, DEV_PREFIX, prefix_len))
    return strdup(device_name);
  else {
    char *full_device_name = (char*) malloc(strlen(device_name)+prefix_len);
    sprintf(full_device_name, DEV_PREFIX "%s", device_name);
    return full_device_name;
  }
#endif
}

/* Prints out drive capabilities */
void
print_drive_capabilities(cdio_drive_cap_t i_drive_cap)
{
  if (CDIO_DRIVE_CAP_ERROR == i_drive_cap) {
    printf("Error in getting drive properties\n");
  } else {
    printf("Hardware             : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_FILE  
	   ? "Disk Image"  : "CD-ROM or DVD");
    printf("Can open tray        : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_OPEN_TRAY  ? "Yes"  : "No");
    printf("Can close tray       : %s\n\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_OPEN_TRAY  ? "Yes"  : "No");
    
    printf("Compact Disc         : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_CD         ? "Yes"  : "No");
    printf("   Can play audio    : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_CD_AUDIO   ?  "Yes" : "No");
    printf("   Can read  CD-RW   : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_CD_RW      ?  "Yes" : "No");
    printf("   Can write CD-R    : %s\n\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_CD_R       ?  "Yes" : "No");
    
    printf("Digital Versital Disc: %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_DVD        ?  "Yes" : "No");
    printf("   Can write DVD-R   : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_DVD_R      ?  "Yes" : "No");
    printf("   Can write DVD-RAM : %s\n", 
	   i_drive_cap & CDIO_DRIVE_CAP_DVD_RAM    ?  "Yes" : "No");
  }
  if (CDIO_DRIVE_CAP_UNKNOWN == i_drive_cap) {
    printf("Not completely sure about drive properties\n\n");
  }
}
