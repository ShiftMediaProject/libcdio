/* -*- c -*-
  $Id: types.swg,v 1.6 2008/05/01 16:55:06 karl Exp $

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

/* 
   Various typedef's "constant"s/variables

   Many of these are documented more fully in libcdio's <cdio/types.h>
   I'm not sure why, but including that directly will not allow
   SWIG to understand say that a lsn_t is an int.
*/
#include <sys/types.h>

#ifdef NEED_LONG
#define LONG long
#else 
#define LONG
#endif

#define uint32_t LONG unsigned int
#define int32_t  LONG int

typedef long int driver_id_t;
typedef int lsn_t;
typedef int int16_t;
typedef int lba_t;
typedef unsigned int track_t;
typedef long int driver_return_code_t;
typedef long int ssize_t;

%constant long int VERSION_NUM = LIBCDIO_VERSION_NUM;

%constant long int INVALID_LBA = CDIO_INVALID_LBA;
%constant long int INVALID_LSN = CDIO_INVALID_LSN;

/* More documentation on the below is in cdio/sector.h */
%constant long int CD_FRAMESIZE      = CDIO_CD_FRAMESIZE;
%constant long int CD_FRAMESIZE_RAW  = CDIO_CD_FRAMESIZE_RAW;
%constant long int ISO_BLOCKSIZE     = CDIO_CD_FRAMESIZE;
%constant long int M2F2_SECTOR_SIZE  = M2F2_SECTOR_SIZE; 
%constant long int M2RAW_SECTOR_SIZE = M2RAW_SECTOR_SIZE;
