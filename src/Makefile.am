#   $Id: Makefile.am,v 1.1 2003/03/24 19:01:09 rocky Exp $
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
if BUILD_CDINFO_LINUX
bin_PROGRAMS = cdinfo cdinfo-linux
cdinfo_linux_SOURCES = cdinfo-linux.c
cdinfo_linux_LDADD = $(LIBCDIO_LIBS) -lpopt
else
EXTRA_DIST = cdinfo-linux.c
bin_PROGRAMS = cdinfo
endif

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

cdinfo_SOURCES = cdinfo.c
cdinfo_LDADD = $(LIBCDIO_LIBS) -lpopt
