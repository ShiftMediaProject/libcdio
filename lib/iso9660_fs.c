/*
    $Id: iso9660_fs.c,v 1.8 2003/09/07 18:15:26 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
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

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <cdio/util.h>

/* Private headers */
#include "cdio_assert.h"
#include "bytesex.h"
#include "ds.h"

#include <stdio.h>

static const char _rcsid[] = "$Id: iso9660_fs.c,v 1.8 2003/09/07 18:15:26 rocky Exp $";

static void
_idr2statbuf (const iso9660_dir_t *idr, iso9660_stat_t *stat, bool is_mode2)
{
  iso9660_xa_t *xa_data = NULL;
  uint8_t dir_len= iso9660_get_dir_len(idr);

  memset ((void *) stat, 0, sizeof (iso9660_stat_t));

  if (!dir_len) return;

  stat->type    = (idr->flags & ISO_DIRECTORY) ? _STAT_DIR : _STAT_FILE;
  stat->lsn     = from_733 (idr->extent);
  stat->size    = from_733 (idr->size);
  stat->secsize = _cdio_len2blocks (stat->size, ISO_BLOCKSIZE);

  iso9660_get_time(idr->date, &(stat->tm));

  cdio_assert (dir_len >= sizeof (iso9660_dir_t));

  if (is_mode2) {
    int su_length = iso9660_get_dir_len(idr) - sizeof (iso9660_dir_t);
    su_length -= idr->name_len;
    
    if (su_length % 2)
      su_length--;
    
    if (su_length < 0 || su_length < sizeof (iso9660_xa_t))
      return;
    
    xa_data = (void *) (((char *) idr) + (iso9660_get_dir_len(idr) - su_length));
    
    if (xa_data->signature[0] != 'X' 
	|| xa_data->signature[1] != 'A')
    {
      cdio_warn ("XA signature not found in ISO9660's system use area;"
		 " ignoring XA attributes for this file entry.");
      cdio_debug ("%d %d %d, '%c%c' (%d, %d)", iso9660_get_dir_len(idr), 
		  idr->name_len,
		  su_length,
		  xa_data->signature[0], xa_data->signature[1],
		  xa_data->signature[0], xa_data->signature[1]);
      return;
    }
    stat->xa = *xa_data;
  }
    
}

static char *
_idr2name (const iso9660_dir_t *idr)
{
  char namebuf[256] = { 0, };

  if (!iso9660_get_dir_len(idr))
    return NULL;

  cdio_assert (iso9660_get_dir_len(idr) >= sizeof (iso9660_dir_t));

  /* (idr->flags & ISO_DIRECTORY) */
  
  if (idr->name[0] == '\0')
    strcpy (namebuf, ".");
  else if (idr->name[0] == '\1')
    strcpy (namebuf, "..");
  else
    strncpy (namebuf, idr->name, idr->name_len);

  return strdup (namebuf);
}


static void
_fs_stat_root (const CdIo *cdio, iso9660_stat_t *stat, bool is_mode2)
{
  char block[ISO_BLOCKSIZE] = { 0, };
  const iso9660_pvd_t *pvd = (void *) &block;
  const iso9660_dir_t *idr = (void *) pvd->root_directory_record;

  if (is_mode2) {
    if (cdio_read_mode2_sector (cdio, &block, ISO_PVD_SECTOR, false))
      cdio_assert_not_reached ();
  } else {
    if (cdio_read_mode1_sector (cdio, &block, ISO_PVD_SECTOR, false))
      cdio_assert_not_reached ();
  }

  _idr2statbuf (idr, stat, is_mode2);
}

static int
_fs_stat_traverse (const CdIo *cdio, const iso9660_stat_t *_root, 
		   char **splitpath, /*out*/ iso9660_stat_t *buf, 
		   bool is_mode2)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;

  if (!splitpath[0])
    {
      *buf = *_root;
      return 0;
    }

  if (_root->type == _STAT_FILE)
    return -1;

  cdio_assert (_root->type == _STAT_DIR);

  if (_root->size != ISO_BLOCKSIZE * _root->secsize)
    {
      cdio_warn ("bad size for ISO9660 directory (%ud) should be (%d)!",
		(unsigned) _root->size, ISO_BLOCKSIZE * _root->secsize);
    }
  
  _dirbuf = _cdio_malloc (_root->secsize * ISO_BLOCKSIZE);

  if (is_mode2) {
    if (cdio_read_mode2_sectors (cdio, _dirbuf, _root->lsn, false, 
				 _root->secsize))
      cdio_assert_not_reached ();
  } else {
    if (cdio_read_mode1_sectors (cdio, _dirbuf, _root->lsn, false,
				 _root->secsize))
      cdio_assert_not_reached ();
  }
  
  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      const iso9660_dir_t *idr = (void *) &_dirbuf[offset];
      iso9660_stat_t stat;
      char *name;

      if (!iso9660_get_dir_len(idr))
	{
	  offset++;
	  continue;
	}
      
      name = _idr2name (idr);
      _idr2statbuf (idr, &stat, is_mode2);

      if (!strcmp (splitpath[0], name))
	{
	  int retval = _fs_stat_traverse (cdio, &stat, &splitpath[1], buf,
					  is_mode2);
	  free (name);
	  free (_dirbuf);
	  return retval;
	}

      free (name);

      offset += iso9660_get_dir_len(idr);
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));
  
  /* not found */
  free (_dirbuf);
  return -1;
}

/*!
  Get file status for pathname into stat. As with libc's stat, 0 is returned
  if no error and -1 on error.
 */
int
iso9660_fs_stat (const CdIo *cdio, const char pathname[], 
		 /*out*/ iso9660_stat_t *stat, bool is_mode2)
{
  iso9660_stat_t _root;
  int retval;
  char **splitpath;

  cdio_assert (cdio != NULL);
  cdio_assert (pathname != NULL);
  cdio_assert (stat != NULL);

  _fs_stat_root (cdio, &_root, is_mode2);

  splitpath = _cdio_strsplit (pathname, '/');
  retval = _fs_stat_traverse (cdio, &_root, splitpath, stat, is_mode2);
  _cdio_strfreev (splitpath);

  return retval;
}

void * /* list of char* -- caller must free it */
iso9660_fs_readdir (const CdIo *cdio, const char pathname[], bool is_mode2)
{
  iso9660_stat_t stat;

  cdio_assert (cdio != NULL);
  cdio_assert (pathname != NULL);

  if (iso9660_fs_stat (cdio, pathname, &stat, is_mode2))
    return NULL;

  if (stat.type != _STAT_DIR)
    return NULL;

  {
    unsigned offset = 0;
    uint8_t *_dirbuf = NULL;
    CdioList *retval = _cdio_list_new ();

    if (stat.size != ISO_BLOCKSIZE * stat.secsize)
      {
	cdio_warn ("bad size for ISO9660 directory (%ud) should be (%d)!",
		  (unsigned) stat.size, ISO_BLOCKSIZE * stat.secsize);
      }

    _dirbuf = _cdio_malloc (stat.secsize * ISO_BLOCKSIZE);

    if (is_mode2) {
      if (cdio_read_mode2_sectors (cdio, _dirbuf, stat.lsn, false, 
				 stat.secsize))
	cdio_assert_not_reached ();
    } else {
      if (cdio_read_mode1_sectors (cdio, _dirbuf, stat.lsn, false,
				   stat.secsize))
	cdio_assert_not_reached ();
    }

    while (offset < (stat.secsize * ISO_BLOCKSIZE))
      {
	const iso9660_dir_t *idr = (void *) &_dirbuf[offset];

	if (!iso9660_get_dir_len(idr))
	  {
	    offset++;
	    continue;
	  }

	_cdio_list_append (retval, _idr2name (idr));

	offset += iso9660_get_dir_len(idr);
      }

    cdio_assert (offset == (stat.secsize * ISO_BLOCKSIZE));

    free (_dirbuf);
    return retval;
  }
}

static bool
find_fs_lsn_recurse (const CdIo *cdio, const char pathname[], 
		     /*out*/ iso9660_stat_t *statbuf, lsn_t lsn)
{
  CdioList *entlist = iso9660_fs_readdir (cdio, pathname, true);
  CdioList *dirlist =  _cdio_list_new ();
  CdioListNode *entnode;
    
  cdio_assert (entlist != NULL);

  /* iterate over each entry in the directory */
  
  _CDIO_LIST_FOREACH (entnode, entlist)
    {
      char *name = _cdio_list_node_data (entnode);
      char _fullname[4096] = { 0, };

      snprintf (_fullname, sizeof (_fullname), "%s%s", pathname, name);
  
      if (iso9660_fs_stat (cdio, _fullname, statbuf, true))
        cdio_assert_not_reached ();

      strncat (_fullname, "/", sizeof (_fullname));

      if (statbuf->type == _STAT_DIR
          && strcmp (name, ".") 
          && strcmp (name, ".."))
        _cdio_list_append (dirlist, strdup (_fullname));

      if (statbuf->lsn == lsn) {
        _cdio_list_free (entlist, true);
        _cdio_list_free (dirlist, true);
        return true;
      }
      
    }

  _cdio_list_free (entlist, true);

  /* now recurse/descend over directories encountered */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);

      if (find_fs_lsn_recurse (cdio, _fullname, statbuf, lsn)) {
        _cdio_list_free (dirlist, true);
        return true;
      }
    }

  _cdio_list_free (dirlist, true);
  return false;
}

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it in stat. 

   Returns true if we found an entry with the lsn and false if not.
 */
bool
iso9660_find_fs_lsn(const CdIo *cdio, lsn_t lsn, /*out*/ iso9660_stat_t *stat)
{
  return find_fs_lsn_recurse (cdio, "/", stat, lsn);
}

