/*
  $Id: buffering_write.h,v 1.4 2008/06/19 15:44:30 flameeyes Exp $
 
  Copyright (C) 2004, 2008 Rocky Bernstein <rocky@gnu.org>
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

/** buffering_write() - buffers data to a specified size before writing.
 *
 * Restrictions:
 * - MUST CALL BUFFERING_CLOSE() WHEN FINISHED!!!
 *
 */
extern long buffering_write(int outf, char *buffer, long num);

/** buffering_close() - writes out remaining buffered data before
 * closing file.
 *
 */
extern int buffering_close(int fd);


