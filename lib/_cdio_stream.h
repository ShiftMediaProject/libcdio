/*
    $Id: _cdio_stream.h,v 1.1 2003/03/24 19:01:09 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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


#ifndef __CDIO_STREAM_H__
#define __CDIO_STREAM_H__

#include "cdio_types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* typedef'ed IO functions prototypes */

typedef int(*cdio_data_open_t)(void *user_data);

typedef long(*cdio_data_read_t)(void *user_data, void *buf, long count);

typedef long(*cdio_data_seek_t)(void *user_data, long offset);

typedef long(*cdio_data_stat_t)(void *user_data);

typedef int(*cdio_data_close_t)(void *user_data);

typedef void(*cdio_data_free_t)(void *user_data);


/* abstract data source */

typedef struct _CdioDataSource CdioDataSource;

typedef struct {
  cdio_data_open_t open;
  cdio_data_seek_t seek; 
  cdio_data_stat_t stat; 
  cdio_data_read_t read;
  cdio_data_close_t close;
  cdio_data_free_t free;
} cdio_stream_io_functions;

CdioDataSource*
cdio_stream_new(void *user_data, const cdio_stream_io_functions *funcs);

long
cdio_stream_read(CdioDataSource* obj, void *ptr, long size, long nmemb);

long
cdio_stream_seek(CdioDataSource* obj, long offset);

long
cdio_stream_stat(CdioDataSource* obj);

void
cdio_stream_destroy(CdioDataSource* obj);

void
cdio_stream_close(CdioDataSource* obj);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_STREAM_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
