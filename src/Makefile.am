#   $Id: Makefile.am,v 1.11 2003/08/16 12:59:03 rocky Exp $
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

if BUILD_CDINFO
cd_info_SOURCES = cd-info.c analyze.c analyze.h
cd_info_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(CDDB_LIBS) $(VCDINFO_LIBS)
cd_info_old_SOURCES = cd-info-old.c
cd_info_old_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(CDDB_LIBS) $(VCDINFO_LIBS)
if BUILD_CDINFO_LINUX
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)
bin_PROGRAMS = cd-info cdinfo-linux
else
bin_PROGRAMS = cd-info cd-info-old
EXTRA_DIST = cdinfo-linux.c
endif
else 
EXTRA_DIST = cdinfo-linux.c cd-info.c
endif

INCLUDES = -I$(top_srcdir) $(LIBPOPT_CFLAGS) $(LIBCDIO_CFLAGS) $(VCDINFO_CFLAGS)

