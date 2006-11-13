/* -*- c -*-
    $Id: disc.swg,v 1.1 2006/11/13 05:12:42 rocky Exp $

    Copyright (C) 2006 Rocky Bernstein <rocky@cpan.org>

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
/* See <cdio/device.h> for more extensive documentation. */

%constant long int DISC_MODE_CD_DA    = CDIO_DISC_MODE_CD_DA;
%constant long int DISC_MODE_CD_DATA  = CDIO_DISC_MODE_CD_DATA;
%constant long int DISC_MODE_CD_XA  = CDIO_DISC_MODE_CD_XA;
%constant long int DISC_MODE_CD_MIXED  = CDIO_DISC_MODE_CD_MIXED;
%constant long int DISC_MODE_DVD_ROM  = CDIO_DISC_MODE_DVD_ROM;
%constant long int DISC_MODE_DVD_RAM  = CDIO_DISC_MODE_DVD_RAM;
%constant long int DISC_MODE_DVD_R  = CDIO_DISC_MODE_DVD_R;
%constant long int DISC_MODE_DVD_RW  = CDIO_DISC_MODE_DVD_RW;
%constant long int DISC_MODE_DVD_PR  = CDIO_DISC_MODE_DVD_PR;
%constant long int DISC_MODE_DVD_PRW  = CDIO_DISC_MODE_DVD_PRW;
%constant long int DISC_MODE_DVD_OTHER  = CDIO_DISC_MODE_DVD_OTHER;
%constant long int DISC_MODE_NO_INFO  = CDIO_DISC_MODE_NO_INFO;
%constant long int DISC_MODE_ERROR  = CDIO_DISC_MODE_ERROR;
%constant long int DISC_MODE_CD_I = CDIO_DISC_MODE_CD_I;

%rename cdio_get_disc_last_lsn get_disc_last_lsn;
%feature("autodoc",
"get_disc_last_lsn(cdio)->lsn
Get the LSN of the end of the CD.

perlcdio.INVALID_LSN is returned on error.");
lsn_t cdio_get_disc_last_lsn(const CdIo_t *p_cdio);
  

%feature("autodoc",
"get_disc_mode(p_cdio) -> str

Get disc mode - the kind of CD (CD-DA, CD-ROM mode 1, CD-MIXED, etc.
that we've got. The notion of 'CD' is extended a little to include
DVD's.");
const char *get_disc_mode (CdIo_t *p_cdio);

%rename cdio_get_joliet_level get_joliet_level;
%feature("autodoc",
"get_joliet_level(cdio)->int

Return the Joliet level recognized for cdio.
This only makes sense for something that has an ISO-9660
filesystem.");
int cdio_get_joliet_level(const CdIo_t *p_cdio);

/*!
  Get the media catalog number (MCN) from the CD.
  
  @return the media catalog number or NULL if there is none or we
  don't have the ability to get it.
  
  Note: string is malloc'd so caller has to free() the returned
  string when done with it.
  
*/
%rename cdio_get_mcn get_mcn;
%newobject cdio_get_mcn; // free malloc'd return value
char * cdio_get_mcn (const CdIo_t *p_cdio);

%rename cdio_get_num_tracks get_num_tracks;
%feature("autodoc",
"get_num_tracks(p_cdio)->int

Return the number of tracks on the CD. 
On error rubycdio::INVALID_TRACK is returned.");
track_t cdio_get_num_tracks (const CdIo_t *p_cdio);

/*========================INLINE======================================*/

%inline %{
const char *
get_disc_mode (CdIo_t *p_cdio) 
{
  discmode_t discmode = cdio_get_discmode(p_cdio);
  if (CDIO_DISC_MODE_ERROR == discmode) return NULL;
  return discmode2str[discmode];
}

%}
