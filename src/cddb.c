/*
  $Id: cddb.c,v 1.2 2005/03/06 16:00:35 rocky Exp $

  Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
  
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <cdio/audio.h>
#include "cddb.h"

/*!
   Returns the sum of the decimal digits in a number. Eg. 1955 = 20
*/
static int
cddb_dec_digit_sum(int n)
{
  int ret=0;
  
  for (;;) {
    ret += n%10;
    n    = n/10;
    if (!n) return ret;
  }
}

/*!
   Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
   consisting of the concatenation of 3 things:
      the sum of the decimal digits of sizes of all tracks, 
      the total length of the disk, and 
      the number of tracks.
*/
unsigned long
cddb_discid(CdIo_t *p_cdio, track_t i_tracks)
{
  int i,t,n=0;
  msf_t start_msf;
  msf_t msf;
  
  for (i = 1; i <= i_tracks; i++) {
    cdio_get_track_msf(p_cdio, i, &msf);
    n += cddb_dec_digit_sum(cdio_audio_get_msf_seconds(&msf));
  }

  cdio_get_track_msf(p_cdio, 1, &start_msf);
  cdio_get_track_msf(p_cdio, CDIO_CDROM_LEADOUT_TRACK, &msf);
  
  t = cdio_audio_get_msf_seconds(&msf)-cdio_audio_get_msf_seconds(&start_msf);
  
  return ((n % 0xff) << 24 | t << 8 | i_tracks);
}
