/* -*- C++ -*-
    $Id: iso9660.hpp,v 1.1 2006/03/05 06:52:15 rocky Exp $

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

/** \file iso966o.hpp
 *
 *  \brief C++ class for libcdio: the CD Input and Control
 *  library. Applications use this for anything regarding libcdio.
 */

#ifndef __ISO9660_HPP__
#define __ISO9660_HPP__

#include <cdio/iso9660.h>

/** ISO 9660 class.
*/
class ISO9660
{

public:

  class IFS
  {
    
    IFS()
    { 
      p_iso9660=NULL; 
    };

    ~IFS() 
    { 
      iso9660_close(p_iso9660); 
      p_iso9660 = (iso9660_t *) NULL;
    };
    
    /*! Close previously opened ISO 9660 image and free resources
      associated with the image. Call this when done using using an ISO
      9660 image.
      
      @return true is unconditionally returned. If there was an error
      false would be returned.
    */
    bool 
    close()
    {
      iso9660_close(p_iso9660);
      p_iso9660 = (iso9660_t *) NULL;
      return true;
    };
    
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
    open(const char *psz_path, 
	 iso_extension_mask_t iso_extension_mask=ISO_EXTENSION_NONE)
    {
      if (p_iso9660) iso9660_close(p_iso9660);
      p_iso9660 = iso9660_open_ext(psz_path, iso_extension_mask);
      return NULL != (iso9660_t *) p_iso9660 ;
    };
    
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
    open_fuzzy (const char *psz_path,
		iso_extension_mask_t iso_extension_mask=ISO_EXTENSION_NONE,
		uint16_t i_fuzz=20)
    {
      p_iso9660 = iso9660_open_fuzzy_ext(psz_path, iso_extension_mask, i_fuzz);
      //return p_iso9660 != (iso9660_t *) NULL;
      return true;
    };

    /*!
      Read the Super block of an ISO 9660 image but determine framesize
      and datastart and a possible additional offset. Generally here we are
      not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
      filesystem.
    */
    bool 
    fuzzy_read_superblock (iso_extension_mask_t iso_extension_mask=
			   ISO_EXTENSION_NONE,
			   uint16_t i_fuzz=20)
    {
      return iso9660_ifs_fuzzy_read_superblock (p_iso9660, iso_extension_mask, 
						i_fuzz);

    };
    
    /*!
      Seek to a position and then read n bytes. Size read is returned.
    */
    long int seek_read (void *ptr, lsn_t start, long int i_size=1) 
    {
      return iso9660_iso_seek_read (p_iso9660, ptr, start, i_size);
    };

    
  private:
    iso9660_t *p_iso9660;
  };
};


#endif /* __ISO9660_HPP__ */
