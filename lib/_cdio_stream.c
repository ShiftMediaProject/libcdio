/*
    $Id: _cdio_stream.c,v 1.4 2003/04/06 23:40:21 rocky Exp $

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

static const char _rcsid[] = "$Id: _cdio_stream.c,v 1.4 2003/04/06 23:40:21 rocky Exp $";

/* 
 * DataSource implementations
 */

struct _CdioDataSource {
  void* user_data;
  cdio_stream_io_functions op;
  int is_open;
  long position;
};

/* 
   Open if not already open. 
   Return false if we hit an error. Errno should be set for that error.
*/
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

/*! 
  Like 3 fseek and in fact may be the same.
  
  This  function sets the file position indicator for the stream
  pointed to by stream.  The new position, measured in bytes, is obtained
  by  adding offset bytes to the position specified by whence.  If whence
  is set to SEEK_SET, SEEK_CUR, or SEEK_END, the offset  is  relative  to
  the  start of the file, the current position indicator, or end-of-file,
  respectively.  A successful call to the fseek function clears the end-
  of-file indicator for the stream and undoes any effects of the
  ungetc(3) function on the same stream.
  
  RETURN VALUE
  Upon successful completion, return 0,
  Otherwise, -1 is returned and the global variable errno is set to indi-
  cate the error.
*/
long
cdio_stream_seek(CdioDataSource* obj, long offset, int whence)
{
  cdio_assert (obj != NULL);

  if (!_cdio_stream_open_if_necessary(obj)) 
    /* errno is set by _cdio_stream_open_if necessary. */
    return -1;

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

/*!
  Like fread(3) and in fact may be the same.
  
  DESCRIPTION:
  The function fread reads nmemb elements of data, each size bytes long,
  from the stream pointed to by stream, storing them at the location
  given by ptr.
  
  RETURN VALUE:
  return the number of items successfully read or written (i.e.,
  not the number of characters).  If an error occurs, or the
  end-of-file is reached, the return value is a short item count
  (or zero).
  
  We do not distinguish between end-of-file and error, and callers
  must use feof(3) and ferror(3) to determine which occurred.
*/
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

/*!
  Return whatever size of stream reports, I guess unit size is bytes. 
  On error return -1;
 */
long int
cdio_stream_stat(CdioDataSource* obj)
{
  cdio_assert (obj != NULL);

  if (!_cdio_stream_open_if_necessary(obj)) return -1;

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
