/* -*- c -*-
  $Id: track.swg,v 1.4 2008/05/01 16:55:05 karl Exp $

  Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* See <cdio/track.h> For more extensive documentation */
%constant long int TRACK_FORMAT_AUDIO      = TRACK_FORMAT_AUDIO;
%constant long int TRACK_FORMAT_CDI        = TRACK_FORMAT_CDI;
%constant long int TRACK_FORMAT_XA         = TRACK_FORMAT_XA;
%constant long int TRACK_FORMAT_DATA       = TRACK_FORMAT_DATA;
%constant long int TRACK_FORMAT_PSX        = TRACK_FORMAT_PSX;

%constant long int CDIO_TRACK_FLAG_FALSE   = CDIO_TRACK_FLAG_FALSE;
%constant long int CDIO_TRACK_FLAG_TRUE    = CDIO_TRACK_FLAG_TRUE;
%constant long int CDIO_TRACK_FLAG_ERROR   = CDIO_TRACK_FLAG_ERROR;
%constant long int CDIO_TRACK_FLAG_UNKNOWN = CDIO_TRACK_FLAG_UNKNOWN;

%constant long int CDIO_CDROM_LBA          = CDIO_CDROM_LBA;
%constant long int CDIO_CDROM_MSF          = CDIO_CDROM_MSF;
%constant long int CDIO_CDROM_DATA_TRACK   = CDIO_CDROM_DATA_TRACK;
%constant long int CDIO_CDROM_CDI_TRACK    = CDIO_CDROM_CDI_TRACK;
%constant long int CDIO_CDROM_XA_TRACK     = CDIO_CDROM_XA_TRACK;

%constant long int AUDIO          = AUDIO;
%constant long int MODE1          = MODE1;
%constant long int MODE1_RAW      = MODE1_RAW;
%constant long int MODE2          = MODE2;
%constant long int MODE2_FORM1    = MODE2_FORM1;
%constant long int MODE2_FORM2    = MODE2_FORM2;
%constant long int MODE2_FORM_MIX = MODE2_FORM_MIX;
%constant long int MODE2_RAW      = MODE2_RAW;

%constant long int INVALID_TRACK  = CDIO_INVALID_TRACK;
%constant long int CDROM_LEADOUT_TRACK = 0xAA;

typedef int track_flag_t;

%rename cdio_get_first_track_num get_first_track_num;
%feature("autodoc",
"get_first_track_num(p_cdio) -> int
Get the number of the first track. 

return the track number or pycdio.INVALID_TRACK if there was
a problem.");
track_t cdio_get_first_track_num(const CdIo_t *p_cdio);

%rename cdio_get_last_track_num    get_last_track_num ;
%feature("autodoc",
"get_last_track_num 
Return the last track number.
pycdio.INVALID_TRACK is if there was a problem.");
track_t cdio_get_last_track_num (const CdIo_t *p_cdio);

%rename cdio_get_track get_track;
%feature("autodoc",
"cdio_get_track(lsn)->int

    Find the track which contains lsn.
    pycdio.INVALID_TRACK is returned if the lsn outside of the CD or
    if there was some error. 
    
    If the lsn is before the pregap of the first track, 0 is returned.
    Otherwise we return the track that spans the lsn.");
track_t cdio_get_track(const CdIo_t *p_cdio, lsn_t lsn);

%rename cdio_get_track_channels get_track_channels;
%feature("autodoc",
"get_track_channels(cdio, track)->int

Return number of channels in track: 2 or 4; -2 if implemented or -1
for error.  Not meaningful if track is not an audio track.");
int cdio_get_track_channels(const CdIo_t *p_cdio, track_t i_track);

%rename cdio_get_track_copy_permit get_track_copy_permit;
%feature("autodoc",
"get_copy_permit(cdio, track)->int

Return copy protection status on a track. Is this meaningful 
not an audio track?
");
track_flag_t cdio_get_track_copy_permit(const CdIo_t *p_cdio, 
					 track_t i_track);

%feature("autodoc",
"get_track_format(cdio, track)->format

Get the format (audio, mode2, mode1) of track. ");
const char *get_track_format(const CdIo_t *p_cdio, track_t i_track);

%rename cdio_get_track_green is_track_green;
%feature("autodoc",
"is_track_green(cdio, track) -> bool

Return True if we have XA data (green, mode2 form1) or
XA data (green, mode2 form2). That is track begins:
 sync - header - subheader
12     4      -  8
    
FIXME: there's gotta be a better design for this and get_track_format?");
bool cdio_get_track_green(const CdIo_t *p_cdio, track_t i_track);

%rename cdio_get_track_last_lsn  get_track_last_lsn;
%feature("autodoc",
"cdio_get_track_last_lsn(cdio, track)->lsn

Return the ending LSN for track number
track in cdio.  CDIO_INVALID_LSN is returned on error.");
lsn_t cdio_get_track_last_lsn(const CdIo_t *p_cdio, track_t i_track);

%rename cdio_get_track_lba get_track_lba;
%feature("autodoc",
"cdio_get_track_lba  
    Get the starting LBA for track number
    i_track in p_cdio.  Track numbers usually start at something 
    greater than 0, usually 1.

    The 'leadout' track is specified either by
    using i_track CDIO_CDROM_LEADOUT_TRACK or the total tracks+1.

    @param p_cdio object to get information from
    @param i_track  the track number we want the LSN for
    @return the starting LBA or CDIO_INVALID_LBA on error.");
  lba_t cdio_get_track_lba(const CdIo_t *p_cdio, track_t i_track);

%rename cdio_get_track_lsn   get_track_lsn;
%feature("autodoc",
"cdio_get_track_lsn (cdio, track)->int

Return the starting LSN for track number. 
Track numbers usually start at something greater than 0, usually 1.

The 'leadout' track is specified either by
using i_track pycdio.CDROM_LEADOUT_TRACK or the total tracks+1.

pycdio.INVALID_LSN is returned on error.");
lsn_t cdio_get_track_lsn(const CdIo_t *p_cdio, track_t i_track);

%feature("autodoc",
"get_track_msf(cdio,track)->string

  Return the starting MSF (minutes/secs/frames) for track number
  track.  Track numbers usually start at something 
  greater than 0, usually 1.

  The 'leadout' track is specified either by
  using i_track CDIO_CDROM_LEADOUT_TRACK or the total tracks+1.
    
@return string mm:ss:ff if all good, or string 'error' on error.");
char *get_track_msf(const CdIo_t *p_cdio, track_t i_track);
%inline %{
char *get_track_msf(const CdIo_t *p_cdio, track_t i_track)
{
	msf_t msf;

	if (!cdio_get_track_msf( p_cdio, i_track, &msf )) {
	  return NULL;
	} else {
	  return cdio_msf_to_str( &msf );
	}
}
%}

%rename cdio_get_track_preemphasis get_track_preemphasis;
%feature("autodoc",
"cdio_get_track_preemphasis(cdio, track)

Get linear preemphasis status on an audio track.
This is not meaningful if not an audio track?");
track_flag_t cdio_get_track_preemphasis(const CdIo_t *p_cdio, track_t i_track);

%rename cdio_get_track_sec_count get_track_sec_count;
%feature("autodoc",
"get_track_sec_count(cdio, track)->int

Get the number of sectors between this track an the next.  This
includes any pregap sectors before the start of the next track.
Track numbers usually start at something 
greater than 0, usually 1.

0 is returned if there is an error.");
unsigned int cdio_get_track_sec_count(const CdIo_t *p_cdio, track_t i_track);

%inline %{
const char 
*get_track_format(const CdIo_t *p_cdio, track_t i_track) 
{
  track_format_t track_format = cdio_get_track_format(p_cdio, i_track);
  return track_format2str[track_format];
}
%}

 
