#   $Id: Makefile.am,v 1.8 2004/10/09 03:20:28 rocky Exp $
#
#    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
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
noinst_PROGRAMS = sample1 sample2 sample3 sample4 sample5 sample6 \
	          sample7 sample8 sample9 sample10 iso1

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

iso1_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
sample1_LDADD = $(LIBCDIO_LIBS)
sample2_LDADD = $(LIBCDIO_LIBS)
sample3_LDADD = $(LIBCDIO_LIBS)
sample4_LDADD = $(LIBCDIO_LIBS)
sample5_LDADD = $(LIBCDIO_LIBS)
sample6_LDADD = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
sample7_LDADD = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS)
sample8_LDADD = $(LIBCDIO_LIBS)
sample9_LDADD = $(LIBCDIO_LIBS)
sample10_LDADD = $(LIBCDIO_LIBS)
