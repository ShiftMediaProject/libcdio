/*
    $Id: _cdio_generic.c,v 1.2 2003/03/29 21:13:55 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002,2003 Rocky Bernstein <rocky@panix.com>

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

/* This file contains Linux-specific code and implements low-level 
   control of the CD drive.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

static const char _rcsid[] = "$Id: _cdio_generic.c,v 1.2 2003/03/29 21:13:55 rocky Exp $";

#include "cdio_assert.h"
#include "cdio_private.h"
#include "sector.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>

/*!
  Release and free resources associated with cd. 
 */
void
cdio_generic_free (void *user_data)
{
  generic_img_private_t *_obj = user_data;

  if (NULL == _obj) return;
  free (_obj->source_name);

  if (_obj->fd >= 0)
    close (_obj->fd);

  free (_obj);
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Is in fact libc's read().
*/
off_t
cdio_generic_lseek (void *user_data, off_t offset, int whence)
{
  generic_img_private_t *_obj = user_data;
  return lseek(_obj->fd, offset, whence);
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Is in fact libc's read().
*/
ssize_t
cdio_generic_read (void *user_data, void *buf, size_t size)
{
  generic_img_private_t *_obj = user_data;
  return read(_obj->fd, buf, size);
}


int
cdio_generic_read_mode2_sectors (CdIo *obj, void *buf, lsn_t lsn, 
				 bool mode2raw, unsigned num_sectors)
{
  char *_buf = buf;
  const int blocksize = mode2raw ? M2RAW_SECTOR_SIZE : M2F1_SECTOR_SIZE;
  int n, rc;

  cdio_assert (obj != NULL);
  cdio_assert (buf != NULL);
  cdio_assert (obj->op.read_mode2_sector != NULL);

  for (n = 0; n < num_sectors; n++)
    if ((rc = cdio_read_mode2_sector (obj, &_buf[n * blocksize],
				      lsn + n, mode2raw)))
      return rc;
  
  return 0;
}

