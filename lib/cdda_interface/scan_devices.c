/*
  $Id: scan_devices.c,v 1.12 2005/01/15 16:05:44 rocky Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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

/******************************************************************
 * 
 * Autoscan for or verify presence of a CD-ROM device
 * 
 ******************************************************************/

#include "common_interface.h"
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "low_interface.h"
#include "utils.h"
#include "cdio/scsi_mmc.h"

#define MAX_DEV_LEN 20 /* Safe because strings only come from below */
/* must be absolute paths! */
const char *scsi_cdrom_prefixes[]={
  "/dev/scd",
  "/dev/sr",
  NULL};
const char *scsi_generic_prefixes[]={
  "/dev/sg",
  NULL};

const char *devfs_scsi_test="/dev/scsi/";
const char *devfs_scsi_cd="cd";
const char *devfs_scsi_generic="generic";

const char *cdrom_devices[]={
  "/dev/cdrom",
  "/dev/cdroms/cdrom?",
  "/dev/hd?",
  "/dev/sg?",
  "/dev/cdu31a",
  "/dev/cdu535",
  "/dev/sbpcd",
  "/dev/sbpcd?",
  "/dev/sonycd",
  "/dev/mcd",
  "/dev/sjcd",
  /* "/dev/aztcd", timeout is too long */
  "/dev/cm206cd",
  "/dev/gscd",
  "/dev/optcd",NULL};

/* Functions here look for a cdrom drive; full init of a drive type
   happens in interface.c */

cdrom_drive_t *
cdda_find_a_cdrom(int messagedest, char **messages){
  /* Brute force... */
  
  int i=0;
  cdrom_drive_t *d;

  while(cdrom_devices[i]!=NULL){

    /* is it a name or a pattern? */
    char *pos;
    if((pos=strchr(cdrom_devices[i],'?'))){
      int j;
      /* try first eight of each device */
      for(j=0;j<4;j++){
	char *buffer=strdup(cdrom_devices[i]);

	/* number, then letter */
	
	buffer[pos-(cdrom_devices[i])]=j+48;
	if((d=cdda_identify(buffer,messagedest,messages)))
	  return(d);
	idmessage(messagedest,messages,"",NULL);
	buffer[pos-(cdrom_devices[i])]=j+97;
	if((d=cdda_identify(buffer,messagedest,messages)))
	  return(d);
	idmessage(messagedest,messages,"",NULL);
      }
    }else{
      /* Name.  Go for it. */
      if((d=cdda_identify(cdrom_devices[i],messagedest,messages)))
	return(d);
      
      idmessage(messagedest,messages,"",NULL);
    }
    i++;
  }
  {
    struct passwd *temp;
    temp=getpwuid(geteuid());
    idmessage(messagedest,messages,
	      "\n\nNo cdrom drives accessible to %s found.\n",
	      temp->pw_name);
  }
  return(NULL);
}

cdrom_drive_t *
cdda_identify(const char *device, int messagedest,char **messages)
{
  cdrom_drive_t *d=NULL;
  idmessage(messagedest,messages,"Checking %s for cdrom...",device);

  d=cdda_identify_cooked(device,messagedest,messages);

  return(d);
}

static char *
test_resolve_symlink(const char *file,int messagedest,char **messages)
{
  char resolved[PATH_MAX];
  struct stat st;
  if(lstat(file,&st)){
    idperror(messagedest,messages,"\t\tCould not stat %s",file);
    return(NULL);
  }

  if(realpath(file,resolved))
    return(strdup(resolved));

  idperror(messagedest,messages,"\t\tCould not resolve symlink %s",file);
  return(NULL);
}

cdrom_drive_t *
cdda_identify_cooked(const char *dev, int messagedest, char **messages)
{
  cdrom_drive_t *d=NULL;
  int drive_type = 0;
  char *description=NULL;
  char *device = NULL;
  CdIo_t *p_cdio = NULL;
#ifdef HAVE_LINUX_MAJOR_H
  struct stat st;
#endif

  if (dev) {
    device = test_resolve_symlink(dev,messagedest,messages);
    if ( !device ) device = strdup(dev);
  }

  p_cdio = cdio_open(device, DRIVER_UNKNOWN);
  
  if (!p_cdio) {
    idperror(messagedest,messages,"\t\tUnable to open %s", dev);
    if (device) free(device);
    return NULL;
  }
  
#ifdef HAVE_LINUX_MAJOR_H
  if ( 0 == stat(device, &st) ) {
    if (S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode)) {
      drive_type=(int)(st.st_rdev>>8);
      switch (drive_type) {
      case IDE0_MAJOR:
      case IDE1_MAJOR:
      case IDE2_MAJOR:
      case IDE3_MAJOR:
	/* Yay, ATAPI... */
	description=strdup("ATAPI compatible ");
	break;
      case CDU31A_CDROM_MAJOR:
	/* major indicates this is a cdrom; no ping necessary. */
	description=strdup("Sony CDU31A or compatible");
	break;
      case CDU535_CDROM_MAJOR:
	/* major indicates this is a cdrom; no ping necessary. */
	description=strdup("Sony CDU535 or compatible");
	break;
	
      case MATSUSHITA_CDROM_MAJOR:
      case MATSUSHITA_CDROM2_MAJOR:
      case MATSUSHITA_CDROM3_MAJOR:
      case MATSUSHITA_CDROM4_MAJOR:
	/* major indicates this is a cdrom; no ping necessary. */
	description=strdup("non-ATAPI IDE-style Matsushita/Panasonic CR-5xx or compatible");
	break;
      case SANYO_CDROM_MAJOR:
	description=strdup("Sanyo proprietary or compatible: NOT CDDA CAPABLE");
	break;
      case MITSUMI_CDROM_MAJOR:
      case MITSUMI_X_CDROM_MAJOR:
	description=strdup("Mitsumi proprietary or compatible: NOT CDDA CAPABLE");
	break;
      case OPTICS_CDROM_MAJOR:
	description=strdup("Optics Dolphin or compatible: NOT CDDA CAPABLE");
	break;
      case AZTECH_CDROM_MAJOR:
	description=strdup("Aztech proprietary or compatible: NOT CDDA CAPABLE");
	break;
      case GOLDSTAR_CDROM_MAJOR:
	description=strdup("Goldstar proprietary: NOT CDDA CAPABLE");
	break;
      case CM206_CDROM_MAJOR:
	description=strdup("Philips/LMS CM206 proprietary: NOT CDDA CAPABLE");
	break;
	
      case SCSI_CDROM_MAJOR:   
      case SCSI_GENERIC_MAJOR: 
	/* Nope nope nope */
	idmessage(messagedest,messages,"\t\t%s is not a cooked ioctl CDROM.",
		  device);
	free(device);
	return(NULL);
      default:
	/* What the hell is this? */
	idmessage(messagedest,messages,"\t\t%s is not a cooked ioctl CDROM.",
		  device);
	free(device);
	return(NULL);
      }
    }
  }
#endif /*HAVE_LINUX_MAJOR_H*/

  /* Minimum init */
  
  d=calloc(1,sizeof(cdrom_drive_t));
  d->p_cdio           = p_cdio;
  d->cdda_device_name = device;
  d->drive_type       = drive_type;
  d->interface        = COOKED_IOCTL;
  d->bigendianp       = -1; /* We don't know yet... */
  d->nsectors         = -1;
  
  {
    cdio_hwinfo_t hw_info;

    if ( scsi_mmc_get_hwinfo( p_cdio, &hw_info ) ) {
      unsigned int i_len = strlen(hw_info.psz_vendor) 
	+ strlen(hw_info.psz_model) 
	+ strlen(hw_info.psz_revision)
	+ strlen(description) + 3;

      d->drive_model=malloc( i_len );
      snprintf( d->drive_model, i_len, "%s%s%s %s", 
		hw_info.psz_vendor, hw_info.psz_model, hw_info.psz_revision,
		description );
      idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n", 
		d->drive_model);
      if (description) free(description);
    }
  }
  
  return(d);
}
