/*
    $Id: _cdio_nrg.c,v 1.18 2003/09/25 09:38:16 rocky Exp $

    Copyright (C) 2001,2003 Herbert Valerio Riedel <hvr@gnu.org>

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
/*! This code implements low-level access functions for the Nero native
   CD-image format residing inside a disk file (*.nrg).
*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "bytesex.h"
#include "ds.h"
#include "cdio_private.h"
#include "_cdio_stdio.h"

static const char _rcsid[] = "$Id: _cdio_nrg.c,v 1.18 2003/09/25 09:38:16 rocky Exp $";

/* structures used */

/* this ugly image format is typical for lazy win32 programmers... at
   least structure were set big endian, so at reverse
   engineering wasn't such a big headache... */

PRAGMA_BEGIN_PACKED
typedef struct {
  uint32_t start      GNUC_PACKED;
  uint32_t length     GNUC_PACKED;
  uint32_t type       GNUC_PACKED; /* only 0x3 seen so far... 
                                      -> MIXED_MODE2 2336 blocksize */
  uint32_t start_lsn  GNUC_PACKED; /* does not include any pre-gaps! */
  uint32_t _unknown   GNUC_PACKED; /* wtf is this for? -- always zero... */
} _etnf_array_t;

/* finally they realized that 32bit offsets are a bit outdated for IA64 *eg* */
typedef struct {
  uint64_t start      GNUC_PACKED;
  uint64_t length     GNUC_PACKED;
  uint32_t type       GNUC_PACKED;
  uint32_t start_lsn  GNUC_PACKED;
  uint64_t _unknown   GNUC_PACKED; /* wtf is this for? -- always zero... */
} _etn2_array_t;

typedef struct {
  uint8_t  _unknown1  GNUC_PACKED; /* 0x41 == 'A' */
  uint8_t  track      GNUC_PACKED; /* binary or BCD?? */
  uint8_t  index      GNUC_PACKED; /* makes 0->1 transitions */
  uint8_t  _unknown2  GNUC_PACKED; /* ?? */
  uint32_t lsn        GNUC_PACKED; 
} _cuex_array_t;

typedef struct {
  uint32_t id                    GNUC_PACKED;
  uint32_t len                   GNUC_PACKED;
  char data[EMPTY_ARRAY_SIZE]    GNUC_PACKED;
} _chunk_t;

PRAGMA_END_PACKED

/* to be converted into BE */
#define CUEX_ID  0x43554558
#define CUES_ID  0x43554553
#define DAOX_ID  0x44414f58
#define DAOI_ID  0x44414f49
#define END1_ID  0x454e4421
#define ETN2_ID  0x45544e32
#define ETNF_ID  0x45544e46
#define NER5_ID  0x4e455235
#define NERO_ID  0x4e45524f
#define SINF_ID  0x53494e46

/* reader */

#define DEFAULT_CDIO_DEVICE "image.nrg"

typedef struct {
  int            track_num;  /* Probably is index+1 */
  msf_t          start_msf;
  lba_t          start_lba;
  int            start_index;
  int            sec_count;  /* Number of sectors in track. Does not 
				 include pregap before next entry. */
  int            flags;      /* "DCP", "4CH", "PRE" */
  track_format_t track_format;
  bool           track_green;
  uint16_t  datasize;        /* How much is in the portion we return back? */
  long int  datastart;       /* Offset from begining that data starts */
  uint16_t  endsize;         /* How much stuff at the end to skip over. This
			       stuff may have error correction (EDC, or ECC).*/
  uint16_t  blocksize;       /* total block size = start + size + end */
} track_info_t;

/* 
   Link element of track structure as a linked list.
   Possibly redundant with above track_info_t */
typedef struct {
  uint32_t start_lsn;
  uint32_t sec_count;     /* Number of sectors in track. Does not 
			     include pregap before next entry. */
  uint64_t img_offset;    /* Bytes offset from beginning of disk image file.*/
} _mapping_t;


typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  internal_position_t pos; 

  /* This is a hack because I don't really understnad NERO better. */
  bool            is_cues;

  bool            sector_2336;
  track_info_t    tocent[100];     /* entry info for each track */
  track_t         total_tracks;    /* number of tracks in image */
  track_t         first_track_num; /* track number of first track */
  CdioList       *mapping;         /* List of track information */
  uint32_t size;
} _img_private_t;

static bool     _cdio_parse_nero_footer (_img_private_t *_obj);
static uint32_t _cdio_stat_size (void *env);

/* Updates internal track TOC, so we can later 
   simulate ioctl(CDROMREADTOCENTRY).
 */
static void
_register_mapping (_img_private_t *_obj, lsn_t start_lsn, uint32_t sec_count,
		   uint64_t img_offset, uint32_t blocksize)
{
  const int track_num=_obj->total_tracks;
  track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
  _mapping_t *_map = _cdio_malloc (sizeof (_mapping_t));

  if (!_obj->mapping)
    _obj->mapping = _cdio_list_new ();

  _cdio_list_append (_obj->mapping, _map);
  _map->start_lsn  = start_lsn;
  _map->sec_count  = sec_count;

  _map->img_offset = img_offset;

  _obj->size = MAX (_obj->size, (start_lsn + sec_count));

  /* Update *this_track and track_num. These structures are
     in a sense redundant witht the obj->mapping list. Perhaps one
     or the other can be eliminated.
   */

  cdio_lba_to_msf (cdio_lsn_to_lba(start_lsn), &(this_track->start_msf));
  this_track->start_lba = cdio_msf_to_lba(&this_track->start_msf);
  this_track->track_num = track_num+1;
  this_track->blocksize = blocksize;
  if (_obj->is_cues) 
    this_track->datastart = img_offset + 8;
  else 
    this_track->datastart = 8;
      
  this_track->sec_count = sec_count;

  _obj->total_tracks++;

  /* FIXME: These are probably not right. Probably we need to look at 
     "type." But this hasn't been reverse engineered yet.
   */
  this_track->track_format= TRACK_FORMAT_XA;
  this_track->track_green = true;

  /* cdio_debug ("map: %d +%d -> %ld", start_lsn, sec_count, img_offset); */
}


/* 
   Disk and track information for a Nero file are located at the end
   of the file. This routine extracts that information.
 */
static bool
_cdio_parse_nero_footer (_img_private_t *_obj)
{
  long unsigned int footer_start;
  long unsigned int size;
  char *footer_buf = NULL;

  if (_obj->size)
    return 0;

  size = cdio_stream_stat (_obj->gen.data_source);

  {
PRAGMA_BEGIN_PACKED
    union {
      struct {
	uint32_t __x          GNUC_PACKED;
	uint32_t ID           GNUC_PACKED;
	uint32_t footer_ofs   GNUC_PACKED;
      } v50;
      struct {
	uint32_t ID           GNUC_PACKED;
	uint64_t footer_ofs   GNUC_PACKED;
      } v55;
    } buf;
PRAGMA_END_PACKED

    cdio_assert (sizeof (buf) == 12);
 
 cdio_stream_seek (_obj->gen.data_source, size - sizeof (buf), SEEK_SET);
    cdio_stream_read (_obj->gen.data_source, (void *) &buf, sizeof (buf), 1);
    
    if (buf.v50.ID == UINT32_TO_BE (0x4e45524f)) /* "NERO" */
      {
	cdio_info ("detected v50 (32bit offsets) NRG magic");
	footer_start = uint32_to_be (buf.v50.footer_ofs); 
      }
    else if (buf.v55.ID == UINT32_TO_BE (0x4e455235)) /* "NER5" */
      {
	cdio_info ("detected v55 (64bit offsets) NRG magic");
	footer_start = uint64_from_be (buf.v55.footer_ofs);
      }
    else
      {
	cdio_error ("Image not recognized as either v50 or v55 type NRG");
	return -1;
      }

    cdio_debug ("nrg footer start = %ld, length = %ld", 
	       (long) footer_start, (long) (size - footer_start));

    cdio_assert (IN ((size - footer_start), 0, 4096));

    footer_buf = _cdio_malloc (size - footer_start);

    cdio_stream_seek (_obj->gen.data_source, footer_start, SEEK_SET);
    cdio_stream_read (_obj->gen.data_source, footer_buf, size - footer_start, 1);
  }

  {
    int pos = 0;

    while (pos < size - footer_start) {
      _chunk_t *chunk = (void *) (footer_buf + pos);
      
      bool break_out = false;
      
      switch (UINT32_FROM_BE (chunk->id)) {
      case CUES_ID: { /* "CUES" Seems to have sector size 2336 and 150 sector
			 pregap seems to be included at beginning of image.
		       */


	unsigned entries = UINT32_FROM_BE (chunk->len);
	_cuex_array_t *_entries = (void *) chunk->data;
	
	cdio_assert (_obj->mapping == NULL);
	
	cdio_assert (sizeof (_cuex_array_t) == 8);
	cdio_assert ( UINT32_FROM_BE(chunk->len) % sizeof(_cuex_array_t) 
		      == 0 );
	
	entries /= sizeof (_cuex_array_t);
	
	cdio_info ("CUES type image detected");
	
	{
	  lsn_t lsn = UINT32_FROM_BE (_entries[0].lsn);
	  int idx;
	  
	  /*cdio_assert (lsn == 0?);*/

	  _obj->is_cues         = true; /* HACK alert. */
	  _obj->sector_2336     = true;
	  _obj->total_tracks    = 0;
	  _obj->first_track_num = 1;
	  for (idx = 1; idx < entries-1; idx += 2) {
	    lsn_t sec_count;
	    
	    cdio_assert (_entries[idx].index == 0);
	    cdio_assert (_entries[idx].track == _entries[idx + 1].track);
	    
	    /* lsn and sec_count*2 aren't correct, but it comes closer on the
	       single example I have: svcdgs.nrg
	       We are picking up the wrong fields and/or not interpreting
	       them correctly.
	     */

	    lsn       = UINT32_FROM_BE (_entries[idx].lsn);
	    sec_count = UINT32_FROM_BE (_entries[idx + 1].lsn);

	    _register_mapping (_obj, lsn, sec_count*2, 
              (lsn+CDIO_PREGAP_SECTORS) * M2RAW_SECTOR_SIZE, 
	       M2RAW_SECTOR_SIZE);
	  }
	} 
	break;
      }
	
      case CUEX_ID: /* "CUEX" */ {
	unsigned entries = UINT32_FROM_BE (chunk->len);
	_cuex_array_t *_entries = (void *) chunk->data;
	
	cdio_assert (_obj->mapping == NULL);
	
	cdio_assert ( sizeof (_cuex_array_t) == 8 );
	cdio_assert ( UINT32_FROM_BE (chunk->len) % sizeof(_cuex_array_t) 
		      == 0 );
	
	entries /= sizeof (_cuex_array_t);
	
	cdio_info ("DAO type image detected");
	
	{
	  uint32_t lsn = UINT32_FROM_BE (_entries[0].lsn);
	  int idx;
	  
	  cdio_assert (lsn == 0xffffff6a);
	  
	  for (idx = 2; idx < entries; idx += 2) {
	    lsn_t lsn2;

	    cdio_assert (_entries[idx].index == 1);
	    cdio_assert (_entries[idx].track != _entries[idx + 1].track);
	    
	    lsn = UINT32_FROM_BE (_entries[idx].lsn);
	    lsn2 = UINT32_FROM_BE (_entries[idx + 1].lsn);
	    
	    _register_mapping (_obj, lsn, lsn2 - lsn, 
			       (lsn + CDIO_PREGAP_SECTORS)*M2RAW_SECTOR_SIZE, 
			       M2RAW_SECTOR_SIZE);
	  }
	}    
	break;
      }
	
      case DAOI_ID: /* "DAOI" */
	cdio_debug ("DAOI tag detected...");
	break;
      case DAOX_ID: /* "DAOX" */
	cdio_debug ("DAOX tag detected...");
	break;
	
      case NER5_ID: /* "NER5" */
	cdio_error ("unexpected nrg magic ID NER5 detected");
	return -1;
	break;

      case NERO_ID: /* "NER0" */
	cdio_error ("unexpected nrg magic ID NER0 detected");
	return -1;
	break;
	
      case END1_ID: /* "END!" */
	cdio_debug ("nrg end tag detected");
	break_out = true;
	break;
	
      case ETNF_ID: /* "ETNF" */ {
	unsigned entries = UINT32_FROM_BE (chunk->len);
	_etnf_array_t *_entries = (void *) chunk->data;
	
	cdio_assert (_obj->mapping == NULL);
	
	cdio_assert ( sizeof (_etnf_array_t) == 20 );
	cdio_assert ( UINT32_FROM_BE(chunk->len) % sizeof(_etnf_array_t) 
		      == 0 );
	
	entries /= sizeof (_etnf_array_t);
	
	cdio_info ("SAO type image (ETNF) detected");
	
	_obj->sector_2336 = true;

	{
	  int idx;
	  for (idx = 0; idx < entries; idx++) {
	    uint32_t _len = UINT32_FROM_BE (_entries[idx].length);
	    uint32_t _start = UINT32_FROM_BE (_entries[idx].start_lsn);
	    uint32_t _start2 = UINT32_FROM_BE (_entries[idx].start);
	    
	    cdio_assert (UINT32_FROM_BE (_entries[idx].type) == 3);
	    cdio_assert (_len % M2RAW_SECTOR_SIZE == 0);
	    
	    _len /= M2RAW_SECTOR_SIZE;
	    
	    cdio_assert (_start * M2RAW_SECTOR_SIZE == _start2);
	    
	    _start += idx * CDIO_PREGAP_SECTORS;
	    _register_mapping (_obj, _start, _len, _start2, M2RAW_SECTOR_SIZE);

	  }
	}
	break;
      }
      
      case ETN2_ID: { /* "ETN2", same as above, but with 64bit stuff instead */
	unsigned entries = uint32_from_be (chunk->len);
	_etn2_array_t *_entries = (void *) chunk->data;
	
	cdio_assert (_obj->mapping == NULL);
	
	cdio_assert (sizeof (_etn2_array_t) == 32);
	cdio_assert (uint32_from_be (chunk->len) % sizeof (_etn2_array_t) == 0);
	
	entries /= sizeof (_etn2_array_t);
	
	cdio_info ("SAO type image (ETN2) detected");

	_obj->sector_2336 = true;
	
	{
	  int idx;
	  for (idx = 0; idx < entries; idx++) {
	    uint32_t _len = uint64_from_be (_entries[idx].length);
	    uint32_t _start = uint32_from_be (_entries[idx].start_lsn);
	    uint32_t _start2 = uint64_from_be (_entries[idx].start);
	    
	    cdio_assert (uint32_from_be (_entries[idx].type) == 3);
	    cdio_assert (_len % M2RAW_SECTOR_SIZE == 0);
	    
	    _len /= M2RAW_SECTOR_SIZE;
	    
	    cdio_assert (_start * M2RAW_SECTOR_SIZE == _start2);
	    
	    _start += idx * CDIO_PREGAP_SECTORS;
	    _register_mapping (_obj, _start, _len, _start2, M2RAW_SECTOR_SIZE);
	  }
	}
	break;
      }
	
      case SINF_ID: { /* "SINF" */
	
	uint32_t *_sessions = (void *) chunk->data;
	
	cdio_assert (UINT32_FROM_BE (chunk->len) == 4);
	
	cdio_debug ("SINF: %lu sessions", 
		    (long unsigned int) UINT32_FROM_BE (*_sessions));
      }
	break;
	
      default:
	cdio_warn ("unknown tag %8.8x seen", 
		   (unsigned int) UINT32_FROM_BE (chunk->id));
	break;
      }
	
      if (break_out)
	break;
      
      pos += 8;
      pos += UINT32_FROM_BE (chunk->len);
    }
  }

  /* Fake out leadout track. */
  /* Don't use _cdio_stat_size since that will lead to recursion since
     we haven't fully initialized things yet.
  */
  cdio_lsn_to_msf (_obj->size, &_obj->tocent[_obj->total_tracks].start_msf);
  _obj->tocent[_obj->total_tracks].start_lba = cdio_lsn_to_lba(_obj->size);
  _obj->tocent[_obj->total_tracks-1].sec_count = 
    cdio_lsn_to_lba(_obj->size - _obj->tocent[_obj->total_tracks-1].start_lba);

  return 0;
}

/*!
  Initialize image structures.
 */
static bool
_cdio_init (_img_private_t *_obj)
{
  if (_obj->gen.init) {
    cdio_error ("init called more than once");
    return false;
  }
  
  if (!(_obj->gen.data_source = cdio_stdio_new (_obj->gen.source_name))) {
    cdio_error ("init failed");
    return false;
  }

  _cdio_parse_nero_footer (_obj);
  _obj->gen.init = true;
  return true;

}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Would be libc's seek() but we have to adjust for the extra track header 
  information in each sector.
*/
static off_t
_cdio_lseek (void *env, off_t offset, int whence)
{
  _img_private_t *_obj = env;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. I'm guessing
     the 1st 24 bytes of a bin file are used for something.
  */
  off_t real_offset=0;

  unsigned int i;
  unsigned int user_datasize;

  for (i=0; i<_obj->total_tracks; i++) {
    track_info_t  *this_track=&(_obj->tocent[i]);
    switch (this_track->track_format) {
    case TRACK_FORMAT_AUDIO:
      user_datasize=CDIO_CD_FRAMESIZE_RAW;
      break;
    case TRACK_FORMAT_CDI:
      user_datasize=CDIO_CD_FRAMESIZE;
      break;
    case TRACK_FORMAT_XA:
      user_datasize=CDIO_CD_FRAMESIZE;
      break;
    default:
      user_datasize=CDIO_CD_FRAMESIZE_RAW;
      cdio_warn ("track %d has unknown format %d",
		 i+1, this_track->track_format);
    }

    if ( (this_track->sec_count*user_datasize) >= offset) {
      int blocks = offset / user_datasize;
      int rem    = offset % user_datasize;
      int block_offset = blocks * (M2RAW_SECTOR_SIZE);
      real_offset += block_offset + rem;
      break;
    }
    real_offset += this_track->sec_count*(M2RAW_SECTOR_SIZE);
    
    offset -= this_track->sec_count*user_datasize;
  }

  if (i==_obj->total_tracks) {
    cdio_warn ("seeking outside range of disk image");
    return -1;
  } else
    real_offset += _obj->tocent[i].datastart;
    return cdio_stream_seek(_obj->gen.data_source, real_offset, whence);
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  FIXME: 
   At present we assume a read doesn't cross sector or track
   boundaries.
*/
static ssize_t
_cdio_read (void *env, void *buf, size_t size)
{
  _img_private_t *_obj = env;
  return cdio_stream_read(_obj->gen.data_source, buf, size, 1);
}

static uint32_t 
_cdio_stat_size (void *env)
{
  _img_private_t *_obj = env;

  return _obj->size;
}

/*!
   Reads a single audio sector from CD device into data starting
   from LSN. Returns 0 if no error. 
 */
static int
_cdio_read_audio_sectors (void *env, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *_obj = env;

  CdioListNode *node;

  if (lsn >= _obj->size)
    {
      cdio_warn ("trying to read beyond image size (%lu >= %lu)", 
		 (long unsigned int) lsn, (long unsigned int) _obj->size);
      return -1;
    }

  _CDIO_LIST_FOREACH (node, _obj->mapping) {
    _mapping_t *_map = _cdio_list_node_data (node);
    
    if (IN (lsn, _map->start_lsn, (_map->start_lsn + _map->sec_count - 1))) {
      int ret;
      long int img_offset = _map->img_offset;
      
      img_offset += (lsn - _map->start_lsn) * CDIO_CD_FRAMESIZE_RAW;
      
      ret = cdio_stream_seek (_obj->gen.data_source, img_offset, 
			      SEEK_SET); 
      if (ret!=0) return ret;
      ret = cdio_stream_read (_obj->gen.data_source, data, 
			      CDIO_CD_FRAMESIZE_RAW, nblocks);
      if (ret==0) return ret;
      break;
    }
  }

  if (!node) cdio_warn ("reading into pre gap (lsn %lu)", 
			(long unsigned int) lsn);

  return 0;
}

static int
_cdio_read_mode2_sector (void *env, void *data, lsn_t lsn, 
			 bool mode2_form2)
{
  _img_private_t *_obj = env;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int blocksize = _obj->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  CdioListNode *node;

  if (lsn >= _obj->size)
    {
      cdio_warn ("trying to read beyond image size (%lu >= %lu)", 
		 (long unsigned int) lsn, (long unsigned int) _obj->size);
      return -1;
    }

  _CDIO_LIST_FOREACH (node, _obj->mapping) {
    _mapping_t *_map = _cdio_list_node_data (node);
    
    if (IN (lsn, _map->start_lsn, (_map->start_lsn + _map->sec_count - 1))) {
      int ret;
      long int img_offset = _map->img_offset;
      
      img_offset += (lsn - _map->start_lsn) * blocksize;
      
      ret = cdio_stream_seek (_obj->gen.data_source, img_offset, 
			      SEEK_SET); 
      if (ret!=0) return ret;
      ret = cdio_stream_read (_obj->gen.data_source, _obj->sector_2336 
			      ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE)
			      : buf,
			      blocksize, 1); 
      if (ret==0) return ret;
      break;
    }
  }

  if (!node)
    cdio_warn ("reading into pre gap (lsn %lu)", (long unsigned int) lsn);

  if (mode2_form2)
    memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	    M2RAW_SECTOR_SIZE);
  else
    memcpy (data, buf + CDIO_CD_XA_SYNC_HEADER, CDIO_CD_FRAMESIZE);

  return 0;
}

/*!
   Reads nblocks of mode2 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_cdio_read_mode2_sectors (void *env, void *data, uint32_t lsn, 
		     bool mode2_form2, unsigned nblocks)
{
  _img_private_t *_obj = env;
  int i;
  int retval;

  for (i = 0; i < nblocks; i++) {
    if (mode2_form2) {
      if ( (retval = _cdio_read_mode2_sector (_obj, 
					 ((char *)data) + (M2RAW_SECTOR_SIZE * i),
					 lsn + i, true)) )
	return retval;
    } else {
      char buf[M2RAW_SECTOR_SIZE] = { 0, };
      if ( (retval = _cdio_read_mode2_sector (_obj, buf, lsn + i, true)) )
	return retval;
      
      memcpy (((char *)data) + (CDIO_CD_FRAMESIZE * i), 
	      buf + CDIO_CD_SUBHEADER_SIZE, CDIO_CD_FRAMESIZE);
    }
  }
  return 0;
}

/*!
  Set the device to use in I/O operations.
*/
static int
_cdio_set_arg (void *env, const char key[], const char value[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source"))
    {
      free (_obj->gen.source_name);

      if (!value)
	return -2;

      _obj->gen.source_name = strdup (value);
    }
  else
    return -1;

  return 0;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_cdio_get_arg (void *env, const char key[])
{
  _img_private_t *_obj = env;

  if (!strcmp (key, "source")) {
    return _obj->gen.source_name;
  } 
  return NULL;
}

/*!
  Return a string containing the default VCD device if none is specified.
 */
char *
cdio_get_default_device_nrg()
{
  return strdup(DEFAULT_CDIO_DEVICE);
}

/*!
  Return the number of of the first track. 
  CDIO_INVALID_TRACK is returned on error.
*/
static track_t
_cdio_get_first_track_num(void *env) 
{
  _img_private_t *_obj = env;
  
  return _obj->first_track_num;
}

/*!
  Return the number of tracks. We fake it an just say there's
  one big track. 
*/
static track_t
_cdio_get_num_tracks(void *env) 
{
  _img_private_t *_obj = env;

  return _obj->total_tracks;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
_cdio_get_track_format(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  
  if (track_num > _obj->total_tracks || track_num == 0) 
    return TRACK_FORMAT_ERROR;

  return _obj->tocent[track_num-1].track_format;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_cdio_get_track_green(void *env, track_t track_num) 
{
  _img_private_t *_obj = env;
  
  if (track_num > _obj->total_tracks || track_num == 0) 
    return false;

  return _obj->tocent[track_num-1].track_green;
}

/*!  
  Return the starting MSF (minutes/secs/frames) for the track number
  track_num in obj.  Tracks numbers start at 1.
  The "leadout" track is specified either by
  using track_num LEADOUT_TRACK or the total tracks+1.
*/
static bool
_cdio_get_track_msf(void *env, track_t track_num, msf_t *msf)
{
  _img_private_t *_obj = env;

  if (NULL == msf) return 1;

  if (track_num == CDIO_CDROM_LEADOUT_TRACK) track_num = _obj->total_tracks+1;

  if (track_num <= _obj->total_tracks+1 && track_num != 0) {
    *msf = _obj->tocent[track_num-1].start_msf;
    return true;
  } else 
    return false;
}

CdIo *
cdio_open_nrg (const char *source_name)
{
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs _funcs = {
    .eject_media        = cdio_generic_bogus_eject_media,
    .free               = cdio_generic_stream_free,
    .get_arg            = _cdio_get_arg,
    .get_default_device = cdio_get_default_device_nrg,
    .get_first_track_num= _cdio_get_first_track_num,
    .get_num_tracks     = _cdio_get_num_tracks,
    .get_track_format   = _cdio_get_track_format,
    .get_track_green    = _cdio_get_track_green,
    .get_track_lba      = NULL, /* Will use generic routine via msf */
    .get_track_msf      = _cdio_get_track_msf,
    .lseek              = _cdio_lseek,
    .read               = _cdio_read,
    .read_audio_sectors = _cdio_read_audio_sectors,
    .read_mode2_sector  = _cdio_read_mode2_sector,
    .read_mode2_sectors = _cdio_read_mode2_sectors,
    .set_arg            = _cdio_set_arg,
    .stat_size          = _cdio_stat_size,
  };

  _data                 = _cdio_malloc (sizeof (_img_private_t));
  _data->gen.init       = false;


  _data->total_tracks   = 0;
  _data->first_track_num= 1;
  _data->sector_2336    = false; /* FIXME: remove sector_2336 */
  _data->is_cues        = false; /* FIXME: remove is_cues. */


  _cdio_set_arg(_data, "source", (NULL == source_name) 
		? DEFAULT_CDIO_DEVICE: source_name);

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (_cdio_init(_data))
    return ret;
  else {
    cdio_generic_stream_free (_data);
    return NULL;
  }

}

bool
cdio_have_nrg (void)
{
  return true;
}
