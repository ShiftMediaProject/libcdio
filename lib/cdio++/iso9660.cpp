/* -*- C++ -*-
    $Id: iso9660.cpp,v 1.2 2006/03/07 19:55:11 rocky Exp $

    Copyright (C) 2006 Rocky Bernstein <rocky@panix.com>

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
#include <cdio++/iso9660.hpp>

/*!
  Given a directory pointer, find the filesystem entry that contains
  lsn and return information about it.
  
  @return Stat * of entry if we found lsn, or NULL otherwise.
  Caller must free return value.
*/
ISO9660::Stat *
ISO9660::FS::find_lsn(lsn_t i_lsn) 
{
  return new Stat(iso9660_find_fs_lsn(p_cdio, i_lsn));
}

/*!
  Read the Primary Volume Descriptor for a CD.
  True is returned if read, and false if there was an error.
*/
ISO9660::PVD *
ISO9660::FS::read_pvd ()
{
  iso9660_pvd_t pvd;
  bool b_okay = iso9660_fs_read_pvd (p_cdio, &pvd);
  if (b_okay) {
    return new PVD(&pvd);
  }
  return (PVD *) NULL;
}

/*!
  Read the Super block of an ISO 9660 image. This is the 
  Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume 
  Descriptor if (Joliet) extensions are acceptable.
*/
bool 
ISO9660::FS::read_superblock (iso_extension_mask_t iso_extension_mask) 
{
  return iso9660_fs_read_superblock (p_cdio, iso_extension_mask);
}

/*! Read psz_path (a directory) and return a list of iso9660_stat_t
  pointers for the files inside that directory. The caller must free the
  returned result.
*/
bool 
ISO9660::FS::readdir (const char psz_path[], stat_vector_t& stat_vector,
		      bool b_mode2)
{
  CdioList_t * p_stat_list = iso9660_fs_readdir (p_cdio, psz_path, 
						 b_mode2);
  if (p_stat_list) {
    CdioListNode_t *p_entnode;
    _CDIO_LIST_FOREACH (p_entnode, p_stat_list) {
      iso9660_stat_t *p_statbuf = 
	(iso9660_stat_t *) _cdio_list_node_data (p_entnode);
      stat_vector.push_back(new ISO9660::Stat(p_statbuf));
    }
    _cdio_list_free (p_stat_list, false);
    return true;
  } else {
    return false;
  }
}

/*!
  Return file status for path name psz_path. NULL is returned on
  error.
  
  If translate is true, version numbers in the ISO 9660 name are
  dropped, i.e. ;1 is removed and if level 1 ISO-9660 names are
  lowercased.
  
  Mode2 is used only if translate is true and is a hack that
  really should go away in libcdio sometime. If set use mode 2
  reading, otherwise use mode 1 reading.
  
  @return file status object for psz_path. NULL is returned on
  error.
*/
ISO9660::Stat *
ISO9660::FS::stat (const char psz_path[], bool b_translate, bool b_mode2) 
{
  if (b_translate) 
    return new ISO9660::Stat(iso9660_fs_stat_translate (p_cdio, psz_path, 
							b_mode2));
  else 
    return new ISO9660::Stat(iso9660_fs_stat (p_cdio, psz_path));
}

/*! Close previously opened ISO 9660 image and free resources
  associated with the image. Call this when done using using an ISO
  9660 image.
  
  @return true is unconditionally returned. If there was an error
  false would be returned.
*/
bool 
ISO9660::IFS::close()
{
  iso9660_close(p_iso9660);
  p_iso9660 = (iso9660_t *) NULL;
  return true;
}

/*!
  Given a directory pointer, find the filesystem entry that contains
  lsn and return information about it.
  
  Returns Stat*  of entry if we found lsn, or NULL otherwise.
*/
ISO9660::Stat *
ISO9660::IFS::find_lsn(lsn_t i_lsn) 
{
  return new Stat(iso9660_ifs_find_lsn(p_iso9660, i_lsn));
}

/*!  
  Get the application ID.  psz_app_id is set to NULL if there
  is some problem in getting this and false is returned.
*/
bool 
ISO9660::IFS::get_application_id(/*out*/ char * &psz_app_id) 
{
  return iso9660_ifs_get_application_id(p_iso9660, &psz_app_id);
}

/*!  
  Return the Joliet level recognized. 
*/
uint8_t
ISO9660::IFS::get_joliet_level()
{
  return iso9660_ifs_get_joliet_level(p_iso9660);
}

/*!  
  Get the preparer ID.  psz_preparer_id is set to NULL if there
  is some problem in getting this and false is returned.
*/
bool
ISO9660::IFS::get_preparer_id(/*out*/ char * &psz_preparer_id) 
{
  return iso9660_ifs_get_preparer_id(p_iso9660, &psz_preparer_id);
}

/*!  
  Get the publisher ID.  psz_publisher_id is set to NULL if there
  is some problem in getting this and false is returned.
*/
bool
ISO9660::IFS::get_publisher_id(/*out*/ char * &psz_publisher_id) 
{
  return iso9660_ifs_get_publisher_id(p_iso9660, &psz_publisher_id);
}

/*!  
  Get the system ID.  psz_system_id is set to NULL if there
  is some problem in getting this and false is returned.
*/
bool
ISO9660::IFS::get_system_id(/*out*/ char * &psz_system_id) 
{
  return iso9660_ifs_get_system_id(p_iso9660, &psz_system_id);
}

/*! Return the volume ID in the PVD. psz_volume_id is set to
  NULL if there is some problem in getting this and false is
  returned.
*/
bool
ISO9660::IFS::get_volume_id(/*out*/ char * &psz_volume_id) 
{
  return iso9660_ifs_get_volume_id(p_iso9660, &psz_volume_id);
}

/*! Return the volumeset ID in the PVD. psz_volumeset_id is set to
  NULL if there is some problem in getting this and false is
  returned.
*/
bool
ISO9660::IFS::get_volumeset_id(/*out*/ char * &psz_volumeset_id) 
{
  return iso9660_ifs_get_volumeset_id(p_iso9660, &psz_volumeset_id);
}

/*!
  Return true if ISO 9660 image has extended attrributes (XA).
*/
bool
ISO9660::IFS::is_xa () 
{
  return iso9660_ifs_is_xa (p_iso9660);
}

/*! Open an ISO 9660 image for reading. Maybe in the future we will
  have a mode. NULL is returned on error. An open routine should be
  called before using any read routine. If device object was
  previously opened it is closed first.
  
  @param psz_path location of ISO 9660 image
  @param iso_extension_mask the kinds of ISO 9660 extensions will be
  considered on access.
  
  @return true if open succeeded or false if error.
  
  @see open_fuzzy
*/
bool
ISO9660::IFS::open(const char *psz_path, 
		   iso_extension_mask_t iso_extension_mask)
{
  if (p_iso9660) iso9660_close(p_iso9660);
  p_iso9660 = iso9660_open_ext(psz_path, iso_extension_mask);
  return NULL != (iso9660_t *) p_iso9660 ;
}

/*! Open an ISO 9660 image for "fuzzy" reading. This means that we
  will try to guess various internal offset based on internal
  checks. This may be useful when trying to read an ISO 9660 image
  contained in a file format that libiso9660 doesn't know natively
  (or knows imperfectly.)
  
  Maybe in the future we will have a mode. NULL is returned on
  error.
  
  @see open
*/
bool
ISO9660::IFS::open_fuzzy (const char *psz_path,
			  iso_extension_mask_t iso_extension_mask,
			  uint16_t i_fuzz)
{
  p_iso9660 = iso9660_open_fuzzy_ext(psz_path, iso_extension_mask, i_fuzz);
  //return p_iso9660 != (iso9660_t *) NULL;
  return true;
}

/*! Read the Primary Volume Descriptor for an ISO 9660 image.  A
  PVD object is returned if read, and NULL if there was an error.
*/
ISO9660::PVD *
ISO9660::IFS::read_pvd () 
{
  iso9660_pvd_t pvd;
  bool b_okay = iso9660_ifs_read_pvd (p_iso9660, &pvd);
  if (b_okay) {
    return new PVD(&pvd);
  }
  return (PVD *) NULL;
}

/*!
  Read the Super block of an ISO 9660 image but determine framesize
  and datastart and a possible additional offset. Generally here we are
  not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
  filesystem.
  
  @see read_superblock
*/
bool
ISO9660::IFS::read_superblock (iso_extension_mask_t iso_extension_mask,
			       uint16_t i_fuzz)
{
  return iso9660_ifs_read_superblock (p_iso9660, iso_extension_mask);
}

/*!
  Read the Super block of an ISO 9660 image but determine framesize
  and datastart and a possible additional offset. Generally here we are
  not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
  filesystem.
  
  @see read_superblock
*/
bool 
ISO9660::IFS::read_superblock_fuzzy (iso_extension_mask_t iso_extension_mask,
				     uint16_t i_fuzz)
{
  return iso9660_ifs_fuzzy_read_superblock (p_iso9660, iso_extension_mask, 
					    i_fuzz);
}

/*! Read psz_path (a directory) and return a list of iso9660_stat_t
  pointers for the files inside that directory. The caller must free
  the returned result.
*/
bool
ISO9660::IFS::readdir (const char psz_path[], 
		       stat_vector_t& stat_vector) 
{
  CdioList_t *p_stat_list = iso9660_ifs_readdir (p_iso9660, psz_path);
  
  if (p_stat_list) {
    CdioListNode_t *p_entnode;
    _CDIO_LIST_FOREACH (p_entnode, p_stat_list) {
      iso9660_stat_t *p_statbuf = 
	(iso9660_stat_t *) _cdio_list_node_data (p_entnode);
      stat_vector.push_back(new ISO9660::Stat(p_statbuf));
    }
    _cdio_list_free (p_stat_list, false);
    return true;
  } else {
    return false;
  }
}

/*!
  Seek to a position and then read n bytes. Size read is returned.
*/
long int 
ISO9660::IFS::seek_read (void *ptr, lsn_t start, long int i_size) 
{
  return iso9660_iso_seek_read (p_iso9660, ptr, start, i_size);
}

/*!  
  Return file status for pathname. NULL is returned on error.
*/
ISO9660::Stat *
ISO9660::IFS::stat (const char psz_path[], bool b_translate) 
{
  if (b_translate) 
    return new Stat(iso9660_ifs_stat_translate (p_iso9660, psz_path));
  else 
    return new Stat(iso9660_ifs_stat (p_iso9660, psz_path));
}

char * 
ISO9660::PVD::get_application_id() 
{
  return iso9660_get_application_id(&pvd);
}

int 
ISO9660::PVD::get_pvd_block_size() 
{
  return iso9660_get_pvd_block_size(&pvd);
}

/*!
  Return the PVD's preparer ID.
  NULL is returned if there is some problem in getting this. 
*/
char * 
ISO9660::PVD::get_preparer_id() 
{
  return iso9660_get_preparer_id(&pvd);
}

/*!
  Return the PVD's publisher ID.
  NULL is returned if there is some problem in getting this. 
*/
char * 
ISO9660::PVD::get_publisher_id() 
{
  return iso9660_get_publisher_id(&pvd);
}

const char *
ISO9660::PVD::get_pvd_id() 
{
  return iso9660_get_pvd_id(&pvd);
}

int 
ISO9660::PVD::get_pvd_space_size() 
{
  return iso9660_get_pvd_space_size(&pvd);
}

uint8_t 
ISO9660::PVD::get_pvd_type() {
  return iso9660_get_pvd_type(&pvd);
}

/*! Return the primary volume id version number (of pvd).
  If there is an error 0 is returned. 
*/
int 
ISO9660::PVD::get_pvd_version() 
{
  return iso9660_get_pvd_version(&pvd);
}

/*! Return the LSN of the root directory for pvd.
  If there is an error CDIO_INVALID_LSN is returned. 
*/
lsn_t 
ISO9660::PVD::get_root_lsn() 
{
  return iso9660_get_root_lsn(&pvd);
}

/*!
  Return the PVD's system ID.
  NULL is returned if there is some problem in getting this. 
*/
char * 
ISO9660::PVD::get_system_id() 
{
  return iso9660_get_system_id(&pvd);
}

/*!
  Return the PVD's volume ID.
  NULL is returned if there is some problem in getting this. 
*/
char * 
ISO9660::PVD::get_volume_id() 
{
  return iso9660_get_volume_id(&pvd);
}

/*!
  Return the PVD's volumeset ID.
  NULL is returned if there is some problem in getting this. 
*/
char * 
ISO9660::PVD::get_volumeset_id() 
{
  return iso9660_get_volumeset_id(&pvd);
}
