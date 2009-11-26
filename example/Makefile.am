#   Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009
#   Rocky Bernstein <rocky@gnu.org>
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
if BUILD_EXAMPLES
noinst_PROGRAMS = audio cdchange cdtext device drives eject \
	          isofile isofile2 isofuzzy isolist isolsn \
	          mmc1 mmc2 mmc2a mmc3 $(paranoia_progs) tracks \
	          sample3 sample4 udf1 udffile cdio-eject
endif

INCLUDES = -I$(top_srcdir) $(LIBCDIO_CFLAGS)

audio_DEPENDENCIES    = $(LIBCDIO_DEPS)
audio_LDADD           = $(LIBCDIO_LIBS) $(LTLIBICONV)

cdchange_DEPENDENCIES = $(LIBCDIO_DEPS)
cdchange_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

cdtext_DEPENDENCIES   = $(LIBCDIO_DEPS)
cdtext_LDADD          = $(LIBCDIO_LIBS) $(LTLIBICONV)

device_DEPENDENCIES   = $(LIBCDIO_DEPS)
device_LDADD          = $(LIBCDIO_LIBS) $(LTLIBICONV)

drives_DEPENDENCIES   = $(LIBCDIO_DEPS)
drives_LDADD          = $(LIBCDIO_LIBS) $(LTLIBICONV)

eject_DEPENDENCIES    = $(LIBCDIO_DEPS)
eject_LDADD           = $(LIBCDIO_LIBS) $(LTLIBICONV)

cdio_eject_DEPENDENCIES = $(LIBCDIO_DEPS)
cdio_eject_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

if BUILD_CD_PARANOIA
paranoia_LDADD  = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)
paranoia2_LDADD = $(LIBCDIO_PARANOIA_LIBS) $(LIBCDIO_CDDA_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)
endif

isofile_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)
isofile2_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)
isofuzzy_LDADD   = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)
isolist_LDADD    = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)
isolsn_LDADD     = $(LIBISO9660_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)

mmc1_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc1_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)
mmc2_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc2_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

mmc2a_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc2a_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

mmc3_DEPENDENCIES = $(LIBCDIO_DEPS)
mmc3_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

sample3_DEPENDENCIES = $(LIBCDIO_DEPS)
sample3_LDADD = $(LIBCDIO_LIBS) $(LTLIBICONV)

sample4_DEPENDENCIES = $(LIBCDIO_DEPS)
sample4_LDADD = $(LIBCDIO_LIBS) $(LTLIBICONV)

tracks_DEPENDENCIES = $(LIBCDIO_DEPS)
tracks_LDADD        = $(LIBCDIO_LIBS) $(LTLIBICONV)

udf1_DEPENDENCIES = $(LIBUDF_LIBS) $(LIBCDIO_DEPS)
udf1_LDADD        = $(LIBUDF_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)

udffile_DEPENDENCIES = $(LIBUDF_LIBS) $(LIBCDIO_DEPS)
udffile_LDADD        = $(LIBUDF_LIBS) $(LIBCDIO_LIBS) $(LTLIBICONV)

check_PROGRAMS      = cdtext device drives \
                      mmc1 mmc2 mmc2a mmc3 $(paranoia_progs) sample4

TESTS               = $(check_PROGRAMS)

# iso programs create file "copying"
MOSTLYCLEANFILES = copying *.wav
