/*
    $Id: read.c,v 1.1 2005/01/09 16:07:46 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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
/** \file read.h 
 *
 * \brief sector (block, frame)-related libcdio routines.
 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdio.h>
#include "cdio_private.h"
#include "cdio_assert.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

/*!
  lseek - reposition read/write file offset
  Returns (off_t) -1 on error. 
  Similar to (if not the same as) libc's lseek()
*/
off_t
cdio_lseek (const CdIo_t *p_cdio, off_t offset, int whence)
{
  if (p_cdio == NULL) return -1;
  
  if (p_cdio->op.lseek)
    return p_cdio->op.lseek (p_cdio->env, offset, whence);
  return -1;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Similar to (if not the same as) libc's read()
*/
ssize_t
cdio_read (const CdIo_t *p_cdio, void *p_buf, size_t size)
{
  if (p_cdio == NULL) return -1;
  
  if (p_cdio->op.read)
    return p_cdio->op.read (p_cdio->env, p_buf, size);
  return -1;
}

/*!
  Reads an audio sector from cd device into data starting
  from lsn. Returns 0 if no error. 
*/
int
cdio_read_audio_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t lsn) 
{

  if (NULL == p_cdio || NULL == p_buf || CDIO_INVALID_LSN == lsn )
    return 0;

  if  (p_cdio->op.read_audio_sectors != NULL)
    return p_cdio->op.read_audio_sectors (p_cdio->env, p_buf, lsn, 1);
  return -1;
}

/*!
  Reads audio sectors from cd device into data starting
  from lsn. Returns 0 if no error. 
*/
int
cdio_read_audio_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t lsn,
                         unsigned int nblocks) 
{
  if ( NULL == p_cdio || NULL == p_buf || CDIO_INVALID_LSN == lsn )
    return 0;

  if  (p_cdio->op.read_audio_sectors != NULL)
    return p_cdio->op.read_audio_sectors (p_cdio->env, p_buf, lsn, nblocks);
  return -1;
}

#ifndef SEEK_SET
#define SEEK_SET 0
#endif 

/*!
   Reads a single mode1 form1 or form2  sector from cd device 
   into data starting from lsn. Returns 0 if no error. 
 */
int
cdio_read_mode1_sector (const CdIo_t *p_cdio, void *data, lsn_t lsn, 
                        bool b_form2)
{
  uint32_t size = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE ;
  
  if (NULL == p_cdio || NULL == data || CDIO_INVALID_LSN == lsn )
    return 0;

  if (p_cdio->op.read_mode1_sector) {
    return p_cdio->op.read_mode1_sector(p_cdio->env, data, lsn, b_form2);
  } else if (p_cdio->op.lseek && p_cdio->op.read) {
    char buf[CDIO_CD_FRAMESIZE] = { 0, };
    if (0 > cdio_lseek(p_cdio, CDIO_CD_FRAMESIZE*lsn, SEEK_SET))
      return -1;
    if (0 > cdio_read(p_cdio, buf, CDIO_CD_FRAMESIZE))
      return -1;
    memcpy (data, buf, size);
    return 0;
  } 

  return 1;

}

/*!
  Reads mode 1 sectors
  
  @param p_cdio object to read from
  @param buf place to read data into
  @param lsn sector to read
  @param b_form2 true for reading mode 1 form 2 sectors or false for 
  mode 1 form 1 sectors.
  @param i_sectors number of sectors to read
  
  @return 0 if no error, nonzero otherwise.
*/
int
cdio_read_mode1_sectors (const CdIo_t *cdio, void *p_buf, lsn_t lsn, 
                         bool b_form2,  unsigned int num_sectors)
{

  if (NULL == cdio || NULL == p_buf || CDIO_INVALID_LSN == lsn )
    return 0;

  cdio_assert (cdio->op.read_mode1_sectors != NULL);
  
  return cdio->op.read_mode1_sectors (cdio->env, p_buf, lsn, b_form2, 
                                      num_sectors);
}

/*!
  Reads a mode 2 sector
  
  @param p_cdio object to read from
  @param buf place to read data into
  @param lsn sector to read
  @param b_form2 true for reading mode 2 form 2 sectors or false for 
  mode 2 form 1 sectors.
  
  @return 0 if no error, nonzero otherwise.
*/
int
cdio_read_mode2_sector (const CdIo_t *p_cdio, void *p_buf, lsn_t lsn, 
                        bool b_form2)
{
  if (NULL == p_cdio || NULL == p_buf || CDIO_INVALID_LSN == lsn )
    return 0;

  cdio_assert (p_cdio->op.read_mode2_sector != NULL 
	      || p_cdio->op.read_mode2_sectors != NULL);

  if (p_cdio->op.read_mode2_sector)
    return p_cdio->op.read_mode2_sector (p_cdio->env, p_buf, lsn, b_form2);

  /* fallback */
  if (p_cdio->op.read_mode2_sectors != NULL)
    return cdio_read_mode2_sectors (p_cdio, p_buf, lsn, b_form2, 1);
  return 1;
}

/*!
  Reads mode 2 sectors
  
  @param p_cdio object to read from
  @param buf place to read data into
  @param lsn sector to read
  @param b_form2 true for reading mode2 form 2 sectors or false for 
  mode 2  form 1 sectors.
  @param i_sectors number of sectors to read
  
  @return 0 if no error, nonzero otherwise.
*/
int
cdio_read_mode2_sectors (const CdIo_t *p_cdio, void *p_buf, lsn_t i_lsn, 
                         bool b_form2, unsigned int i_sectors)
{

  if (NULL == p_cdio || NULL == p_buf || CDIO_INVALID_LSN == i_lsn )
    return 0;

  cdio_assert (p_cdio->op.read_mode2_sectors != NULL);
  
  return p_cdio->op.read_mode2_sectors (p_cdio->env, p_buf, i_lsn,
					b_form2, i_sectors);
}


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
