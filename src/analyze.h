/*
    $Id: analyze.h,v 1.1 2003/08/14 13:41:26 rocky Exp $

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
#define SVCD       	     8192   /* Choiji Video CD */


#define FS_NO_DATA              0   /* audio only */
#define FS_HIGH_SIERRA		1
#define FS_ISO_9660		2
#define FS_INTERACTIVE		3
#define FS_HFS			4
#define FS_UFS			5
#define FS_EXT2			6
#define FS_ISO_HFS              7  /* both hfs & isofs filesystem */
#define FS_ISO_9660_INTERACTIVE 8  /* both CD-RTOS and isofs filesystem */
#define FS_3DO			9
#define FS_UNKNOWN	       15
#define FS_MASK		       15

typedef struct 
{
  int joliet_level;
  char iso_label[32];
  int  isofs_size;
} cdio_analysis_t;

extern cdio_analysis_t cdio_analysis;


int guess_filesystem(CdIo *cdio, int start_session, track_t track_num);
