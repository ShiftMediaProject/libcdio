#   $Id: Makefile.am,v 1.7 2003/06/01 21:05:45 rocky Exp $
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
cd_info_SOURCES = cd-info.c
cd_info_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS) $(CDDB_LIBS) $(VCDINFO_LIB)
if BUILD_CDINFO_LINUX
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD = $(LIBCDIO_LIBS) $(LIBPOPT_LIBS)
bin_PROGRAMS = cd-info cdinfo-linux
else
bin_PROGRAMS = cd-info
EXTRA_DIST = cdinfo-linux.c
endif
else 
EXTRA_DIST = cdinfo-linux.c cd-info.c
endif

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

