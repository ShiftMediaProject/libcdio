/* -*- c -*-
  $Id: read.swg,v 1.7 2008/05/01 16:55:05 karl Exp $

  Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>

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
/* See <cdio/read.h> for more extensive documentation. */

%constant long int READ_MODE_AUDIO = CDIO_READ_MODE_AUDIO;
%constant long int READ_MODE_M1F1  = CDIO_READ_MODE_M1F1;
%constant long int READ_MODE_M1F2  = CDIO_READ_MODE_M1F2;
%constant long int READ_MODE_M2F1  = CDIO_READ_MODE_M2F1;
%constant long int READ_MODE_M2F2  = CDIO_READ_MODE_M2F2;

typedef int cdio_read_mode_t;

%rename cdio_lseek lseek;
%feature("autodoc",
"lseek(cdio, offset, whence)->int
Reposition read offset
Similar to (if not the same as) libc's fseek()

cdio is object to get adjested, offset is amount to seek and 
whence is like corresponding parameter in libc's lseek, e.g. 
it should be SEEK_SET or SEEK_END.

the offset is returned or -1 on error.");
off_t cdio_lseek(const CdIo_t *p_cdio, int offset, int whence=SEEK_SET);
    
%feature("autodoc",
"read_cd(cdio, size)->[size, data]

Reads into buf the next size bytes.
Similar to (if not the same as) libc's read()

The number of reads read is returned. -1 is returned on error.");
%cstring_output_withsize(char *p_buf, ssize_t *pi_size);
ssize_t read_cd(const CdIo_t *p_cdio, char *p_buf, ssize_t *pi_size);

%inline %{
ssize_t
read_cd(const CdIo_t *p_cdio, char *p_buf, ssize_t *pi_size) 
{
  *pi_size = cdio_read(p_cdio, p_buf, *pi_size);
  return *pi_size;
}
%}


%feature("autodoc",
"read_sectors(bytes, lsn, read_mode)->[size, data]
Reads a number of sectors (AKA blocks).
  
lsn is sector to read, bytes is the number of bytes.

If read_mode is pycdio.MODE_AUDIO, the return buffer size will be
truncated to multiple of pycdio.CDIO_FRAMESIZE_RAW i_blocks bytes.

If read_mode is pycdio.MODE_DATA, buffer will be truncated to a
multiple of pycdio.ISO_BLOCKSIZE, pycdio.M1RAW_SECTOR_SIZE or
pycdio.M2F2_SECTOR_SIZE bytes depending on what mode the data is in.
If read_mode is pycdio.CDIO_MODE_M2F1, buffer will be truncated to a 
multiple of pycdio.M2RAW_SECTOR_SIZE bytes.

If read_mode is CDIO_MODE_M2F2, the return buffer size will be
truncated to a multiple of pycdio.CD_FRAMESIZE bytes.

If size <= 0 an error has occurred.");

%inline %{

ssize_t read_sectors(const CdIo_t *p_cdio, char *p_buf, ssize_t *pi_size, 
		     lsn_t i_lsn, cdio_read_mode_t read_mode)
{
  
  driver_return_code_t drc;
  uint32_t i_blocks;
  uint16_t i_blocksize;
  switch (read_mode) {
  case CDIO_READ_MODE_AUDIO: 
    i_blocksize = CDIO_CD_FRAMESIZE_RAW;
    break;
  case CDIO_READ_MODE_M1F1: 
    i_blocksize = M2RAW_SECTOR_SIZE;
    break;
  case CDIO_READ_MODE_M1F2: 
    i_blocksize = M2RAW_SECTOR_SIZE;
    break;
  case CDIO_READ_MODE_M2F1: 
    i_blocksize = CDIO_CD_FRAMESIZE;
    break;
  case CDIO_READ_MODE_M2F2: 
    i_blocksize = M2F2_SECTOR_SIZE;
    break;
  default: 
    pi_size = NULL;
    return DRIVER_OP_BAD_PARAMETER;
  }

  i_blocks = *pi_size / i_blocksize;
  drc = cdio_read_sectors(p_cdio, p_buf, i_lsn, read_mode, i_blocks);
  if (drc < 0) {
    pi_size = NULL;
    return drc;
  }
  return *pi_size;
}
%}

%feature("autodoc",
"read_data_bytes(lsn, bytes)

Reads a number of data sectors (AKA blocks).
  
lsn is sector to read, bytes is the number of bytes.
If you don't know whether you have a Mode 1/2, 
Form 1/ Form 2/Formless sector best to reserve space for the maximum
which is pycdio.M2RAW_SECTOR_SIZE.

If size <= 0 an error has occurred.");
%inline %{
ssize_t read_data_bytes(const CdIo_t *p_cdio, char *p_buf, ssize_t *pi_size, 
			lsn_t i_lsn, int16_t i_blocksize)
{
  driver_return_code_t drc;
  uint32_t i_blocks = *pi_size / i_blocksize;

  switch (i_blocksize) {
  case CDIO_CD_FRAMESIZE:
  case CDIO_CD_FRAMESIZE_RAW:
  case M2F2_SECTOR_SIZE:
  case M2RAW_SECTOR_SIZE:
    break;
  default:
    /* Don't know about these block sizes */
    pi_size = NULL;
    return DRIVER_OP_BAD_PARAMETER;
  }
  
#if DEBUGGING  
  printf("p_cdio: %x, i_size: %d, lsn: %d, blocksize %d, blocks %d\n",
	 p_cdio, *pi_size, i_lsn, i_blocksize, i_blocks);
#endif
  drc = cdio_read_data_sectors (p_cdio, p_buf, i_lsn, 
				i_blocksize, i_blocks);
#if DEBUGGING  
  printf("drc: %d\n", drc);
#endif
  if (drc < 0) {
    pi_size = NULL;
    return drc;
  }
  return *pi_size;
}
%}
