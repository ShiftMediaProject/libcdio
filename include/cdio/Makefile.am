#   $Id: Makefile.am,v 1.3 2003/04/21 14:13:54 hvr Exp $
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
########################################################
# Things to make the install (public) libcdio headers
########################################################
#

# generated part should be put into version.h.in and included from cdio.h
EXTRA_DIST = cdio.h.in

libcdioincludedir=$(includedir)/cdio
libcdioinclude_HEADERS = cdio.h logging.h sector.h types.h util.h

BUILT_SOURCES = cdio.h
