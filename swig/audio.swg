/* -*- c -*-
  $Id: audio.swg,v 1.3 2008/05/01 16:55:05 karl Exp $

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

/** NOTE: THIS IS THE EASILY CONVERTED SUBSET OF LIBCDIO WE CAN DO. **/

/* See <cdio/audio.h> for more extensive documentation. */

%rename cdio_audio_pause audio_pause;
%feature("autodoc", 
"audio_pause(cdio)->status

Pause playing CD through analog output.");
driver_return_code_t cdio_audio_pause (CdIo_t *p_cdio);


%feature("autodoc", 
"auto_play_lsn(cdio, start_lsn, end_lsn)->status

Playing CD through analog output at the given lsn to the ending lsn");
driver_return_code_t audio_play_lsn (CdIo_t *p_cdio, lsn_t start_lsn,
				     lsn_t end_lsn);
%inline %{
driver_return_code_t audio_play_lsn (CdIo_t *p_cdio, lsn_t start_lsn,
				     lsn_t end_lsn)
{
  msf_t start_msf;
  msf_t end_msf;
  cdio_lsn_to_msf (start_lsn, &start_msf);
  cdio_lsn_to_msf (end_lsn, &end_msf);
  return cdio_audio_play_msf(p_cdio, &start_msf, &end_msf);
}
%}

%rename cdio_audio_resume audio_resume;
%feature("autodoc", 
"audio_resume(cdio)->status
Resume playing an audio CD.");
driver_return_code_t cdio_audio_resume (CdIo_t *p_cdio);


%rename cdio_audio_stop audio_stop;
%feature("autodoc", 
"audio_stop(cdio)->status
Stop playing an audio CD.");
driver_return_code_t cdio_audio_stop (CdIo_t *p_cdio);

