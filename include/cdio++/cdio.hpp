/* -*- C++ -*-
    $Id: cdio.hpp,v 1.2 2005/11/11 12:26:57 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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

/** \file cdio.hpp
 *
 *  \brief C++ class for libcdio: the CD Input and Control
 *  library. Applications use this for anything regarding libcdio.
 */

#ifndef __CDIO_HPP__
#define __CDIO_HPP__

#include <cdio/cdio.h>

// Make pre- and post-increment operators for enums in libcdio where it 
// makes sense.
#include <cdio++/enum.hpp>

class Cdio {

public:

  // Other member functions
#include "devices.hpp"
};

/** A class relating to tracks. A track object basically saves device
    and track number information so that in track operations these
    don't have be specified. Note use invalid track number 0 to specify
    CD-Text for the CD (as opposed to a specific track).
*/
class CdioCDText
{
public: 
  CdioCDText(cdtext_t *p)
  { 
    p_cdtext = p;
    cdtext_init(p); // make sure we're initialized on the C side
  }

  ~CdioCDText() 
  {
    cdtext_destroy(p_cdtext);
    p_cdtext = (cdtext_t *) NULL;
  }

  // Other member functions
#include "cdtext.hpp"

private:
  cdtext_t *p_cdtext;
};
    
/** A class relating to tracks. A track object basically saves device
    and track number information so that in track operations these
    don't have be specified.
*/
class CdioTrack
{

public: 
  CdioTrack(CdIo_t *p, track_t t)
  { 
    i_track = t;
    p_cdio = p;
  }

  // Other member functions
#include "track.hpp"

private:
  track_t i_track;
  CdIo_t *p_cdio;
};
    
/** A class relating to a CD-ROM device or pseudo CD-ROM device with
    has a particular CD image. A device basically saves the libcdio
    "object" (of type CdIo *). 
*/
class CdioDevice 
{

public:
  CdioDevice(CdIo_t *p = (CdIo_t *) NULL)
  { 
    p_cdio=p; 
  };

  ~CdioDevice() 
  { 
    cdio_destroy(p_cdio); 
    p_cdio = (CdIo_t *) NULL;
  };

  // Other member functions  
#include "device.hpp"
#include "disc.hpp"
#include "read.hpp"

private:
  CdIo_t *p_cdio;
};

#endif /* __CDIO_HPP__ */
