/*
    $Id: analyze.c,v 1.1 2003/08/14 13:41:26 rocky Exp $

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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "config.h"

#include <cdio/cdio.h>
#include <cdio/logging.h>
#include <cdio/util.h>

#include <fcntl.h>
#ifdef __linux__
# include <linux/version.h>
# include <linux/cdrom.h>
# if LINUX_VERSION_CODE < KERNEL_VERSION(2,1,50)
#  include <linux/ucdrom.h>
# endif
#endif
 
#include <errno.h>
#include "analyze.h"

#ifdef ENABLE_NLS
#include <locale.h>
#    include <libintl.h>
#    define _(String) dgettext ("cdinfo", String)
#else
/* Stubs that do something close enough.  */
#    define _(String) (String)
#endif

#define dbg_print(level, s, args...) 

/*
Subject:   -65- How can I read an IRIX (EFS) CD-ROM on a machine which
                doesn't use EFS?
Date: 18 Jun 1995 00:00:01 EST

  You want 'efslook', at
  ftp://viz.tamu.edu/pub/sgi/software/efslook.tar.gz.

and
! Robert E. Seastrom <rs@access4.digex.net>'s software (with source
! code) for using an SGI CD-ROM on a Macintosh is at
! ftp://bifrost.seastrom.com/pub/mac/CDROM-Jumpstart.sit151.hqx.

*/

static char buffer[6][CDIO_CD_FRAMESIZE_RAW];  /* for CD-Data */

/* Some interesting sector numbers stored in the above buffer. */
#define ISO_SUPERBLOCK_SECTOR  16  /* buffer[0] */
#define UFS_SUPERBLOCK_SECTOR   4  /* buffer[2] */
#define BOOT_SECTOR            17  /* buffer[3] */
#define VCD_INFO_SECTOR       150  /* buffer[4] */


typedef struct signature
{
  unsigned int buf_num;
  unsigned int offset;
  const char *sig_str;
  const char *description;
} signature_t;

static signature_t sigs[] =
  {
/*buffer[x] off look for     description */
    {0,     1, "CD001",      "ISO 9660"}, 
    {0,     1, "CD-I",       "CD-I"}, 
    {0,     8, "CDTV",       "CDTV"}, 
    {0,     8, "CD-RTOS",    "CD-RTOS"}, 
    {0,     9, "CDROM",      "HIGH SIERRA"}, 
    {0,    16, "CD-BRIDGE",  "BRIDGE"}, 
    {0,  1024, "CD-XA001",   "XA"}, 
    {1,    64, "PPPPHHHHOOOOTTTTOOOO____CCCCDDDD",  "PHOTO CD"}, 
    {1, 0x438, "\x53\xef",   "EXT2 FS"}, 
    {2,  1372, "\x54\x19\x01\x0", "UFS"}, 
    {3,     7, "EL TORITO",  "BOOTABLE"}, 
    {4,     0, "VIDEO_CD",   "VIDEO CD"}, 
    {4,     0, "SUPERVCD",   "SVCD or Chaoji VCD"}, 
    { 0 }
  };


#define IS_ISOFS     0
#define IS_CD_I      1
#define IS_CDTV      2
#define IS_CD_RTOS   3
#define IS_HS        4
#define IS_BRIDGE    5
#define IS_XA        6
#define IS_PHOTO_CD  7
#define IS_EXT2      8
#define IS_UFS       9
#define IS_BOOTABLE 10
#define IS_VIDEO_CD 11 /* Video CD */
#define IS_SVCD      12 /* Chinese Video CD - slightly incompatible with SVCD */

cdio_analysis_t cdio_analysis;

/* ------------------------------------------------------------------------ */
/* some ISO 9660 fiddling                                                   */

static int 
read_block(CdIo *cdio, int superblock, uint32_t offset, uint8_t bufnum, 
	   track_t track_num)
{
  unsigned int track_sec_count = cdio_get_track_sec_count(cdio, track_num);
  memset(buffer[bufnum], 0, CDIO_CD_FRAMESIZE);

  if ( track_sec_count < superblock) {
    dbg_print(1, "reading block %u skipped track %d has only %u sectors\n", 
	      superblock, track_num, track_sec_count);
    return -1;
  }
  
  dbg_print(2, "about to read sector %u\n", offset+superblock);

  if (cdio_get_track_green(cdio,  track_num)) {
    if (0 > cdio_read_mode2_sector(cdio, buffer[bufnum], 
				   offset+superblock, false))
      return -1;
  } else {
    if (0 > cdio_read_yellow_sector(cdio, buffer[bufnum], 
				    offset+superblock, false))
      return -1;
  }

  return 0;
}

static bool 
is_it(int num) 
{
  signature_t *sigp=&sigs[num];
  int len=strlen(sigp->sig_str);

  /* TODO: check that num < largest sig. */
  return 0 == memcmp(&buffer[sigp->buf_num][sigp->offset], sigp->sig_str, len);
}

static int 
is_hfs(void)
{
  return (0 == memcmp(&buffer[1][512],"PM",2)) ||
    (0 == memcmp(&buffer[1][512],"TS",2)) ||
    (0 == memcmp(&buffer[1][1024], "BD",2));
}

static int 
is_3do(void)
{
  return (0 == memcmp(&buffer[1][0],"\x01\x5a\x5a\x5a\x5a\x5a\x01", 7)) &&
    (0 == memcmp(&buffer[1][40], "CD-ROM", 6));
}

static int is_joliet(void)
{
  return 2 == buffer[3][0] && buffer[3][88] == 0x25 && buffer[3][89] == 0x2f;
}

/* ISO 9660 volume space in M2F1_SECTOR_SIZE byte units */
static int 
get_size(void)
{
  return ((buffer[0][80] & 0xff) |
	  ((buffer[0][81] & 0xff) << 8) |
	  ((buffer[0][82] & 0xff) << 16) |
	  ((buffer[0][83] & 0xff) << 24));
}

static int 
get_joliet_level( void )
{
  switch (buffer[3][90]) {
  case 0x40: return 1;
  case 0x43: return 2;
  case 0x45: return 3;
  }
  return 0;
}

#define is_it_dbg(sig) \
    if (is_it(sig)) printf("%s, ", sigs[sig].description)

int 
guess_filesystem(CdIo *cdio, int start_session, track_t track_num)
{
  int ret = 0;
  
  if (read_block(cdio, ISO_SUPERBLOCK_SECTOR, start_session, 0, track_num) < 0)
    return FS_UNKNOWN;
  
  /* filesystem */
  if (is_it(IS_CD_I) && is_it(IS_CD_RTOS) 
      && !is_it(IS_BRIDGE) && !is_it(IS_XA)) {
    return FS_INTERACTIVE;
  } else {	/* read sector 0 ONLY, when NO greenbook CD-I !!!! */

    if (read_block(cdio, 0, start_session, 1, track_num) < 0)
      return ret;
    
    if (is_it(IS_HS))
      ret |= FS_HIGH_SIERRA;
    else if (is_it(IS_ISOFS)) {
      if (is_it(IS_CD_RTOS) && is_it(IS_BRIDGE))
	ret = FS_ISO_9660_INTERACTIVE;
      else if (is_hfs())
	ret = FS_ISO_HFS;
      else
	ret = FS_ISO_9660;
      cdio_analysis.isofs_size = get_size();
      sprintf(cdio_analysis.iso_label, buffer[0]+40);
      
#if 0
      if (is_rockridge())
	ret |= ROCKRIDGE;
#endif

      if (read_block(cdio, BOOT_SECTOR, start_session, 3, track_num) < 0)
	return ret;
      
      if (is_joliet()) {
	cdio_analysis.joliet_level = get_joliet_level();
	ret |= JOLIET;
      }
      if (is_it(IS_BOOTABLE))
	ret |= BOOTABLE;
      
      if (is_it(IS_XA) && is_it(IS_ISOFS) 
	  && !is_it(IS_PHOTO_CD)) {

        if (read_block(cdio, VCD_INFO_SECTOR, start_session, 4, track_num) < 0)
	  return ret;
	
	if (is_it(IS_BRIDGE) && is_it(IS_CD_RTOS)) {
	  if (is_it(IS_VIDEO_CD))  ret |= VIDEOCDI;
	  else if (is_it(IS_SVCD)) ret |= SVCD;
	}
      }
    } 
    else if (is_hfs())       ret |= FS_HFS;
    else if (is_it(IS_EXT2)) ret |= FS_EXT2;
    else if (is_3do())       ret |= FS_3DO;
    else {
      if (read_block(cdio, UFS_SUPERBLOCK_SECTOR, start_session, 2, track_num) < 0)
	return ret;
      
      if (is_it(IS_UFS)) 
	ret |= FS_UFS;
      else
	ret |= FS_UNKNOWN;
    }
  }
  
  /* other checks */
  if (is_it(IS_XA))       ret |= XA;
  if (is_it(IS_PHOTO_CD)) ret |= PHOTO_CD;
  if (is_it(IS_CDTV))     ret |= CDTV;
  return ret;
}
