#   $Id: Makefile.am,v 1.36 2006/04/16 02:34:09 rocky Exp $
#
#    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
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
# Sample programs
####################################################
#
if ENABLE_CPP
SUBDIRS =  C++
endif
if BUILD_CD_PARANOIA
paranoia_progs = paranoia paranoia2
endif
noinst_PROGRAMS = audio cdchange cdtext device drives eject \
	          isofile isofile2 isofuzzy isolist \
	          mmc1 mmc2 mmc2a $(paranoia_progs) tracks \
	          sample3 sample4 udf1 udffile

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

audio_DEPENDENCIES    = $(LIBCDIO_DEPS)
audio_LDADD           = $(LIBCDIO_LIBS)

cdchange_DEPENDENCIES = $(LIBCDIO_DEPS)
cdchange_LDADD        = $(LIBCDIO_LIBS)

cdtext_DEPENDENCIES   = $(LIBCDIO_DEPS)
cdtext_LDADD          = $(LIBCDIO_LIBS)

device_DEPENDENCIES   = $(LIBCDIO_DEPS)
device_LDADD          = $(LIBCDIO_LIBS)

drives_DEPENDENCIES   = $(LIBCDIO_DEPS)
drives_LDADD          = $(LIBCDIO_LIBS)

eject_DEPENDENCIES    = $(LIBCDIO_DEPS)
eject_LDADD           = $(LIBCDIO_LIBS)

if BUILD_CD_PARANOIA
paranoia_LDADD  = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)
paranoia2_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS)
endif

isofile_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isofile2_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isofuzzy_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)
isolist_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LIBICONV)

mmc1_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc1_LDADD        = $(LIBCDIO_LIBS)
mmc2_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc2_LDADD        = $(LIBCDIO_LIBS)

mmc2a_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc2a_LDADD        = $(LIBCDIO_LIBS)

sample3_DEPENDENCIES = $(LIBCDIO_DEPS)
sample3_LDADD = $(LIBCDIO_LIBS)

sample4_DEPENDENCIES = $(LIBCDIO_DEPS)
sample4_LDADD = $(LIBCDIO_LIBS)

tracks_DEPENDENCIES = $(LIBCDIO_DEPS)
tracks_LDADD        = $(LIBCDIO_LIBS)

udf1_DEPENDENCIES = $(LIBUDF_LIBS) $(LIBCDIO_DEPS)
udf1_LDADD        = $(LIBUDF_LIBS) $(LIBCDIO_LIBS)

udffile_DEPENDENCIES = $(LIBUDF_LIBS) $(LIBCDIO_DEPS)
udffile_LDADD        = $(LIBUDF_LIBS) $(LIBCDIO_LIBS)


# iso programs create file "copying"
MOSTLYCLEANFILES = copying
