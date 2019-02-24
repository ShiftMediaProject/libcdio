/*
  Copyright (C) 2008, 2010-2012, 2017, 2018 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 2014 Robert Kausch <robert.kausch@freac.org>

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Changes up to version 0.76 */
/*
 * Copyright (c) 2003
 *      Matthias Drochner.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * NetBSD and OpenBSD support for libcdio.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#endif

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "cdio_private.h"

#ifndef USE_MMC_SUBCHANNEL
#define USE_MMC_SUBCHANNEL 0
#endif

#if defined(__NetBSD__) && (defined(__i386__) || defined(__amd64__))
#define DEFAULT_CDIO_DEVICE "/dev/rcd0d"
#else
#define DEFAULT_CDIO_DEVICE "/dev/rcd0c"
#endif

#define MAX_CD_DEVICES 64

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_NETBSD_CDROM
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>
#include <sys/scsiio.h>
#include <sys/sysctl.h>
#include <sys/disklabel.h>

#define TOTAL_TRACKS (_obj->tochdr.ending_track \
                      - _obj->tochdr.starting_track + 1)
#define FIRST_TRACK_NUM (_obj->tochdr.starting_track)

typedef enum {
  _AM_NONE,
  _AM_IOCTL,
  _AM_READ_CD,
  _AM_MMC_RDWR,
  _AM_MMC_RDWR_EXCL,
} access_mode_t;

typedef struct {
  /* Things common to all drivers like this.
     This must be first. */
  generic_img_private_t gen;

  access_mode_t access_mode;

  bool toc_valid;
  struct ioc_toc_header tochdr;
  struct cd_toc_entry tocent[100];

  bool sessionformat_valid;
  int sessionformat[100]; /* format of the session the track is in */
} _img_private_t;

static driver_return_code_t
run_scsi_cmd_netbsd(void *p_user_data, unsigned int i_timeout_ms,
                    unsigned int i_cdb, const mmc_cdb_t *p_cdb,
                    cdio_mmc_direction_t e_direction,
                    unsigned int i_buf, void *p_buf )
{
        const _img_private_t *_obj = p_user_data;
        scsireq_t req;

        memset(&req, 0, sizeof(req));
        memcpy(&req.cmd[0], p_cdb, i_cdb);
        req.cmdlen = i_cdb;
        req.datalen = i_buf;
        req.databuf = p_buf;
        req.timeout = i_timeout_ms;
        req.flags = e_direction == SCSI_MMC_DATA_READ ? SCCMD_READ : SCCMD_WRITE;

        if (ioctl(_obj->gen.fd, SCIOCCOMMAND, &req) < 0) {
                cdio_info("SCIOCCOMMAND: %s", strerror(errno));
                return -1;
        }
        if (req.retsts != SCCMD_OK) {
                cdio_info("SCIOCCOMMAND cmd 0x%02x sts %d\n", req.cmd[0], req.retsts);
                return -1;
        }

        return 0;
}

static access_mode_t
str_to_access_mode_netbsd(const char *psz_access_mode)
{
  const access_mode_t default_access_mode = _AM_IOCTL;

  if (NULL==psz_access_mode) return default_access_mode;

  if (!strcmp(psz_access_mode, "IOCTL"))
    return _AM_IOCTL;
  else if (!strcmp(psz_access_mode, "READ_CD"))
    return _AM_READ_CD;
  else if (!strcmp(psz_access_mode, "MMC_RDWR"))
    return _AM_MMC_RDWR;
  else if (!strcmp(psz_access_mode, "MMC_RDWR_EXCL"))
    return _AM_MMC_RDWR_EXCL;
  else {
    cdio_warn ("unknown access type: %s. Default IOCTL used.",
               psz_access_mode);
    return default_access_mode;
  }
}

static int
read_audio_sectors_netbsd(void *user_data, void *data, lsn_t lsn,
                          unsigned int nblocks)
{
        scsireq_t req;
        _img_private_t *_obj = user_data;

        memset(&req, 0, sizeof(req));
        req.cmd[0] = 0xbe;
        req.cmd[1] = 0;
        req.cmd[2] = (lsn >> 24) & 0xff;
        req.cmd[3] = (lsn >> 16) & 0xff;
        req.cmd[4] = (lsn >> 8) & 0xff;
        req.cmd[5] = (lsn >> 0) & 0xff;
        req.cmd[6] = (nblocks >> 16) & 0xff;
        req.cmd[7] = (nblocks >> 8) & 0xff;
        req.cmd[8] = (nblocks >> 0) & 0xff;
        req.cmd[9] = 0x78;
        req.cmdlen = 10;

        req.datalen = nblocks * CDIO_CD_FRAMESIZE_RAW;
        req.databuf = data;
        req.timeout = 10000;
        req.flags = SCCMD_READ;

        if (ioctl(_obj->gen.fd, SCIOCCOMMAND, &req) < 0) {
                cdio_info("SCIOCCOMMAND: %s", strerror(errno));
                return 1;
        }
        if (req.retsts != SCCMD_OK) {
                cdio_info("SCIOCCOMMAND cmd 0xbe sts %d\n", req.retsts);
                return 1;
        }

        return 0;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error.
 */
static driver_return_code_t
_read_mode1_sector_netbsd (void *p_user_data, void *p_data, lsn_t lsn,
                           bool b_form2)
{

  return cdio_generic_read_form1_sector(p_user_data, p_data, lsn);
}

/*!
   Reads i_blocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error.
 */
static driver_return_code_t
_read_mode1_sectors_netbsd (void *p_user_data, void *p_data, lsn_t lsn,
                            bool b_form2, uint32_t i_blocks)
{
  _img_private_t *p_env = p_user_data;
  unsigned int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < i_blocks; i++) {
    if ( (retval = _read_mode1_sector_netbsd (p_env,
                                              ((char *)p_data) + (blocksize*i),
                                              lsn + i, b_form2)) )
      return retval;
  }
  return DRIVER_OP_SUCCESS;
}

static int
read_mode2_sector_netbsd(void *user_data, void *data, lsn_t lsn,
                         bool mode2_form2)
{
        scsireq_t req;
        _img_private_t *_obj = user_data;
        char buf[M2RAW_SECTOR_SIZE] = { 0, };

        memset(&req, 0, sizeof(req));
        req.cmd[0] = 0xbe;
        req.cmd[1] = 0;
        req.cmd[2] = (lsn >> 24) & 0xff;
        req.cmd[3] = (lsn >> 16) & 0xff;
        req.cmd[4] = (lsn >> 8) & 0xff;
        req.cmd[5] = (lsn >> 0) & 0xff;
        req.cmd[6] = 0;
        req.cmd[7] = 0;
        req.cmd[8] = 1;
        req.cmd[9] = 0x58; /* subheader + userdata + ECC */
        req.cmdlen = 10;

        req.datalen = M2RAW_SECTOR_SIZE;
        req.databuf = buf;
        req.timeout = 10000;
        req.flags = SCCMD_READ;

        if (ioctl(_obj->gen.fd, SCIOCCOMMAND, &req) < 0) {
                cdio_info("SCIOCCOMMAND: %s", strerror(errno));
                return 1;
        }
        if (req.retsts != SCCMD_OK) {
                cdio_info("SCIOCCOMMAND cmd 0xbe sts %d\n", req.retsts);
                return 1;
        }

        if (mode2_form2)
                memcpy(data, buf, M2RAW_SECTOR_SIZE);
        else
                memcpy(data, buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);

        return 0;
}

static int
read_mode2_sectors_netbsd(void *user_data, void *data, lsn_t lsn,
                          bool mode2_form2, unsigned int nblocks)
{
        int i, res;
        char *buf = data;

        for (i = 0; i < nblocks; i++) {
                res = read_mode2_sector_netbsd(user_data, buf, lsn, mode2_form2);
                if (res)
                        return res;

                buf += (mode2_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE);
                lsn++;
        }

        return 0;
}

static int
set_arg_netbsd(void *p_user_data, const char key[], const char value[])
{
  _img_private_t *p_env = p_user_data;

  if (!strcmp (key, "source"))
    {
      if (!value) return DRIVER_OP_ERROR;
      free (p_env->gen.source_name);
      p_env->gen.source_name = strdup (value);
    }
  else if (!strcmp (key, "access-mode"))
    {
      p_env->access_mode = str_to_access_mode_netbsd(key);
    }
  else return DRIVER_OP_ERROR;

  return DRIVER_OP_SUCCESS;
}

static bool
_cdio_read_toc(_img_private_t *_obj)
{
        int res;
        struct ioc_read_toc_entry req;

        res = ioctl(_obj->gen.fd, CDIOREADTOCHEADER, &_obj->tochdr);
        if (res < 0) {
                cdio_warn("error in ioctl(CDIOREADTOCHEADER): %s\n",
                           strerror(errno));
                return false;
        }

        req.address_format = CD_MSF_FORMAT;
        req.starting_track = FIRST_TRACK_NUM;
        req.data_len = (TOTAL_TRACKS + 1) /* leadout! */
                * sizeof(struct cd_toc_entry);
        req.data = _obj->tocent;

        res = ioctl(_obj->gen.fd, CDIOREADTOCENTRIES, &req);
        if (res < 0) {
                cdio_warn("error in ioctl(CDROMREADTOCENTRIES): %s\n",
                           strerror(errno));
                return false;
        }

        _obj->toc_valid = 1;
        _obj->gen.i_first_track = FIRST_TRACK_NUM;
        _obj->gen.i_tracks = TOTAL_TRACKS;
        _obj->gen.toc_init = true;
        return true;
}

static bool
read_toc_netbsd (void *p_user_data)
{

        return _cdio_read_toc(p_user_data);
}

static int
_cdio_read_discinfo(_img_private_t *_obj)
{
        scsireq_t req;
#define FULLTOCBUF (4 + 1000*11)
        unsigned char buf[FULLTOCBUF] = { 0, };
        int i, j;

        memset(&req, 0, sizeof(req));
        req.cmd[0] = 0x43; /* READ TOC/PMA/ATIP */
        req.cmd[1] = 0x02;
        req.cmd[2] = 0x02; /* full TOC */
        req.cmd[3] = 0;
        req.cmd[4] = 0;
        req.cmd[5] = 0;
        req.cmd[6] = 0;
        req.cmd[7] = FULLTOCBUF / 256;
        req.cmd[8] = FULLTOCBUF % 256;
        req.cmd[9] = 0;
        req.cmdlen = 10;

        req.datalen = FULLTOCBUF;
        req.databuf = buf;
        req.timeout = 10000;
        req.flags = SCCMD_READ;

        if (ioctl(_obj->gen.fd, SCIOCCOMMAND, &req) < 0) {
                cdio_info("SCIOCCOMMAND: %s", strerror(errno));
                return 1;
        }
        if (req.retsts != SCCMD_OK) {
                cdio_info("SCIOCCOMMAND cmd 0x43 sts %d\n", req.retsts);
                return 1;
        }
#if 1
        printf("discinfo:");
        for (i = 0; i < 4; i++)
                printf(" %02x", buf[i]);
        printf("\n");
        for (i = 0; i < buf[1] - 2; i++) {
                printf(" %02x", buf[i + 4]);
                if (!((i + 1) % 11))
                        printf("\n");
        }
#endif

        for (i = 4; i < req.datalen_used; i += 11) {
                if (buf[i + 3] == 0xa0) { /* POINT */
                        /* XXX: assume entry 0xa1 follows */
                        for (j = buf[i + 8] - 1; j <= buf[i + 11 + 8] - 1; j++)
                                _obj->sessionformat[j] = buf[i + 9];
                }
        }

        _obj->sessionformat_valid = true;
        return 0;
}

static int
eject_media_netbsd(void *user_data) {

        _img_private_t *_obj = user_data;
        int fd, res, ret = 0;

        fd = open(_obj->gen.source_name, O_RDONLY|O_NONBLOCK);
        if (fd < 0)
                return 2;

        res = ioctl(fd, CDIOCALLOW);
        if (res < 0) {
                cdio_warn("ioctl(fd, CDIOCALLOW) failed: %s\n",
                           strerror(errno));
                /* go on... */
        }
        res = ioctl(fd, CDIOCEJECT);
        if (res < 0) {
                cdio_warn("ioctl(CDIOCEJECT) failed: %s\n",
                           strerror(errno));
                ret = 1;
        }

        close(fd);
        return ret;
}

static bool
is_mmc_supported(void *user_data)
{
    _img_private_t *env = user_data;
    return (_AM_NONE == env->access_mode) ? false : true;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
get_arg_netbsd(void *env, const char key[])
{
  _img_private_t *_obj = env;

  if (!strcmp(key, "source")) {
    return _obj->gen.source_name;
  } else if (!strcmp(key, "access-mode")) {
    switch (_obj->access_mode) {
    case _AM_IOCTL:
      return "IOCTL";
    case _AM_READ_CD:
      return "READ_CD";
    case _AM_MMC_RDWR:
      return "MMC_RDWR";
    case _AM_MMC_RDWR_EXCL:
      return "MMC_RDWR_EXCL";
    case _AM_NONE:
      return "no access method";
    }
  } else if (!strcmp (key, "mmc-supported?")) {
      return is_mmc_supported(env) ? "true" : "false";
  }
  return NULL;
}

static track_t
get_first_track_num_netbsd(void *user_data)
{
        _img_private_t *_obj = user_data;
        int res;

        if (!_obj->toc_valid) {
                res = _cdio_read_toc(_obj);
                if (!res)
                        return CDIO_INVALID_TRACK;
        }

        return FIRST_TRACK_NUM;
}

static track_t
get_num_tracks_netbsd(void *user_data)
{
        _img_private_t *_obj = user_data;
        int res;

        if (!_obj->toc_valid) {
                res = _cdio_read_toc(_obj);
                if (!res)
                        return CDIO_INVALID_TRACK;
        }

        return TOTAL_TRACKS;
}

/*!
  Return the international standard recording code ISRC.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static char *
get_track_isrc_netbsd (const void *p_user_data, track_t i_track) {

  const _img_private_t *p_env = p_user_data;
  return mmc_get_track_isrc( p_env->gen.cdio, i_track );
}

static driver_return_code_t
audio_get_volume_netbsd(void *p_user_data, cdio_audio_volume_t *p_volume)
{
  const _img_private_t *p_env = p_user_data;
  return (ioctl(p_env->gen.fd, CDIOCGETVOL, p_volume));
}

static driver_return_code_t
audio_pause_netbsd(void *p_user_data)
{
  const _img_private_t *p_env = p_user_data;
  return (ioctl(p_env->gen.fd, CDIOCPAUSE));
}

static driver_return_code_t
audio_stop_netbsd(void *p_user_data)
{
  const _img_private_t *p_env = p_user_data;
  return (ioctl(p_env->gen.fd, CDIOCSTOP));
}

static driver_return_code_t
audio_resume_netbsd(void *p_user_data)
{
  const _img_private_t *p_env = p_user_data;
  return (ioctl(p_env->gen.fd, CDIOCRESUME));
}

static driver_return_code_t
audio_set_volume_netbsd(void *p_user_data, cdio_audio_volume_t *p_volume)
{
  const _img_private_t *p_env = p_user_data;
  return (ioctl(p_env->gen.fd, CDIOCSETVOL, p_volume));
}

/*!
  Get format of track.
*/
static track_format_t
get_track_format_netbsd(void *user_data, track_t track_num)
{
        _img_private_t *_obj = user_data;
        int res, first_track = 0, track_idx = 0;

        if (!_obj->toc_valid) {
                res = _cdio_read_toc(_obj);
                if (!res)
                        return TRACK_FORMAT_ERROR;
        }

        first_track = _obj->gen.i_first_track;

        if (!_obj->gen.toc_init ||
            track_num > (first_track + _obj->gen.i_tracks) ||
            track_num < first_track)
            return (CDIO_INVALID_TRACK);

        track_idx = track_num - first_track;

        if (_obj->tocent[track_idx].control & 0x04) {
                if (!_obj->sessionformat_valid) {
                        res = _cdio_read_discinfo(_obj);
                        if (res)
                                return TRACK_FORMAT_ERROR;
                }

                if (_obj->sessionformat[track_idx] == 0x10)
                        return TRACK_FORMAT_CDI;
                else if (_obj->sessionformat[track_idx] == 0x20)
                        return TRACK_FORMAT_XA;
                else
                        return TRACK_FORMAT_DATA;
        } else
                return TRACK_FORMAT_AUDIO;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8

  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
get_track_green_netbsd(void *user_data, track_t track_num)
{

        return (get_track_format_netbsd(user_data, track_num)
                == TRACK_FORMAT_XA);
}

/*!
  Return the starting MSF (minutes/secs/frames) for track number
  track_num in obj.  Track numbers usually start at something
  greater than 0, usually 1.

  The "leadout" track is specified by passing i_track as either
  LEADOUT_TRACK or the track number of the last audio track plus one.

  False is returned if there is no track entry.
*/
static bool
get_track_msf_netbsd(void *user_data, track_t track_num, msf_t *msf)
{
        _img_private_t *_obj = user_data;
        int res, first_track = 0, track_idx = 0;

        if (!msf)
                return false;

        if (!_obj->toc_valid) {
                res = _cdio_read_toc(_obj);
                if (!res)
                        return false;
        }

        if (track_num == CDIO_CDROM_LEADOUT_TRACK)
                track_num = _obj->gen.i_tracks + _obj->gen.i_first_track;

        first_track = _obj->gen.i_first_track;

        if (!_obj->gen.toc_init ||
            track_num > (first_track + _obj->gen.i_tracks) ||
            track_num < first_track)
            return (CDIO_INVALID_TRACK);

        track_idx = track_num - first_track;
        msf->m = cdio_to_bcd8(_obj->tocent[track_idx].addr.msf.minute);
        msf->s = cdio_to_bcd8(_obj->tocent[track_idx].addr.msf.second);
        msf->f = cdio_to_bcd8(_obj->tocent[track_idx].addr.msf.frame);

        return true;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
   @return the lsn. On error return CDIO_INVALID_LSN.

   Also note that in one at least one test the corresponding MMC gives
   a different answer, so there may be some disagreement about what is in
   fact the last lsn.
 */
static lsn_t
get_disc_last_lsn_netbsd(void *user_data)
{
        msf_t msf;

        get_track_msf_netbsd(user_data, CDIO_CDROM_LEADOUT_TRACK, &msf);

        return (((msf.m * 60) + msf.s) * CDIO_CD_FRAMES_PER_SEC + msf.f);
}


static driver_return_code_t
get_last_session_netbsd(void *p_user_data, lsn_t *i_last_session)
{
  const _img_private_t *p_env = p_user_data;
  int addr = 0;

  if (ioctl(p_env->gen.fd, CDIOREADMSADDR, &addr) == 0) {
    *i_last_session = addr;
    return (DRIVER_OP_SUCCESS);
  } else {
    cdio_warn("ioctl CDIOREADMSADDR failed: %s\n",
        strerror(errno));
    return (DRIVER_OP_ERROR);
  }
}
#endif /* HAVE_NETBSD_CDROM */

char **
cdio_get_devices_netbsd (void)
{
#ifndef HAVE_NETBSD_CDROM
  return NULL;
#else
  char drive[16];
  char **drives = NULL;
  unsigned int num_drives = 0;
  int cdfd;
  int n;

  /* Search for open(2)able /dev/rcd* devices */
  for (n = 0; n <= MAX_CD_DEVICES; n++) {
    snprintf(drive, sizeof(drive), "/dev/rcd%d%c", n, 'a' + RAW_PART);
    if (!cdio_is_device_quiet_generic(drive))
      continue;
    if ((cdfd = open(drive, O_RDONLY|O_NONBLOCK, 0)) == -1)
      continue;
    close(cdfd);
    cdio_add_device_list(&drives, drive, &num_drives);
  }
  cdio_add_device_list(&drives, NULL, &num_drives);
  return (drives);
#endif /* HAVE_NETBSD_CDROM */
}

#ifdef HAVE_NETBSD_CDROM
static driver_return_code_t
audio_play_msf_netbsd(void *p_user_data, msf_t *p_start_msf, msf_t *p_end_msf)
{
  const _img_private_t *p_env = p_user_data;
  struct ioc_play_msf a;

  a.start_m = cdio_from_bcd8(p_start_msf->m);
  a.start_s = cdio_from_bcd8(p_start_msf->s);
  a.start_f = cdio_from_bcd8(p_start_msf->f);
  a.end_m = cdio_from_bcd8(p_end_msf->m);
  a.end_s = cdio_from_bcd8(p_end_msf->s);
  a.end_f = cdio_from_bcd8(p_end_msf->f);

  return (ioctl(p_env->gen.fd, CDIOCPLAYMSF, (char *)&a));
}

#if !USE_MMC_SUBCHANNEL
static driver_return_code_t
audio_read_subchannel_netbsd(void *p_user_data, cdio_subchannel_t *subchannel)
{
  const _img_private_t *p_env = p_user_data;
  struct ioc_read_subchannel s;
  struct cd_sub_channel_info data;

  bzero(&s, sizeof(s));
  s.data = &data;
  s.data_len = sizeof(data);
  s.address_format = CD_MSF_FORMAT;
  s.data_format = CD_CURRENT_POSITION;

  if (ioctl(p_env->gen.fd, CDIOCREADSUBCHANNEL, &s) != -1) {
    subchannel->control = s.data->what.position.control;
    subchannel->track = s.data->what.position.track_number;
    subchannel->index = s.data->what.position.index_number;

    subchannel->abs_addr.m =
        cdio_to_bcd8(s.data->what.position.absaddr.msf.minute);
    subchannel->abs_addr.s =
        cdio_to_bcd8(s.data->what.position.absaddr.msf.second);
    subchannel->abs_addr.f =
        cdio_to_bcd8(s.data->what.position.absaddr.msf.frame);
    subchannel->rel_addr.m =
        cdio_to_bcd8(s.data->what.position.reladdr.msf.minute);
    subchannel->rel_addr.s =
        cdio_to_bcd8(s.data->what.position.reladdr.msf.second);
    subchannel->rel_addr.f =
        cdio_to_bcd8(s.data->what.position.reladdr.msf.frame);
    subchannel->audio_status = s.data->header.audio_status;

    return (DRIVER_OP_SUCCESS);
  } else {
    cdio_warn("ioctl CDIOCREADSUBCHANNEL failed: %s\n",
        strerror(errno));
    return (DRIVER_OP_ERROR);
  }
}
#endif
#endif /* HAVE_NETBSD_CDROM */

/*!
  Return a string containing the default CD device.
 */
char *
cdio_get_default_device_netbsd()
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*!
  Close tray on CD-ROM.

  @param psz_device the CD-ROM drive to be closed.

*/
driver_return_code_t
close_tray_netbsd (const char *psz_device)
{
#ifdef HAVE_NETBSD_CDROM
  return DRIVER_OP_UNSUPPORTED;
#else
  return DRIVER_OP_NO_DRIVER;
#endif
}

#ifdef HAVE_NETBSD_CDROM
static cdio_funcs_t _funcs = {
  .audio_get_volume      = audio_get_volume_netbsd,
  .audio_pause           = audio_pause_netbsd,
  .audio_play_msf        = audio_play_msf_netbsd,
  .audio_play_track_index= NULL,
#if USE_MMC_SUBCHANNEL
  .audio_read_subchannel = audio_read_subchannel_mmc,
#else
  .audio_read_subchannel = audio_read_subchannel_netbsd,
#endif
  .audio_stop            = audio_stop_netbsd,
  .audio_resume          = audio_resume_netbsd,
  .audio_set_volume      = audio_set_volume_netbsd,
  .eject_media           = eject_media_netbsd,
  .free                  = cdio_generic_free,
  .get_arg               = get_arg_netbsd,
  .get_blocksize         = get_blocksize_mmc,
  .get_cdtext            = get_cdtext_generic,
  .get_cdtext_raw        = read_cdtext_generic,
  .get_default_device    = cdio_get_default_device_netbsd,
  .get_devices           = cdio_get_devices_netbsd,
  .get_disc_last_lsn     = get_disc_last_lsn_netbsd,
  .get_last_session      = get_last_session_netbsd,
  .get_media_changed     = get_media_changed_mmc,
  .get_discmode          = get_discmode_generic,
  .get_drive_cap         = get_drive_cap_mmc,
  .get_first_track_num   = get_first_track_num_netbsd,
  .get_hwinfo            = NULL,
  .get_mcn               = get_mcn_mmc,
  .get_num_tracks        = get_num_tracks_netbsd,
  .get_track_channels    = get_track_channels_generic,
  .get_track_copy_permit = get_track_copy_permit_generic,
  .get_track_format      = get_track_format_netbsd,
  .get_track_green       = get_track_green_netbsd,
   /* Not because we can't talk LBA, but the driver assumes MSF throughout */
  .get_track_lba         = NULL,
  .get_track_preemphasis = get_track_preemphasis_generic,
  .get_track_msf         = get_track_msf_netbsd,
  .get_track_isrc        = get_track_isrc_netbsd,
  .lseek                 = cdio_generic_lseek,
  .read                  = cdio_generic_read,
  .read_audio_sectors    = read_audio_sectors_netbsd,
  .read_data_sectors     = read_data_sectors_generic,
  .read_mode1_sector     = _read_mode1_sector_netbsd,
  .read_mode1_sectors    = _read_mode1_sectors_netbsd,
  .read_mode2_sector     = read_mode2_sector_netbsd,
  .read_mode2_sectors    = read_mode2_sectors_netbsd,
  .read_toc              = read_toc_netbsd,
  .run_mmc_cmd           = run_scsi_cmd_netbsd,
  .set_arg               = set_arg_netbsd,
};
#endif /*HAVE_NETBSD_CDROM*/

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_netbsd(const char *source_name)
{
#ifdef HAVE_NETBSD_CDROM
    CdIo_t *ret;
    _img_private_t *_data;
    int open_access_mode;  /* Access mode passed to cdio_generic_init. */

    _data = calloc(1, sizeof(_img_private_t));

    _data->gen.init = false;
    _data->toc_valid = false;
    _data->sessionformat_valid = false;
    _data->gen.fd = -1;
    _data->gen.b_cdtext_error = false;

    set_arg_netbsd(_data, "source",
                   (source_name ? source_name : DEFAULT_CDIO_DEVICE));

    if (source_name && !cdio_is_device_generic(source_name))
        goto err_exit;

    ret = cdio_new(&_data->gen, &_funcs);
    if (ret == NULL) {
        goto err_exit;
    }

    ret->driver_id = DRIVER_NETBSD;

  open_access_mode = O_NONBLOCK;
  if (_AM_MMC_RDWR == _data->access_mode)
    open_access_mode |= O_RDWR;
  else if (_AM_MMC_RDWR_EXCL == _data->access_mode)
    open_access_mode |= O_RDWR | O_EXCL;
  else
    open_access_mode |= O_RDONLY;
  if (cdio_generic_init(_data, open_access_mode)) {
    return ret;
  }
  free(ret);

 err_exit:
    cdio_generic_free(_data);
    return NULL;

#else
  return NULL;
#endif /* HAVE_NETBSD_CDROM */

}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo_t *
cdio_open_am_netbsd(const char *source_name, const char *am)
{
  return (cdio_open_netbsd(source_name));
}

bool
cdio_have_netbsd (void)
{
#ifdef HAVE_NETBSD_CDROM
  return true;
#else
  return false;
#endif /* HAVE_NETBSD_CDROM */
}


/*
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
