/*
    $Id: cd_types.h,v 1.3 2003/10/02 02:59:57 rocky Exp $

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

/*
   Filesystem types we understand. The highest-numbered fs type should
   be less than CDIO_FS_MASK defined below.
*/

#ifndef __CDIO_CD_TYPES_H__
#define __CDIO_CD_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define CDIO_FS_AUDIO                1   /* audio only - not really a fs */
#define CDIO_FS_HIGH_SIERRA	     2
#define CDIO_FS_ISO_9660	     3
#define CDIO_FS_INTERACTIVE	     4
#define CDIO_FS_HFS		     5
#define CDIO_FS_UFS		     6

/* EXT2 was the GNU/Linux native filesystem for early kernels. Newer
   GNU/Linux OS's may use EXT3 which EXT2 with a journal. */
#define CDIO_FS_EXT2		     7

#define CDIO_FS_ISO_HFS              8  /* both hfs & isofs filesystem */
#define CDIO_FS_ISO_9660_INTERACTIVE 9  /* both CD-RTOS and isofs filesystem */


/* The 3DO is, technically, a set of specifications created by the 3DO
company.  These specs are for making a 3DO Interactive Multiplayer
which uses a CD-player. Panasonic in the early 90's was the first
company to manufacture and market a 3DO player. */
#define CDIO_FS_3DO		    10

#define CDIO_FS_MASK		    15  /* Should be 2*n-1 and > above */
#define CDIO_FS_UNKNOWN	            CDIO_FS_MASK

/* Macro to extract just the FS type portion defined above */
#define CDIO_FSTYPE(fs) (fs & CDIO_FS_MASK)

/* 
   Bit masks for the classes of CD-images. These are generally
   higher-level than the fs-type information above and may be determined
   based of the fs type information.
 */
#define CDIO_FS_ANAL_XA		        16
#define CDIO_FS_ANAL_MULTISESSION	32
#define CDIO_FS_ANAL_PHOTO_CD	        64
#define CDIO_FS_ANAL_HIDDEN_TRACK      128
#define CDIO_FS_ANAL_CDTV	       256
#define CDIO_FS_ANAL_BOOTABLE          512
#define CDIO_FS_ANAL_VIDEOCD          1024   /* VCD 1.1 */
#define CDIO_FS_ANAL_ROCKRIDGE        2048
#define CDIO_FS_ANAL_JOLIET           4096
#define CDIO_FS_ANAL_SVCD             8192   /* Super VCD or Choiji Video CD */
#define CDIO_FS_ANAL_CVD       	     16384   /* Choiji Video CD */

/* Pattern which can be used by cdio_get_devices to specify matching
   any sort of CD.
*/
#define CDIO_FS_MATCH_ALL            (cdio_fs_anal_t) (~CDIO_FS_MASK)


typedef struct 
{
  unsigned int  joliet_level;
  char          iso_label[33]; /* 32 + 1 for null byte at the end in 
				  formatting the string */
  unsigned int  isofs_size;
} cdio_analysis_t;

/* 
   Try to determine what kind of CD-image and/or filesystem we
   have at track track_num. Return information about the CD image
   is returned in cdio_analysis and the return value.
*/
cdio_fs_anal_t cdio_guess_cd_type(const CdIo *cdio, int start_session, 
				  track_t track_num, 
				  /*out*/ cdio_analysis_t *cdio_analysis);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_CD_TYPES_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
