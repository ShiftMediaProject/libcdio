#   $Id: Makefile.am,v 1.2 2003/03/24 23:59:22 rocky Exp $
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
# Things to make the libcdio library
####################################################
#
noinst_HEADERS = cdio_assert.h bytesex.h bytesex_asm.h \
	cdio_private.h ds.h 

include_HEADERS = cdio.h logging.h sector.h cdio_types.h util.h

libcdio_sources = \
	_cdio_bincue.c \
	_cdio_bsdi.c \
	_cdio_freebsd.c \
	_cdio_linux.c \
	_cdio_nrg.c \
	_cdio_stdio.c \
	_cdio_stdio.h \
	_cdio_stream.c \
	_cdio_stream.h \
	_cdio_sunos.c \
	bytesex.h \
	bytesex_asm.h \
	cdio.c \
	cdio.h \
	ds.c \
	ds.h \
	cdio_types.h \
	logging.c \
	logging.h \
	sector.c \
	sector.h \
	util.c \
	util.h 


lib_LTLIBRARIES     = libcdio.la 
libcdio_la_SOURCES = $(libcdio_sources)

###
# Install header files (default=$includedir/cdio)
#
install-includeHEADERS: $(include_HEADERS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(includedir)/cdio
	@list='$(include_HEADERS)'; for p in $$list; do \
	  if test -f "$$p"; then d= ; else d="$(srcdir)/"; fi; \
	  echo " $(INSTALL_DATA) $$d$$p $(DESTDIR)$(includedir)/cdio/"; \
	  $(INSTALL_DATA) $$d$$p $(DESTDIR)$(includedir)/cdio/; \
	done


###
# Remove them
#
uninstall-includeHEADERS:
	@$(NORMAL_UNINSTALL)
	list='$(include_HEADERS)'; for p in $$list; do \
	  rm -f $(DESTDIR)$(includedir)/cdio/$$p; \
	done

