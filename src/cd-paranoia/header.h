/*
  $Id: header.h,v 1.2 2008/04/11 15:44:00 karl Exp $

  Copyright (C) 2008 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 1998 Monty <xiphmont@mit.edu>

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

/** \file header.h 
 *  \brief header for WAV, AIFF and AIFC header-writing routines.
 */

/** Writes WAV headers */
extern void WriteWav(int f,long int i_bytes);

/** Writes AIFC headers */
extern void WriteAifc(int f,long int i_bytes);

/** Writes AIFF headers */
extern void WriteAiff(int f,long int_bytes);
