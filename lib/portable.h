/*
    $Id: portable.h,v 1.2 2004/10/31 14:55:35 rocky Exp $

    Copyright (C) Rocky Bernstein <rocky@panix.com>

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

/* 
   This file contains definitions to fill in for differences or
   deficiencies to OS or compiler irregularities.  If this file is
   included other routines can be more portable.
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if !defined(HAVE_FTRUNCATE)
#if defined ( WIN32 )
#define ftruncate chsize
#endif
#endif /*HAVE_FTRUNCATE*/

#if !defined(HAVE_SNPRINTF)
#if defined ( MSVC )
#define snprintf _snprintf
#endif
#endif /*HAVE_SNPRINTF*/

#if !defined(HAVE_VSNPRINTF)
#if defined ( MSVC )
#define snprintf _vsnprintf
#endif
#endif /*HAVE_SNPRINTF*/

