/*
  $Id: scan_devices.c,v 1.4 2005/01/06 03:38:58 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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
 * Autoscan for or verify presence of a cdrom device
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
  struct stat st;
  cdrom_drive_t *d=NULL;
  idmessage(messagedest,messages,"Checking %s for cdrom...",device);

  if(stat(device,&st)){
    idperror(messagedest,messages,"\tCould not stat %s",device);
    return(NULL);
  }
    
#ifndef CDDA_TEST
  if (!S_ISCHR(st.st_mode) &&
      !S_ISBLK(st.st_mode)){
    idmessage(messagedest,messages,"\t%s is not a block or character device",device);
    return(NULL);
  }
#endif

  d=cdda_identify_cooked(device,messagedest,messages);

#ifdef CDDA_TEST
  if(!d)d=cdda_identify_test(device,messagedest,messages);
#endif
  
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
  char *device;
  CdIo_t *p_cdio = NULL;

  device = test_resolve_symlink(dev,messagedest,messages);
  if ( !device ) return NULL;

  p_cdio = cdio_open(device, DRIVER_UNKNOWN);
  
  if (!p_cdio) {
    idperror(messagedest,messages,"\t\tUnable to open %s", dev);
    free(device);
    return NULL;
  }
  
  /* Minimum init */
  
  d=calloc(1,sizeof(cdrom_drive_t));
  d->p_cdio = p_cdio;
  d->cdda_device_name = device;
  d->interface        = COOKED_IOCTL;
  d->bigendianp       = -1; /* We don't know yet... */
  d->nsectors         = -1;
  
  {
    cdio_hwinfo_t hw_info;

    if ( scsi_mmc_get_hwinfo( p_cdio, &hw_info ) ) {
      d->drive_model=calloc(38,1);
      snprintf(d->drive_model, 38, "%s %s %s", 
	       hw_info.psz_vendor, hw_info.psz_model, hw_info.psz_revision );
      idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n", 
		d->drive_model);
    }
  }
  
  return(d);
}

struct  sg_id {
  long    l1; /* target | lun << 8 | channel << 16 | low_ino << 24 */
  long    l2; /* Unique id */
} sg_id;

typedef struct scsiid{
  int bus;
  int id;
  int lun;
} scsiid;

#ifdef CDDA_TEST

cdrom_drive_t *cdda_identify_test(const char *filename, int messagedest,
				  char **messages){
  
  cdrom_drive_t *d=NULL;
  struct stat st;
  int fd=-1;

  idmessage(messagedest,messages,"\tTesting %s for file/test interface",
	    filename);

  if(stat(filename,&st)){
    idperror(messagedest,messages,"\t\tCould not access file %s",
	     filename);
    return(NULL);
  }

  if(!S_ISREG(st.st_mode)){
    idmessage(messagedest,messages,"\t\t%s is not a regular file",
		  filename);
    return(NULL);
  }

  fd=open(filename,O_RDONLY);
  
  if(fd==-1){
    idperror(messagedest,messages,"\t\tCould not open file %s",filename);
    return(NULL);
  }
  
  d=calloc(1,sizeof(cdrom_drive_t));

  d->cdda_device_name=strdup(filename);
  d->ioctl_device_name=strdup(filename);
  d->drive_type=-1;
  d->cdda_fd=fd;
  d->ioctl_fd=fd;
  d->interface=TEST_INTERFACE;
  d->bigendianp=-1; /* We don't know yet... */
  d->nsectors=-1;
  d->drive_model=strdup("File based test interface");
  idmessage(messagedest,messages,"\t\tCDROM sensed: %s\n",d->drive_model);
  
  return(d);
}

#endif
