#   $Id: Makefile.am,v 1.3 2004/12/23 09:57:33 rocky Exp $
#
#    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
#    Copyright (C) 1998 Monty xiphmont@mit.edu
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
###############################################################################

if BUILD_CD_PARANOIA
cd_paranoia_SOURCES = cd-paranoia.c \
	buffering_write.c buffering_write.h \
	header.c header.h \
	report.c report.h

cd_paranoia_LDADD =  $(LIBCDIO_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_PARANOIA_LIBS)

bin_PROGRAMS = cd-paranoia

man_MANS = cd-paranoia.1 cd-paranoia.1.jp
EXTRA_DIST = $(man_MANS)

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)
endif

