/*
    $Id: _cdio_stream.c,v 1.3 2003/04/06 17:57:20 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "cdio_assert.h"

/* #define STREAM_DEBUG  */

#include "logging.h"
#include "util.h"
#include "_cdio_stream.h"

static const char _rcsid[] = "$Id: _cdio_stream.c,v 1.3 2003/04/06 17:57:20 rocky Exp $";

/* 
 * DataSource implementations
 */

struct _CdioDataSource {
  void* user_data;
  cdio_stream_io_functions op;
  int is_open;
  long position;
};

static bool
_cdio_stream_open_if_necessary(CdioDataSource *obj)
{
  cdio_assert (obj != NULL);

  if (!obj->is_open) {
    if (obj->op.open(obj->user_data)) {
      cdio_error ("could not opening input stream...");
      return false;
    } else {
      cdio_debug ("opened source...");
      obj->is_open = 1;
      obj->position = 0;
    }
  }
  return true;
}

long
cdio_stream_seek(CdioDataSource* obj, long offset, int whence)
{
  cdio_assert (obj != NULL);

  if (!_cdio_stream_open_if_necessary(obj)) return 0;

  if (obj->position != offset) {
#ifdef STREAM_DEBUG
    cdio_warn("had to reposition DataSource from %ld to %ld!", obj->position, offset);
#endif
    obj->position = offset;
    return obj->op.seek(obj->user_data, offset, whence);
  }

  return 0;
}

CdioDataSource*
cdio_stream_new(void *user_data, const cdio_stream_io_functions *funcs)
{
  CdioDataSource *new_obj;

  new_obj = _cdio_malloc (sizeof (CdioDataSource));

  new_obj->user_data = user_data;
  memcpy(&(new_obj->op), funcs, sizeof(cdio_stream_io_functions));

  return new_obj;
}

long
cdio_stream_read(CdioDataSource* obj, void *ptr, long size, long nmemb)
{
  long read_bytes;

  cdio_assert (obj != NULL);

  if (!_cdio_stream_open_if_necessary(obj)) return 0;

  read_bytes = obj->op.read(obj->user_data, ptr, size*nmemb);
  obj->position += read_bytes;

  return read_bytes;
}

long
cdio_stream_stat(CdioDataSource* obj)
{
  cdio_assert (obj != NULL);

  if (!_cdio_stream_open_if_necessary(obj)) return 0;

  return obj->op.stat(obj->user_data);
}

void
cdio_stream_close(CdioDataSource* obj)
{
  cdio_assert (obj != NULL);

  if (obj->is_open) {
    cdio_debug ("closed source...");
    obj->op.close(obj->user_data);
    obj->is_open = 0;
    obj->position = 0;
  }
}

void
cdio_stream_destroy(CdioDataSource* obj)
{
  cdio_assert (obj != NULL);

  cdio_stream_close(obj);

  obj->op.free(obj->user_data);
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
