#   $Id: Makefile.am,v 1.18 2003/11/18 03:35:19 rocky Exp $
#
#    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>
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
# Things to make the sample/test programs
####################################################
CDDB_LIBS=@CDDB_LIBS@

if MAINTAINER_MODE
MAINTAINERCLEANFILES = $(man_MANS)
man_MANS = cd-info.1 cd-read.1 
$(man_MANS): %.1: %
	-$(HELP2MAN) --output=$@ ./$<
endif

if BUILD_CDINFO
cd_info_SOURCES = cd-info.c util.c util.h
cd_info_LDADD =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(CDDB_LIBS) $(VCDINFO_LIBS)

cd_read_SOURCES = cd-read.c util.c util.h
cd_read_LDADD =  $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)

if BUILD_CDINFO_LINUX
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)
bin_PROGRAMS = cd-info cd-read cdinfo-linux
else
bin_PROGRAMS = cd-info cd-read
EXTRA_DIST = cdinfo-linux.c $(man_MANS)
endif
else 
EXTRA_DIST = cdinfo-linux.c cd-info.c cd-read.c $(man_MANS)
man_MANS = 
endif

INCLUDES = -I$(top_srcdir) $(LIBPOPT_CFLAGS) $(LIBCDIO_CFLAGS) $(VCDINFO_CFLAGS)
