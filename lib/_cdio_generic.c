/*
    $Id: _cdio_generic.c,v 1.14 2004/05/05 10:34:55 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002, 2003, 2004 Rocky Bernstein <rocky@panix.com>

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

static const char _rcsid[] = "$Id: _cdio_generic.c,v 1.14 2004/05/05 10:34:55 rocky Exp $";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"
#include "_cdio_stdio.h"

/*!
  Eject media -- there's nothing to do here. We always return 2.
  Should we also free resources? 
 */
int 
cdio_generic_bogus_eject_media (void *user_data) {
  /* Sort of a stub here. Perhaps log a message? */
  return 2;
}


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
  Initialize CD device.
 */
bool
cdio_generic_init (void *user_data)
{
  generic_img_private_t *_obj = user_data;
  if (_obj->init) {
    cdio_error ("init called more than once");
    return false;
  }
  
  _obj->fd = open (_obj->source_name, O_RDONLY, 0);

  if (_obj->fd < 0)
    {
      cdio_warn ("open (%s): %s", _obj->source_name, strerror (errno));
      return false;
    }

  _obj->init = true;
  _obj->toc_init = false;
  return true;
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

/*!
  Release and free resources associated with stream or disk image.
*/
void
cdio_generic_stdio_free (void *user_data)
{
  generic_img_private_t *_obj = user_data;

  if (NULL == _obj) return;
  if (NULL != _obj->source_name) 
    free (_obj->source_name);

  if (_obj->data_source)
    cdio_stdio_destroy (_obj->data_source);
}


/*!  
  Return true if source_name could be a device containing a CD-ROM.
*/
bool
cdio_is_device_generic(const char *source_name)
{
  struct stat buf;
  if (0 != stat(source_name, &buf)) {
    cdio_warn ("Can't get file status for %s:\n%s", source_name, 
		strerror(errno));
    return false;
  }
  return (S_ISBLK(buf.st_mode) || S_ISCHR(buf.st_mode));
}

/*!  
  Like above, but don't give a warning device doesn't exist.
*/
bool
cdio_is_device_quiet_generic(const char *source_name)
{
  struct stat buf;
  if (0 != stat(source_name, &buf)) {
    return false;
  }
  return (S_ISBLK(buf.st_mode) || S_ISCHR(buf.st_mode));
}

/*! 
  Add/allocate a drive to the end of drives. 
  Use cdio_free_device_list() to free this device_list.
*/
void 
cdio_add_device_list(char **device_list[], const char *drive, int *num_drives)
{
  if (NULL != drive) {
    unsigned int j;

    /* Check if drive is already in list. */
    for (j=0; j<*num_drives; j++) {
      if (strcmp((*device_list)[j], drive) == 0) break;
    }

    if (j==*num_drives) {
      /* Drive not in list. Add it. */
      (*num_drives)++;
      if (*device_list) {
	*device_list = realloc(*device_list, (*num_drives) * sizeof(char *));
      } else {
	/* num_drives should be 0. Add assert? */
	*device_list = malloc((*num_drives) * sizeof(char *));
      }
      
      (*device_list)[*num_drives-1] = strdup(drive);
    }
  } else {
    (*num_drives)++;
    if (*device_list) {
      *device_list = realloc(*device_list, (*num_drives) * sizeof(char *));
    } else {
      *device_list = malloc((*num_drives) * sizeof(char *));
    }
    (*device_list)[*num_drives-1] = NULL;
  }
}


