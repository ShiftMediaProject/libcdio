#   $Id: Makefile.am,v 1.2 2003/12/24 11:05:48 uid67423 Exp $
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
# Things to regression testing
####################################################
#
noinst_PROGRAMS = sample1 sample2 sample3 sample4 sample5 sample6

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

sample1_LDADD = $(LIBCDIO_LIBS)
sample2_LDADD = $(LIBCDIO_LIBS)
sample3_LDADD = $(LIBCDIO_LIBS)
sample4_LDADD = $(LIBCDIO_LIBS)
sample5_LDADD = $(LIBCDIO_LIBS)
sample6_LDADD = $(LIBCDIO_LIBS) $(LIBISO9660_LIBS)
