/*
    $Id: analyze.h,v 1.2 2003/08/16 12:59:03 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
    Copyright (C) 1996,1997,1998  Gerd Knorr <kraxel@bytesex.org>
         and       Heiko Eiﬂfeldt <heiko@hexco.de>

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

#define XA		       16
#define MULTISESSION	       32
#define PHOTO_CD	       64
#define HIDDEN_TRACK          128
#define CDTV		      256
#define BOOTABLE       	      512
#define VIDEOCDI       	     1024
#define ROCKRIDGE            2048
#define JOLIET               4096
#define SVCD       	     8192   /* Super VCD or Choiji Video CD */
#define CVD       	    16384   /* Choiji Video CD */


#define CDIO_FS_NO_DATA              0   /* audio only */
#define CDIO_FS_HIGH_SIERRA	     1
#define CDIO_FS_ISO_9660	     2
#define CDIO_FS_INTERACTIVE	     3
#define CDIO_FS_HFS		     4
#define CDIO_FS_UFS		     5
#define CDIO_FS_EXT2		     6
#define CDIO_FS_ISO_HFS              7  /* both hfs & isofs filesystem */
#define CDIO_FS_ISO_9660_INTERACTIVE 8  /* both CD-RTOS and isofs filesystem */
#define CDIO_FS_3DO		     9
#define CDIO_FS_UNKNOWN	            15
#define CDIO_FS_MASK		    15

typedef struct 
{
  int joliet_level;
  char iso_label[33]; /* 32 + 1 for null byte at the end in formatting */
  int  isofs_size;
} cdio_analysis_t;

/* 
   Try to determine what kind of filesystem is at track track_num 
*/
int cdio_guess_filesystem(/*in*/ CdIo *cdio, int start_session, 
			  track_t track_num, 
			  /*out*/ cdio_analysis_t *cdio_analysis);
