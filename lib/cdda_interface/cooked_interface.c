/*
  $Id: cooked_interface.c,v 1.4 2005/01/06 01:15:51 rocky Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Original interface.c Copyright (C) 1994-1997 
             Eissfeldt heiko@colossus.escape.de
  Current blenderization Copyright (C) 1998-1999 Monty xiphmont@mit.edu
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
 * CDROM code specific to the cooked ioctl interface
 *
 ******************************************************************/

#include "low_interface.h"
#include "common_interface.h"
#include "utils.h"

static int 
cooked_readtoc (cdrom_drive_t *d)
{
  int i;
  track_t i_track;

  /* Save TOC Entries */
  d->tracks = cdio_get_num_tracks(d->p_cdio) ;
  i_track   = cdio_get_first_track_num(d->p_cdio);
  
  for ( i=0; i < d->tracks; i++) {
    d->disc_toc[i].bTrack = i_track;
    d->disc_toc[i].dwStartSector = cdio_get_track_lsn(d->p_cdio, i_track);
    i_track++;
  }

  d->disc_toc[i].bTrack = i_track;
  d->disc_toc[i].dwStartSector = cdio_get_track_lsn(d->p_cdio, 
						    CDIO_CDROM_LEADOUT_TRACK);

  d->cd_extra=FixupTOC(d, i_track);
  return --i_track;  /* without lead-out */
}


/* Set operating speed */
static int 
cooked_setspeed(cdrom_drive_t *d, int speed)
{
#if SET_SPEED_FIXED
  if(d->ioctl_fd!=-1)
    return ioctl(d->ioctl_fd, CDROM_SELECT_SPEED, speed);
  else
#endif
    return 0;
}


/* read 'SectorBurst' adjacent sectors of audio sectors 
 * to Buffer '*p' beginning at sector 'lSector'
 */

static long int
cooked_read (cdrom_drive_t *d, void *p, lsn_t begin, long sectors)
{
  int retry_count,err;
  struct cdrom_read_audio arg;
  char *buffer=(char *)p;

  /* read d->nsectors at a time, max. */
  sectors=( sectors > d->nsectors ? d->nsectors : sectors);

  arg.addr.lba = begin;
  arg.addr_format = CDROM_LBA;
  arg.nframes = sectors;
  arg.buf=buffer;
  retry_count=0;

  do {
    err = cdio_read_audio_sectors( d->p_cdio, buffer, begin, sectors);
    
    if ( 0 != err ) {
      if (!d->error_retry) return -7;
      
      if (sectors==1) {
	/* *Could* be I/O or media error.  I think.  If we're at
	   30 retries, we better skip this unhappy little
	   sector. */
	if (retry_count>MAX_RETRIES-1) {
	  char b[256];
	  snprintf(b, sizeof(b), 
		   "010: Unable to access sector %ld: skipping...\n",
		   (long int) begin);
	  cderror(d, b);
	  return -10;
	}
	break;
      }

      if(retry_count>4)
	if(sectors>1)
	  sectors=sectors*3/4;
      retry_count++;
      if (retry_count>MAX_RETRIES) {
	cderror(d,"007: Unknown, unrecoverable error reading data\n");
	return(-7);
      }
    } else
      break;
  } while (err);
  
  return(sectors);
}

/* hook */
static int Dummy (cdrom_drive_t *d,int Switch)
{
  return(0);
}

static int 
verify_read_command(cdrom_drive_t *d)
{
  int i;
  int16_t *buff=malloc(CD_FRAMESIZE_RAW);
  int audioflag=0;

  cdmessage(d,"Verifying drive can read CDDA...\n");

  d->enable_cdda(d,1);

  for(i=1;i<=d->tracks;i++){
    if(cdda_track_audiop(d,i)==1){
      long firstsector=cdda_track_firstsector(d,i);
      long lastsector=cdda_track_lastsector(d,i);
      long sector=(firstsector+lastsector)>>1;
      audioflag=1;

      if(d->read_audio(d,buff,sector,1)>0){
	cdmessage(d,"\tExpected command set reads OK.\n");
	d->enable_cdda(d,0);
	free(buff);
	return(0);
      }
    }
  }
 
  d->enable_cdda(d,0);

  if(!audioflag){
    cdmessage(d,"\tCould not find any audio tracks on this disk.\n");
    return(-403);
  }

  cdmessage(d,"\n\tUnable to read any data; "
	    "drive probably not CDDA capable.\n");
  
  cderror(d,"006: Could not read any data from drive\n");

  free(buff);
  return(-6);
}

#include "drive_exceptions.h"

#if CHECK_EXCEPTIONS_FIXED
static void 
check_exceptions(cdrom_drive_t *d, const exception_t *list)
{

  int i=0;
  while(list[i].model){
    if(!strncmp(list[i].model,d->drive_model,strlen(list[i].model))){
      if(list[i].bigendianp!=-1)d->bigendianp=list[i].bigendianp;
      return;
    }
    i++;
  }
}
#endif

/* set function pointers to use the ioctl routines */
int 
cooked_init_drive (cdrom_drive_t *d){
  int ret;

#if BUFSIZE_DETERMINATION_FIXED
  switch(d->drive_type){
  case MATSUSHITA_CDROM_MAJOR:	/* sbpcd 1 */
  case MATSUSHITA_CDROM2_MAJOR:	/* sbpcd 2 */
  case MATSUSHITA_CDROM3_MAJOR:	/* sbpcd 3 */
  case MATSUSHITA_CDROM4_MAJOR:	/* sbpcd 4 */
    /* don't make the buffer too big; this sucker don't preempt */

    cdmessage(d,"Attempting to set sbpcd buffer size...\n");

    d->nsectors=8;
    while(1){

      /* this ioctl returns zero on error; exactly wrong, but that's
         what it does. */

      if (ioctl(d->ioctl_fd, CDROMAUDIOBUFSIZ, d->nsectors)==0) {
	d->nsectors>>=1;
	if(d->nsectors==0){
	  char buffer[256];
	  d->nsectors=8;
	  sprintf(buffer,"\tTrouble setting buffer size.  Defaulting to %d sectors.\n",
		  d->nsectors);
	  cdmessage(d,buffer);
	  break; /* Oh, well.  Try to read anyway.*/
	}
      } else {
	char buffer[256];
	sprintf(buffer,"\tSetting read block size at %d sectors (%ld bytes).\n",
		d->nsectors,(long)d->nsectors*CD_FRAMESIZE_RAW);
	cdmessage(d,buffer);
	break;
      }
    }

    break;
  case IDE0_MAJOR:
  case IDE1_MAJOR:
  case IDE2_MAJOR:
  case IDE3_MAJOR:
    d->nsectors=8; /* it's a define in the linux kernel; we have no
		      way of determining other than this guess tho */
    d->bigendianp=0;
    d->is_atapi=1;

    check_exceptions(d, atapi_list);

    break;
  default:
    d->nsectors=40; 
  }
#else 
  {
    char buffer[256];
    d->nsectors = 8; 
    sprintf(buffer,"\tSetting read block size at %d sectors (%ld bytes).\n",
	    d->nsectors,(long)d->nsectors*CD_FRAMESIZE_RAW);
    cdmessage(d,buffer);
  }
  
#endif

  d->enable_cdda = Dummy;
  d->read_audio = cooked_read;
  d->set_speed = cooked_setspeed;
  d->read_toc = cooked_readtoc;
  ret=d->tracks=d->read_toc(d);
  if(d->tracks<1)
    return(ret);

  d->opened=1;
  if((ret=verify_read_command(d)))return(ret);
  d->error_retry=1;
  return(0);
}

