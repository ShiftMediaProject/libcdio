#   $Id: Makefile.am,v 1.23 2005/04/30 09:42:37 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
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
# Sample programs
####################################################
#
if !DISABLE_CPP
SUBDIRS =  C++
endif
noinst_PROGRAMS = audio cdtext device drives iso1 iso2 iso3 isofuzzy \
	          mmc1 mmc2 paranoia paranoia2 tracks \
	          sample3 sample4 

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

audio_LDADD  = $(LIBCDIO_LIBS)

cdtext_LDADD  = $(LIBCDIO_LIBS)

device_LDADD = $(LIBCDIO_LIBS)

drives_LDADD = $(LIBCDIO_LIBS)

paranoia_LDADD  = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)
paranoia2_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)

iso1_LDADD       = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso2_LDADD       = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
iso3_LDADD       = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isofuzzy_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

mmc1_LDADD = $(LIBCDIO_LIBS)
mmc2_LDADD = $(LIBCDIO_LIBS)

sample3_LDADD = $(LIBCDIO_LIBS)
sample4_LDADD = $(LIBCDIO_LIBS)

tracks_LDADD  = $(LIBCDIO_LIBS)

# iso programs create file "copying"
MOSTLYCLEANFILES = copying
