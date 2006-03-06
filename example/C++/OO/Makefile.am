#   $Id: Makefile.am,v 1.5 2006/03/06 04:48:38 rocky Exp $
#
#    Copyright (C) 2005, 2006 Rocky Bernstein <rocky@panix.com>
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
############################################################
# Sample C++ programs using libcdio++ (with C++ OO wrapper)
############################################################
#
noinst_PROGRAMS = cdtext device drives eject iso1 iso3 mmc1 mmc2 tracks

INCLUDES = -I$(top_srcdir)/include $(LIBCDIO_CFLAGS)

cdtext_SOURCES      = cdtext.cpp
cdtext_DEPENDENCIES = $(LIBCDIO_DEPS)
cdtext_LDADD        = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

device_SOURCES      = device.cpp
device_DEPENDENCIES = $(LIBCDIO_DEPS)
device_LDADD        = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

drives_SOURCES      = drives.cpp
drives_DEPENDENCIES = $(LIBCDIO_DEPS)
drives_LDADD        = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

eject_SOURCES       = eject.cpp
eject_DEPENDENCIES  = $(LIBCDIO_DEPS)
eject_LDADD         = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

iso1_SOURCES        = iso1.cpp
iso1_LDADD          = $(LIBISO966PP0_LIBS) $(LIBISO9660_LIBS) \
	              $(LIBCDIOPP_LIBS) $(LIBICONV)

iso3_SOURCES        = iso3.cpp
iso3_LDADD          = $(LIBISO966PP0_LIBS) $(LIBISO9660_LIBS) \
	              $(LIBCDIOPP_LIBS) $(LIBICONV)

mmc1_SOURCES        = mmc1.cpp
mmc1_DEPENDENCIES   = $(LIBCDIO_DEPS)
mmc1_LDADD          = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

mmc2_SOURCES        = mmc2.cpp
mmc2_DEPENDENCIES   = $(LIBCDIO_DEPS)
mmc2_LDADD          = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

tracks_SOURCES      = tracks.cpp
tracks_LDADD        = $(LIBCDIOPP_LIBS) $(LIBCDIO_LIBS)

