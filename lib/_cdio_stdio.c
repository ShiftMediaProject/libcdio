/*
    $Id: _cdio_stdio.c,v 1.2 2003/03/29 17:32:00 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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
#include <unistd.h> 
#include <sys/stat.h>
#include <errno.h>

#include "logging.h"
#include "util.h"
#include "_cdio_stream.h"
#include "_cdio_stdio.h"

static const char _rcsid[] = "$Id: _cdio_stdio.c,v 1.2 2003/03/29 17:32:00 rocky Exp $";

#define CDIO_STDIO_BUFSIZE (128*1024)

typedef struct {
  char *pathname;
  FILE *fd;
  char *fd_buf;
  off_t st_size; /* used only for source */
} _UserData;

static int
_stdio_open (void *user_data) 
{
  _UserData *const ud = user_data;
  
  if ((ud->fd = fopen (ud->pathname, "rb")))
    {
      ud->fd_buf = _cdio_malloc (CDIO_STDIO_BUFSIZE);
      setvbuf (ud->fd, ud->fd_buf, _IOFBF, CDIO_STDIO_BUFSIZE);
    }

  return (ud->fd == NULL);
}

static int
_stdio_close(void *user_data)
{
  _UserData *const ud = user_data;

  if (fclose (ud->fd))
    cdio_error ("fclose (): %s", strerror (errno));
 
  ud->fd = NULL;

  free (ud->fd_buf);
  ud->fd_buf = NULL;

  return 0;
}

static void
_stdio_free(void *user_data)
{
  _UserData *const ud = user_data;

  if (ud->pathname)
    free(ud->pathname);

  if (ud->fd) /* should be NULL anyway... */
    _stdio_close(user_data); 

  free(ud);
}

static long
_stdio_seek(void *user_data, long offset, int whence)
{
  _UserData *const ud = user_data;

  if (fseek (ud->fd, offset, whence))
    cdio_error ("fseek (): %s", strerror (errno));

  return offset;
}

static long
_stdio_stat(void *user_data)
{
  const _UserData *const ud = user_data;

  return ud->st_size;
}

static long
_stdio_read(void *user_data, void *buf, long count)
{
  _UserData *const ud = user_data;
  long read;

  read = fread(buf, 1, count, ud->fd);

  if (read != count)
    { /* fixme -- ferror/feof */
      if (feof (ud->fd))
        {
          cdio_debug ("fread (): EOF encountered");
          clearerr (ud->fd);
        }
      else if (ferror (ud->fd))
        {
          cdio_error ("fread (): %s", strerror (errno));
          clearerr (ud->fd);
        }
      else
        cdio_debug ("fread (): short read and no EOF?!?");
    }

  return read;
}

CdioDataSource*
cdio_stdio_new(const char pathname[])
{
  CdioDataSource *new_obj = NULL;
  cdio_stream_io_functions funcs = { 0, };
  _UserData *ud = NULL;
  struct stat statbuf;
  
  if (stat (pathname, &statbuf) == -1) 
    {
      cdio_error ("could not stat() file `%s': %s", pathname, strerror (errno));
      return NULL;
    }

  ud = _cdio_malloc (sizeof (_UserData));

  ud->pathname = strdup(pathname);
  ud->st_size  = statbuf.st_size; /* let's hope it doesn't change... */

  funcs.open   = _stdio_open;
  funcs.seek   = _stdio_seek;
  funcs.stat   = _stdio_stat;
  funcs.read   = _stdio_read;
  funcs.close  = _stdio_close;
  funcs.free   = _stdio_free;

  new_obj = cdio_stream_new(ud, &funcs);

  return new_obj;
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
