#   $Id: Makefile.am,v 1.8 2008/03/21 10:42:36 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2008 Rocky Bernstein <rocky@gnu.org>
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
EXTRA_DIST = doxygen/Doxyfile.in doxygen/run_doxygen
info_TEXINFOS = libcdio.texi 
libcdio_TEXINFOS = fdl.texi glossary.texi

reference:
	-( cd ${top_srcdir} && $(MAKE) doxygen )

pdf: libcdio.pdf 

txt: libcdio.txt

ps: libcdio.ps

html: libcdio.html

%.ps.gz: %.ps
	gzip -9c $< > $@

.texi.pdf:
	texi2pdf $<

.texi.html:
	texi2html $<

.texi.txt:
	makeinfo --no-headers $< > $@

all-formats: pdf dvi txt ps html

MOSTLYCLEANFILES = libcdio.html libcdio.pdf libcdio.ps.gz
