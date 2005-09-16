#   $Id: Makefile.am,v 1.4 2005/09/16 09:31:39 rocky Exp $
#
#    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
####################################################
# Things to regression testing
####################################################
#
noinst_PROGRAMS = device iso1 iso2 iso3 mmc1 mmc2 paranoia paranoia2

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

device_SOURCES    = device.cpp
device_LDADD      = $(LIBCDIO_LIBS)

if BUILD_CD_PARANOIA
paranoia_SOURCES  = paranoia.cpp
paranoia_LDADD    = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) \
	            $(LIBCDIO_LIBS)
paranoia2_SOURCES = paranoia.cpp
paranoia2_LDADD   = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) \
	            $(LIBCDIO_LIBS)
endif

iso1_SOURCES      = iso1.cpp
iso1_LDADD        = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso2_SOURCES      = iso2.cpp
iso2_LDADD        = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso3_SOURCES      = iso3.cpp
iso3_LDADD        = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

mmc1_SOURCES      = mmc1.cpp
mmc1_LDADD        = $(LIBCDIO_LIBS)

mmc2_SOURCES      = mmc2.cpp
mmc2_LDADD        = $(LIBCDIO_LIBS)

# iso programs create file "copying"
MOSTLYCLEANFILES = copying
