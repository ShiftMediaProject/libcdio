/*
    $Id: xa.h,v 1.2 2003/08/31 08:32:40 rocky Exp $

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

/* XA attribute definitions */
#define XA_ATTR_U_READ         (1 << 0)
/* reserved */
#define XA_ATTR_U_EXEC         (1 << 2)
/* reserved */
#define XA_ATTR_G_READ         (1 << 4)
/* reserved */
#define XA_ATTR_G_EXEC         (1 << 6)
/* reserved */
#define XA_ATTR_O_READ         (1 << 8)
/* reserved */
#define XA_ATTR_O_EXEC         (1 << 10)

#define XA_ATTR_MODE2FORM1     (1 << 11)
#define XA_ATTR_MODE2FORM2     (1 << 12)
#define XA_ATTR_INTERLEAVED    (1 << 13)
#define XA_ATTR_CDDA           (1 << 14)
#define XA_ATTR_DIRECTORY      (1 << 15)

/* some aggregations */
#define XA_PERM_ALL_READ       (XA_ATTR_U_READ | XA_ATTR_G_READ | XA_ATTR_O_READ)
#define XA_PERM_ALL_EXEC       (XA_ATTR_U_EXEC | XA_ATTR_G_EXEC | XA_ATTR_O_EXEC)
#define XA_PERM_ALL_ALL        (XA_PERM_ALL_READ | XA_PERM_ALL_EXEC)

#define XA_FORM1_DIR    (XA_ATTR_DIRECTORY | XA_ATTR_MODE2FORM1 | XA_PERM_ALL_ALL)
#define XA_FORM1_FILE   (XA_ATTR_MODE2FORM1 | XA_PERM_ALL_ALL)
#define XA_FORM2_FILE   (XA_ATTR_MODE2FORM2 | XA_PERM_ALL_ALL)

typedef struct iso9660_xa  iso9660_xa_t;

struct iso9660_xa /* big endian!! */
{
  uint16_t user_id;       /* 0 */
  uint16_t group_id;      /* 0 */
  uint16_t attributes;    /* XA_ATTR_ */ 
  uint8_t  signature[2];  /* { 'X', 'A' } */
  uint8_t  filenum;       /* file number, see also XA subheader */
  uint8_t  reserved[5];   /* zero */
} GNUC_PACKED;


iso9660_xa_t *
iso9660_xa_init (iso9660_xa_t *_xa, uint16_t uid, uint16_t gid, uint16_t attr, 
		 uint8_t filenum);

