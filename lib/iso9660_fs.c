/*
    $Id: iso9660_fs.c,v 1.16 2004/02/07 18:53:02 rocky Exp $

    Copyright (C) 2001 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

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
#include "_cdio_stdio.h"
#include "cdio_private.h"

#include <stdio.h>

static const char _rcsid[] = "$Id: iso9660_fs.c,v 1.16 2004/02/07 18:53:02 rocky Exp $";

/* Implementation of iso9660_t type */
struct _iso9660 {
  CdioDataSource *stream; /* Stream pointer */
  void *env;              /* environment. */
};

/*!
  Open an ISO 9660 image for reading. Maybe in the future we will have
  flags and mode. NULL is returned on error.
*/
iso9660_t *
iso9660_open (const char *pathname /*flags, mode */)
{
  iso9660_t *iso = (iso9660_t *) _cdio_malloc(sizeof(struct _iso9660)) ;

  if (NULL == iso) return NULL;
  
  iso->stream = cdio_stdio_new( pathname );
  if (NULL == iso->stream) {
    free(iso);
    return NULL;
  }
  
  return iso;
}



/*!
  Close previously opened ISO 9660 image.
  True is unconditionally returned. If there was an error false would
  be returned.
*/
bool 
iso9660_close (iso9660_t *iso)
{
  if (NULL != iso) {
    cdio_stdio_destroy(iso->stream);
    free(iso);
  }
  return true;
}



/*!
  Seek to a position and then read n blocks. Size read is returned.
*/
long int 
iso9660_iso_seek_read (iso9660_t *iso, void *ptr, lsn_t start, long int size)
{
  long int ret;
  if (NULL == iso) return 0;
  ret = cdio_stream_seek (iso->stream, start * ISO_BLOCKSIZE, SEEK_SET);
  if (ret!=0) return 0;
  return cdio_stream_read (iso->stream, ptr, ISO_BLOCKSIZE, size);
}


static iso9660_stat_t *
_iso9660_dir_to_statbuf (const iso9660_dir_t *iso9660_dir, bool is_mode2)
{
  iso9660_xa_t *xa_data = NULL;
  uint8_t dir_len= iso9660_get_dir_len(iso9660_dir);
  unsigned int filename_len;
  unsigned int stat_len;
  iso9660_stat_t *stat;

  if (!dir_len) return NULL;

  filename_len  = from_711(iso9660_dir->filename_len);

  /* .. string in statbuf is one longer than in iso9660_dir's listing '\1' */
  stat_len      = sizeof(iso9660_stat_t)+filename_len+2;

  stat          = _cdio_malloc(stat_len);
  stat->type    = (iso9660_dir->file_flags & ISO_DIRECTORY) 
    ? _STAT_DIR : _STAT_FILE;
  stat->lsn     = from_733 (iso9660_dir->extent);
  stat->size    = from_733 (iso9660_dir->size);
  stat->secsize = _cdio_len2blocks (stat->size, ISO_BLOCKSIZE);

  if (iso9660_dir->filename[0] == '\0')
    strcpy (stat->filename, ".");
  else if (iso9660_dir->filename[0] == '\1')
    strcpy (stat->filename, "..");
  else
    strncpy (stat->filename, iso9660_dir->filename, filename_len);

  iso9660_get_dtime(&(iso9660_dir->recording_time), true, &(stat->tm));

  cdio_assert (dir_len >= sizeof (iso9660_dir_t));

  if (is_mode2) {
    int su_length = iso9660_get_dir_len(iso9660_dir) - sizeof (iso9660_dir_t);
    su_length -= filename_len;
    
    if (su_length % 2)
      su_length--;
    
    if (su_length < 0 || su_length < sizeof (iso9660_xa_t))
      return stat;
    
    xa_data = (void *) (((char *) iso9660_dir) 
			+ (iso9660_get_dir_len(iso9660_dir) - su_length));
    
    if (xa_data->signature[0] != 'X' 
	|| xa_data->signature[1] != 'A')
    {
      cdio_warn ("XA signature not found in ISO9660's system use area;"
		 " ignoring XA attributes for this file entry.");
      cdio_debug ("%d %d %d, '%c%c' (%d, %d)", 
		  iso9660_get_dir_len(iso9660_dir), 
		  filename_len,
		  su_length,
		  xa_data->signature[0], xa_data->signature[1],
		  xa_data->signature[0], xa_data->signature[1]);
      return stat;
    }
    stat->xa = *xa_data;
  }
  return stat;
    
}

/*!
  Return the directory name stored in the iso9660_dir_t

  A string is allocated: the caller must deallocate.
 */
char *
iso9660_dir_to_name (const iso9660_dir_t *iso9660_dir)
{
  char namebuf[256] = { 0, };
  uint8_t len=iso9660_get_dir_len(iso9660_dir);

  if (!len) return NULL;

  cdio_assert (len >= sizeof (iso9660_dir_t));

  /* (iso9660_dir->file_flags & ISO_DIRECTORY) */
  
  if (iso9660_dir->filename[0] == '\0')
    strcpy (namebuf, ".");
  else if (iso9660_dir->filename[0] == '\1')
    strcpy (namebuf, "..");
  else
    strncpy (namebuf, iso9660_dir->filename, iso9660_dir->filename_len);

  return strdup (namebuf);
}


static iso9660_stat_t *
_fs_stat_root (const CdIo *cdio, bool is_mode2)
{
  char block[ISO_BLOCKSIZE] = { 0, };
  const iso9660_pvd_t *pvd = (void *) &block;
  const iso9660_dir_t *iso9660_dir = (void *) pvd->root_directory_record;
  iso9660_stat_t *stat;

  if (is_mode2) {
    if (cdio_read_mode2_sector (cdio, &block, ISO_PVD_SECTOR, false))
      cdio_assert_not_reached ();
  } else {
    if (cdio_read_mode1_sector (cdio, &block, ISO_PVD_SECTOR, false))
      cdio_assert_not_reached ();
  }

  stat = _iso9660_dir_to_statbuf (iso9660_dir, is_mode2);
  return stat;
}

static iso9660_stat_t *
_fs_stat_iso_root (iso9660_t *iso)
{
  char block[ISO_BLOCKSIZE] = { 0, };
  const iso9660_pvd_t *pvd = (void *) &block;
  const iso9660_dir_t *iso9660_dir = (void *) pvd->root_directory_record;
  iso9660_stat_t *stat;
  int ret;

  ret = iso9660_iso_seek_read (iso, block, ISO_PVD_SECTOR, 1);
  if (ret!=ISO_BLOCKSIZE) return NULL;

  stat = _iso9660_dir_to_statbuf (iso9660_dir, true);
  return stat;
}

static iso9660_stat_t *
_fs_stat_traverse (const CdIo *cdio, const iso9660_stat_t *_root, 
		   char **splitpath, bool is_mode2)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;
  iso9660_stat_t *stat;

  if (!splitpath[0])
    {
      unsigned int len=sizeof(iso9660_stat_t) + strlen(_root->filename)+1;
      stat = _cdio_malloc(len);
      memcpy(stat, _root, len);
      return stat;
    }

  if (_root->type == _STAT_FILE)
    return NULL;

  cdio_assert (_root->type == _STAT_DIR);

  if (_root->size != ISO_BLOCKSIZE * _root->secsize)
    {
      cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		 (unsigned) _root->size, 
		 (unsigned long int) ISO_BLOCKSIZE * _root->secsize);
    }
  
  _dirbuf = _cdio_malloc (_root->secsize * ISO_BLOCKSIZE);

  if (is_mode2) {
    if (cdio_read_mode2_sectors (cdio, _dirbuf, _root->lsn, false, 
				 _root->secsize))
      return NULL;
  } else {
    if (cdio_read_mode1_sectors (cdio, _dirbuf, _root->lsn, false,
				 _root->secsize))
      return NULL;
  }
  
  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      const iso9660_dir_t *iso9660_dir = (void *) &_dirbuf[offset];
      iso9660_stat_t *stat;

      if (!iso9660_get_dir_len(iso9660_dir))
	{
	  offset++;
	  continue;
	}
      
      stat = _iso9660_dir_to_statbuf (iso9660_dir, is_mode2);

      if (!strcmp (splitpath[0], stat->filename))
	{
	  iso9660_stat_t *ret_stat 
	    = _fs_stat_traverse (cdio, stat, &splitpath[1], is_mode2);
	  free(stat);
	  free (_dirbuf);
	  return ret_stat;
	}

      free(stat);
	  
      offset += iso9660_get_dir_len(iso9660_dir);
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));
  
  /* not found */
  free (_dirbuf);
  return NULL;
}

static iso9660_stat_t *
_fs_iso_stat_traverse (iso9660_t *iso, const iso9660_stat_t *_root, 
		       char **splitpath)
{
  unsigned offset = 0;
  uint8_t *_dirbuf = NULL;
  iso9660_stat_t *stat;
  int ret;

  if (!splitpath[0])
    {
      unsigned int len=sizeof(iso9660_stat_t) + strlen(_root->filename)+1;
      stat = _cdio_malloc(len);
      memcpy(stat, _root, len);
      return stat;
    }

  if (_root->type == _STAT_FILE)
    return NULL;

  cdio_assert (_root->type == _STAT_DIR);

  if (_root->size != ISO_BLOCKSIZE * _root->secsize)
    {
      cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		 (unsigned) _root->size, 
		 (unsigned long int) ISO_BLOCKSIZE * _root->secsize);
    }
  
  _dirbuf = _cdio_malloc (_root->secsize * ISO_BLOCKSIZE);

  ret = iso9660_iso_seek_read (iso, _dirbuf, _root->lsn, _root->secsize);
  if (ret!=ISO_BLOCKSIZE*_root->secsize) return NULL;
  
  while (offset < (_root->secsize * ISO_BLOCKSIZE))
    {
      const iso9660_dir_t *iso9660_dir = (void *) &_dirbuf[offset];
      iso9660_stat_t *stat;

      if (!iso9660_get_dir_len(iso9660_dir))
	{
	  offset++;
	  continue;
	}
      
      stat = _iso9660_dir_to_statbuf (iso9660_dir, true);

      if (!strcmp (splitpath[0], stat->filename))
	{
	  iso9660_stat_t *ret_stat 
	    = _fs_iso_stat_traverse (iso, stat, &splitpath[1]);
	  free(stat);
	  free (_dirbuf);
	  return ret_stat;
	}

      free(stat);
	  
      offset += iso9660_get_dir_len(iso9660_dir);
    }

  cdio_assert (offset == (_root->secsize * ISO_BLOCKSIZE));
  
  /* not found */
  free (_dirbuf);
  return NULL;
}

/*!
  Get file status for pathname into stat. NULL is returned on error.
 */
iso9660_stat_t *
iso9660_fs_stat (const CdIo *cdio, const char pathname[], bool is_mode2)
{
  iso9660_stat_t *root;
  char **splitpath;
  iso9660_stat_t *stat;

  if (cdio == NULL)     return NULL;
  if (pathname == NULL) return NULL;

  root = _fs_stat_root (cdio, is_mode2);
  if (NULL == root) return NULL;

  splitpath = _cdio_strsplit (pathname, '/');
  stat = _fs_stat_traverse (cdio, root, splitpath, is_mode2);
  free(root);
  _cdio_strfreev (splitpath);

  return stat;
}

/*!
  Get file status for pathname into stat. NULL is returned on error.
 */
void *
iso9660_ifs_stat (iso9660_t *iso, const char pathname[])
{
  iso9660_stat_t *root;
  char **splitpath;
  iso9660_stat_t *stat;

  if (iso == NULL)      return NULL;
  if (pathname == NULL) return NULL;

  root = _fs_stat_iso_root (iso);
  if (NULL == root) return NULL;

  splitpath = _cdio_strsplit (pathname, '/');
  stat = _fs_iso_stat_traverse (iso, root, splitpath);
  free(root);
  _cdio_strfreev (splitpath);

  return stat;
}


/*! 
  Read pathname (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must free the returned result.
*/
void * 
iso9660_fs_readdir (const CdIo *cdio, const char pathname[], bool is_mode2)
{
  iso9660_stat_t *stat;

  if (NULL == cdio)     return NULL;
  if (NULL == pathname) return NULL;

  stat = iso9660_fs_stat (cdio, pathname, is_mode2);
  if (NULL == stat)
    return NULL;

  if (stat->type != _STAT_DIR) {
    free(stat);
    return NULL;
  }

  {
    unsigned offset = 0;
    uint8_t *_dirbuf = NULL;
    CdioList *retval = _cdio_list_new ();

    if (stat->size != ISO_BLOCKSIZE * stat->secsize)
      {
	cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		   (unsigned) stat->size, 
		   (unsigned long int) ISO_BLOCKSIZE * stat->secsize);
      }

    _dirbuf = _cdio_malloc (stat->secsize * ISO_BLOCKSIZE);

    if (is_mode2) {
      if (cdio_read_mode2_sectors (cdio, _dirbuf, stat->lsn, false, 
				   stat->secsize))
	cdio_assert_not_reached ();
    } else {
      if (cdio_read_mode1_sectors (cdio, _dirbuf, stat->lsn, false,
				   stat->secsize))
	cdio_assert_not_reached ();
    }

    while (offset < (stat->secsize * ISO_BLOCKSIZE))
      {
	const iso9660_dir_t *iso9660_dir = (void *) &_dirbuf[offset];
	iso9660_stat_t *iso9660_stat;
	
	if (!iso9660_get_dir_len(iso9660_dir))
	  {
	    offset++;
	    continue;
	  }

	iso9660_stat = _iso9660_dir_to_statbuf(iso9660_dir, is_mode2);
	_cdio_list_append (retval, iso9660_stat);

	offset += iso9660_get_dir_len(iso9660_dir);
      }

    cdio_assert (offset == (stat->secsize * ISO_BLOCKSIZE));

    free (_dirbuf);
    free (stat);
    return retval;
  }
}

/*! 
  Read pathname (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must free the returned result.
*/
void * 
iso9660_ifs_readdir (iso9660_t *iso, const char pathname[])
{
  iso9660_stat_t *stat;

  if (NULL == iso)      return NULL;
  if (NULL == pathname) return NULL;

  stat = iso9660_ifs_stat (iso, pathname);
  if (NULL == stat)     return NULL;

  if (stat->type != _STAT_DIR) {
    free(stat);
    return NULL;
  }

  {
    long int ret;
    unsigned offset = 0;
    uint8_t *_dirbuf = NULL;
    CdioList *retval = _cdio_list_new ();

    if (stat->size != ISO_BLOCKSIZE * stat->secsize)
      {
	cdio_warn ("bad size for ISO9660 directory (%ud) should be (%lu)!",
		   (unsigned) stat->size, 
		   (unsigned long int) ISO_BLOCKSIZE * stat->secsize);
      }

    _dirbuf = _cdio_malloc (stat->secsize * ISO_BLOCKSIZE);

    ret = iso9660_iso_seek_read (iso, _dirbuf, stat->lsn, stat->secsize);
    if (ret != ISO_BLOCKSIZE*stat->secsize) return NULL;
    
    while (offset < (stat->secsize * ISO_BLOCKSIZE))
      {
	const iso9660_dir_t *iso9660_dir = (void *) &_dirbuf[offset];
	iso9660_stat_t *iso9660_stat;
	
	if (!iso9660_get_dir_len(iso9660_dir))
	  {
	    offset++;
	    continue;
	  }

	iso9660_stat = _iso9660_dir_to_statbuf(iso9660_dir, true);
	_cdio_list_append (retval, iso9660_stat);

	offset += iso9660_get_dir_len(iso9660_dir);
      }

    cdio_assert (offset == (stat->secsize * ISO_BLOCKSIZE));

    free (_dirbuf);
    free (stat);
    return retval;
  }
}

static iso9660_stat_t *
find_fs_lsn_recurse (const CdIo *cdio, const char pathname[], lsn_t lsn)
{
  CdioList *entlist = iso9660_fs_readdir (cdio, pathname, true);
  CdioList *dirlist =  _cdio_list_new ();
  CdioListNode *entnode;
    
  cdio_assert (entlist != NULL);

  /* iterate over each entry in the directory */
  
  _CDIO_LIST_FOREACH (entnode, entlist)
    {
      iso9660_stat_t *statbuf = _cdio_list_node_data (entnode);
      char _fullname[4096] = { 0, };
      char *filename = (char *) statbuf->filename;

      snprintf (_fullname, sizeof (_fullname), "%s%s", pathname, filename);
  
      strncat (_fullname, "/", sizeof (_fullname));

      if (statbuf->type == _STAT_DIR
          && strcmp ((char *) statbuf->filename, ".") 
          && strcmp ((char *) statbuf->filename, ".."))
        _cdio_list_append (dirlist, strdup (_fullname));

      if (statbuf->lsn == lsn) {
	unsigned int len=sizeof(iso9660_stat_t)+strlen(statbuf->filename)+1;
	iso9660_stat_t *ret_stat = _cdio_malloc(len);
	memcpy(ret_stat, statbuf, len);
        _cdio_list_free (entlist, true);
        _cdio_list_free (dirlist, true);
        return ret_stat;
      }
      
    }

  _cdio_list_free (entlist, true);

  /* now recurse/descend over directories encountered */

  _CDIO_LIST_FOREACH (entnode, dirlist)
    {
      char *_fullname = _cdio_list_node_data (entnode);
      iso9660_stat_t *ret_stat = find_fs_lsn_recurse (cdio, _fullname, lsn);

      if (NULL != ret_stat) {
        _cdio_list_free (dirlist, true);
        return ret_stat;
      }
    }

  _cdio_list_free (dirlist, true);
  return NULL;
}

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it.

   Returns stat_t of entry if we found lsn, or NULL otherwise.
 */
iso9660_stat_t *
iso9660_find_fs_lsn(const CdIo *cdio, lsn_t lsn)
{
  return find_fs_lsn_recurse (cdio, "/", lsn);
}

