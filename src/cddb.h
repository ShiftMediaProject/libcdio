/*
  $Id: cddb.h,v 1.1 2005/03/06 11:21:52 rocky Exp $

  Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
  
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

/*!
   Compute the CDDB disk ID for an Audio disk.  This is a funny checksum
   consisting of the concatenation of 3 things:
      the sum of the decimal digits of sizes of all tracks, 
      the total length of the disk, and 
      the number of tracks.
*/
unsigned long cddb_discid(CdIo_t *p_cdio, track_t i_tracks);
