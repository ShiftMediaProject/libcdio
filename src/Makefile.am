#   $Id: Makefile.am,v 1.23 2004/05/27 02:46:17 rocky Exp $
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
man_MANS = cd-drive.1 cd-info.1 cd-read.1 iso-read.1 iso-info.1 
$(man_MANS): %.1: %
	-$(HELP2MAN) --output=$@ ./$<
endif

if BUILD_CDINFO
cd_info_SOURCES = cd-info.c util.c util.h
cd_info_LDADD =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(CDDB_LIBS) $(VCDINFO_LIBS)

cd_drive_SOURCES = cd-drive.c util.c util.h
cd_drive_LDADD =  $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)

cd_read_SOURCES = cd-read.c util.c util.h
cd_read_LDADD =  $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)

iso_info_SOURCES = iso-info.c util.c util.h
iso_info_LDADD =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)

iso_read_SOURCES = iso-read.c util.c util.h
iso_read_LDADD =  $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)

if BUILD_CDINFO_LINUX
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)
bin_PROGRAMS = cd-drive cd-info cd-read cdinfo-linux iso-info iso-read
else
bin_PROGRAMS = cd-drive cd-info cd-read iso-info iso-read
EXTRA_DIST = cdinfo-linux.c $(man_MANS)
endif
else 
EXTRA_DIST = cdinfo-linux.c cd-drive.c cd-info.c cd-read.c $(man_MANS)
man_MANS = 
endif

# Put LIBPOPT_CFLAGS after local include in case LIBPOPT has headers common
# to those in LIBCDIO_CFLAGS
INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS) $(VCDINFO_CFLAGS) $(LIBPOPT_CFLAGS) 
