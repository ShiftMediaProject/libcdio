/*
    $Id: sector.c,v 1.4 2003/09/11 02:50:06 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"

#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif


static const char _rcsid[] = "$Id: sector.c,v 1.4 2003/09/11 02:50:06 rocky Exp $";

lba_t
cdio_lba_to_lsn (lba_t lba)
{
  return lba - CDIO_PREGAP_SECTORS; 
}

void
cdio_lba_to_msf (lba_t lba, msf_t *msf)
{
  cdio_assert (msf != 0);

  msf->m = to_bcd8 (lba / (60 * 75));
  msf->s = to_bcd8 ((lba / 75) % 60);
  msf->f = to_bcd8 (lba % 75);
}

/* warning, returns new allocated string */
char *
cdio_lba_to_msf_str (lba_t lba)
{
  char buf[16];
  msf_t _msf = { .m = 0, .s = 0, .f = 0 };

  cdio_lba_to_msf (lba, &_msf);

  snprintf (buf, sizeof (buf), "%.2x:%.2x.%.2x", _msf.m, _msf.s, _msf.f);

  return strdup (buf);
}

lba_t
cdio_lsn_to_lba (lsn_t lsn)
{
  return lsn + CDIO_PREGAP_SECTORS; 
}

void
cdio_lsn_to_msf (lsn_t lsn, msf_t *msf)
{
  cdio_assert (msf != 0);
  cdio_lba_to_msf(cdio_lsn_to_lba(lsn), msf);
}

uint32_t
cdio_msf_to_lba (const msf_t *msf)
{
  uint32_t lba = 0;

  cdio_assert (msf != 0);

  lba = from_bcd8 (msf->m);
  lba *= 60;

  lba += from_bcd8 (msf->s);
  lba *= 75;
  
  lba += from_bcd8 (msf->f);

  return lba;
}

uint32_t
cdio_msf_to_lsn (const msf_t *msf)
{
  return cdio_lba_to_lsn(cdio_msf_to_lba (msf));
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
