/*
  $Id: util.c,v 1.11 2004/07/18 03:35:07 rocky Exp $

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
void print_drive_capabilities(cdio_drive_read_cap_t  i_read_cap,
			      cdio_drive_write_cap_t i_write_cap,
			      cdio_drive_misc_cap_t  i_misc_cap)
{
  if (CDIO_DRIVE_CAP_ERROR == i_misc_cap) {
    printf("Error in getting drive hardware properties\n");
  } else {
    printf(_("Hardware                    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_FILE  
	   ? "Disk Image"  : "CD-ROM or DVD");
    printf(_("Can eject                   : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_EJECT         ? "Yes" : "No");
    printf(_("Can close tray              : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_CLOSE_TRAY    ? "Yes" : "No");
    printf(_("Can disable manual eject    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_LOCK          ? "Yes" : "No");
    printf(_("Can select juke-box disc    : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_DISC   ? "Yes" : "No");

    printf(_("Can set drive speed         : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_SPEED  ? "Yes" : "No");
    printf(_("Can detect if CD changed    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED ? "Yes" : "No");
    printf(_("Can read multiple sessions  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MULTI_SESSION ? "Yes" : "No");
    printf(_("Can hard reset device       : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_RESET         ? "Yes" : "No");
  }
  
    
  if (CDIO_DRIVE_CAP_ERROR == i_read_cap) {
      printf("Error in getting drive reading properties\n");
  } else {
    printf("Reading....\n");
    printf(_("  Can play audio            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_AUDIO      ? "Yes" : "No");
    printf(_("  Can read  CD-R            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_R       ? "Yes" : "No");
    printf(_("  Can read  CD-RW           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW      ? "Yes" : "No");
    printf(_("  Can read  DVD-ROM         : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM    ? "Yes" : "No");
  }
  

  if (CDIO_DRIVE_CAP_ERROR == i_write_cap) {
      printf("Error in getting drive writing properties\n");
  } else {
    printf("\nWriting....\n");
    printf(_("  Can write CD-RW           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW     ? "Yes" : "No");
    printf(_("  Can write DVD-R           : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_READ_DVD_R    ? "Yes" : "No");
    printf(_("  Can write DVD-RAM         : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_READ_DVD_RAM  ? "Yes" : "No");
  }
}
