/*
    $Id: _cdio_nrg.c,v 1.34 2004/02/26 03:57:42 rocky Exp $

    Copyright (C) 2001,2003 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

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

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>
#include "cdio_assert.h"
#include "bytesex.h"
#include "ds.h"
#include "cdio_private.h"
#include "_cdio_stdio.h"

static const char _rcsid[] = "$Id: _cdio_nrg.c,v 1.34 2004/02/26 03:57:42 rocky Exp $";

/* structures used */

/* this ugly image format is typical for lazy win32 programmers... at
   least structure were set big endian, so at reverse
   engineering wasn't such a big headache... */

PRAGMA_BEGIN_PACKED
typedef struct {
  uint32_t start      GNUC_PACKED;
  uint32_t length     GNUC_PACKED;
  uint32_t type       GNUC_PACKED; /* 0x0 -> MODE1,  0x2 -> MODE2 form1,
				      0x3 -> MIXED_MODE2 2336 blocksize 
				   */
  uint32_t start_lsn  GNUC_PACKED; /* does not include any pre-gaps! */
  uint32_t _unknown   GNUC_PACKED; /* wtf is this for? -- always zero... */
} _etnf_array_t;

/* finally they realized that 32bit offsets are a bit outdated for IA64 *eg* */
typedef struct {
  uint64_t start      GNUC_PACKED;
  uint64_t length     GNUC_PACKED;
  uint32_t type       GNUC_PACKED; /* 0x0 -> MODE1,  0x2 -> MODE2 form1,
				      0x3 -> MIXED_MODE2 2336 blocksize 
				   */
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
  uint8_t  _unknown[64]  GNUC_PACKED;
} _daox_array_t;

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
#define SINF_ID  0x53494e46  /* Session information */
#define MTYP_ID  0x4d545950  /* Disc Media type? */

#define MTYP_AUDIO_CD 1 /* This isn't correct. But I don't know the
			   the right thing is and it sometimes works (and
			   sometimes is wrong). */

/* Disk track type Values gleaned from DAOX */
#define DTYP_MODE1     0 
#define DTYP_MODE2_XA  2 
#define DTYP_INVALID 255

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
  uint32_t blocksize;     /* Number of bytes in a block */
} _mapping_t;


typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  internal_position_t pos; 
  bool          is_dao;          /* True if some of disk at once. False
				    if some sort of track at once. */
  uint32_t      mtyp;            /* Value of MTYP (media type?) tag */
  uint8_t       dtyp;            /* Value of DAOX media type tag */

  /* This is a hack because I don't really understnad NERO better. */
  bool            is_cues;

  char         *mcn;             /* Media catalog number. */
  track_info_t  tocent[100];     /* entry info for each track */
  track_t       total_tracks;    /* number of tracks in image */
  track_t       first_track_num; /* track number of first track */
  CdioList     *mapping;         /* List of track information */
  uint32_t      size;
} _img_private_t;

static bool     _cdio_parse_nero_footer (_img_private_t *_obj);
static uint32_t _cdio_stat_size (void *env);

/* Updates internal track TOC, so we can later 
   simulate ioctl(CDROMREADTOCENTRY).
 */
static void
_register_mapping (_img_private_t *_obj, lsn_t start_lsn, uint32_t sec_count,
		   uint64_t img_offset, uint32_t blocksize,
		   track_format_t track_format, bool track_green)
{
  const int track_num=_obj->total_tracks;
  track_info_t  *this_track=&(_obj->tocent[_obj->total_tracks]);
  _mapping_t *_map = _cdio_malloc (sizeof (_mapping_t));

  _map->start_lsn  = start_lsn;
  _map->sec_count  = sec_count;
  _map->img_offset = img_offset;
  _map->blocksize  = blocksize;

  if (!_obj->mapping) _obj->mapping = _cdio_list_new ();
  _cdio_list_append (_obj->mapping, _map);

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
    this_track->datastart = img_offset;
  else 
    this_track->datastart = 0;

  if (track_green) 
    this_track->datastart += CDIO_CD_SUBHEADER_SIZE;
      
  this_track->sec_count = sec_count;

  this_track->track_format= track_format;
  this_track->track_green = track_green;

  switch (this_track->track_format) {
  case TRACK_FORMAT_AUDIO:
    this_track->blocksize   = CDIO_CD_FRAMESIZE_RAW;
    this_track->datasize    = CDIO_CD_FRAMESIZE_RAW;
    /*this_track->datastart   = 0;*/
    this_track->endsize     = 0;
    break;
  case TRACK_FORMAT_CDI:
    this_track->datasize=CDIO_CD_FRAMESIZE;
    break;
  case TRACK_FORMAT_XA:
    if (track_green) {
      this_track->blocksize = CDIO_CD_FRAMESIZE;
      /*this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE;*/
      this_track->datasize  = M2RAW_SECTOR_SIZE;
      this_track->endsize   = 0;
    } else {
      /*this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE +
	CDIO_CD_SUBHEADER_SIZE;*/
      this_track->datasize  = CDIO_CD_FRAMESIZE;
      this_track->endsize   = CDIO_CD_SYNC_SIZE + CDIO_CD_ECC_SIZE;
    }
    break;
  case TRACK_FORMAT_DATA:
    if (track_green) {
      /*this_track->datastart = CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE;*/
      this_track->datasize  = CDIO_CD_FRAMESIZE;
      this_track->endsize   = CDIO_CD_EDC_SIZE + CDIO_CD_M1F1_ZERO_SIZE 
	  + CDIO_CD_ECC_SIZE;
    } else {
      /* Is the below correct? */
      /*this_track->datastart = 0;*/
      this_track->datasize  = CDIO_CD_FRAMESIZE;
      this_track->endsize   = 0;  
    }
    break;
  default:
    /*this_track->datasize=CDIO_CD_FRAMESIZE_RAW;*/
    cdio_warn ("track %d has unknown format %d",
	       _obj->total_tracks, this_track->track_format);
  }
  
  _obj->total_tracks++;

  cdio_debug ("start lsn: %lu sector count: %0lu -> %8ld (%08lx)", 
	      (long unsigned int) start_lsn, 
	      (long unsigned int) sec_count, 
	      (long unsigned int) img_offset,
	      (long unsigned int) img_offset);
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

  if (_obj->size) return true;

  size = cdio_stream_stat (_obj->gen.data_source);
  if (-1 == size) return false;

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
	cdio_warn ("Image not recognized as either v50 or v55 type NRG");
	return false;
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
      uint32_t opcode = UINT32_FROM_BE (chunk->id);
      
      bool break_out = false;
      
      switch (opcode) {

      case CUES_ID: /* "CUES" Seems to have sector size 2336 and 150 sector
		       pregap seems to be included at beginning of image.
		       */
      case CUEX_ID: /* "CUEX" */ 
	{
	  unsigned entries = UINT32_FROM_BE (chunk->len);
	  _cuex_array_t *_entries = (void *) chunk->data;
	  
	  cdio_assert (_obj->mapping == NULL);
	  
	  cdio_assert ( sizeof (_cuex_array_t) == 8 );
	  cdio_assert ( UINT32_FROM_BE (chunk->len) % sizeof(_cuex_array_t) 
			== 0 );
	  
	  entries /= sizeof (_cuex_array_t);
	  
	  if (CUES_ID == opcode) {
	    lsn_t lsn = UINT32_FROM_BE (_entries[0].lsn);
	    int idx;
	    
	    cdio_info ("CUES type image detected" );
	    /*cdio_assert (lsn == 0?);*/
	    
	    _obj->is_cues         = true; /* HACK alert. */
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
				 M2RAW_SECTOR_SIZE, TRACK_FORMAT_XA, true);
	    }
	  } else {
	    lsn_t lsn = UINT32_FROM_BE (_entries[0].lsn);
	    int idx;
	    
	    cdio_info ("CUEX type image detected");
	    cdio_assert (lsn == 0xffffff6a);
	    
	    for (idx = 2; idx < entries; idx += 2) {
	      lsn_t sec_count;
	      
	      cdio_assert (_entries[idx].index == 1);
	      cdio_assert (_entries[idx].track != _entries[idx + 1].track);
	      
	      lsn       = UINT32_FROM_BE (_entries[idx].lsn);
	      sec_count = UINT32_FROM_BE (_entries[idx + 1].lsn);
	      
	      _register_mapping (_obj, lsn, sec_count - lsn, 
				 (lsn + CDIO_PREGAP_SECTORS)*M2RAW_SECTOR_SIZE,
				 M2RAW_SECTOR_SIZE, TRACK_FORMAT_XA, true);
	    }
	  }
	  break;
	}
	
      case DAOI_ID: /* "DAOI" */
      case DAOX_ID: /* "DAOX" */ 
	{
	  _daox_array_t *_entries = (void *) chunk->data;
	  track_format_t track_format;
	  int form     = _entries->_unknown[18];
	  _obj->dtyp   = _entries->_unknown[36];
	  _obj->is_dao = true;
	  cdio_debug ("DAO%c tag detected, track format %d, form %x\n", 
		      opcode==DAOX_ID ? 'X': 'I', _obj->dtyp, form);
	  switch (_obj->dtyp) {
	  case 0:
	    track_format = TRACK_FORMAT_DATA;
	    break;
	  case 0x6:
	  case 0x20:
	    track_format = TRACK_FORMAT_XA;
	    break;
	  case 0x7:
	    track_format = TRACK_FORMAT_AUDIO;
	    break;
	  default:
	    cdio_warn ("Unknown track format %x\n", 
		      _obj->dtyp);
	    track_format = TRACK_FORMAT_AUDIO;
	  }
	  if (0 == form) {
	    int i;
	    for (i=0; i<_obj->total_tracks; i++) {
	      _obj->tocent[i].track_format= track_format;
	      _obj->tocent[i].datastart   = 0;
	      _obj->tocent[i].track_green = false;
	      if (TRACK_FORMAT_AUDIO == track_format) {
		_obj->tocent[i].blocksize   = CDIO_CD_FRAMESIZE_RAW;
		_obj->tocent[i].datasize    = CDIO_CD_FRAMESIZE_RAW;
		_obj->tocent[i].endsize     = 0;
	      } else {
		_obj->tocent[i].datasize    = CDIO_CD_FRAMESIZE;
		_obj->tocent[i].datastart  =  0;
	      }
	    }
	  } else if (2 == form) {
	    int i;
	    for (i=0; i<_obj->total_tracks; i++) {
	      _obj->tocent[i].track_green = true;
	      _obj->tocent[i].track_format= track_format;
	      _obj->tocent[i].datasize    = CDIO_CD_FRAMESIZE;
	      if (TRACK_FORMAT_XA == track_format) {
		_obj->tocent[i].datastart   = CDIO_CD_SYNC_SIZE 
		  + CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;
		_obj->tocent[i].endsize     = CDIO_CD_SYNC_SIZE 
		  + CDIO_CD_ECC_SIZE;
	      } else {
		_obj->tocent[i].datastart   = CDIO_CD_SYNC_SIZE 
		  + CDIO_CD_HEADER_SIZE;
		_obj->tocent[i].endsize     = CDIO_CD_EDC_SIZE 
		  + CDIO_CD_M1F1_ZERO_SIZE + CDIO_CD_ECC_SIZE;
	      
	      }
	    }
	  }
	  break;
	}
      case NERO_ID: /* "NER0" */
      case NER5_ID: /* "NER5" */
	cdio_error ("unexpected nrg magic ID NER%c detected",
		    opcode==NERO_ID ? 'O': '5');
	free(footer_buf);
	return false;
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
	
	{
	  int idx;
	  for (idx = 0; idx < entries; idx++) {
	    uint32_t _len = UINT32_FROM_BE (_entries[idx].length);
	    uint32_t _start = UINT32_FROM_BE (_entries[idx].start_lsn);
	    uint32_t _start2 = UINT32_FROM_BE (_entries[idx].start);
	    uint32_t track_mode= uint32_from_be (_entries[idx].type);
	    bool     track_green = true;
	    track_format_t track_format = TRACK_FORMAT_XA;
	    uint16_t  blocksize;     
	    
	    switch (track_mode) {
	    case 0:
	      track_format = TRACK_FORMAT_DATA;
	      track_green  = false; /* ?? */
	      blocksize    = CDIO_CD_FRAMESIZE;
	      break;
	    case 2:
	      track_format = TRACK_FORMAT_XA;
	      track_green  = false; /* ?? */
	      blocksize    = CDIO_CD_FRAMESIZE;
	      break;
	    case 3:
	      track_format = TRACK_FORMAT_XA;
	      track_green  = true;
	      blocksize    = M2RAW_SECTOR_SIZE;
	      break;
	    case 7:
	      track_format = TRACK_FORMAT_AUDIO;
	      track_green  = false;
	      blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      break;
	    default:
	      cdio_warn ("Don't know how to handle track mode (%lu)?",
			 (long unsigned int) track_mode);
	      free(footer_buf);
	      return false;
	    }
	    
	    cdio_assert (_len % blocksize == 0);
	    
	    _len /= blocksize;
	    
	    cdio_assert (_start * blocksize == _start2);
	    
	    _start += idx * CDIO_PREGAP_SECTORS;
	    _register_mapping (_obj, _start, _len, _start2, blocksize,
			       track_format, track_green);

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

	{
	  int idx;
	  for (idx = 0; idx < entries; idx++) {
	    uint32_t _len = uint64_from_be (_entries[idx].length);
	    uint32_t _start = uint32_from_be (_entries[idx].start_lsn);
	    uint32_t _start2 = uint64_from_be (_entries[idx].start);
	    uint32_t track_mode= uint32_from_be (_entries[idx].type);
	    bool     track_green = true;
	    track_format_t track_format = TRACK_FORMAT_XA;
	    uint16_t  blocksize;     


	    switch (track_mode) {
	    case 0:
	      track_format = TRACK_FORMAT_DATA;
	      track_green  = false; /* ?? */
	      blocksize    = CDIO_CD_FRAMESIZE;
	      break;
	    case 2:
	      track_format = TRACK_FORMAT_XA;
	      track_green  = false; /* ?? */
	      blocksize    = CDIO_CD_FRAMESIZE;
	      break;
	    case 3:
	      track_format = TRACK_FORMAT_XA;
	      track_green  = true;
	      blocksize    = M2RAW_SECTOR_SIZE;
	      break;
	    case 7:
	      track_format = TRACK_FORMAT_AUDIO;
	      track_green  = false;
	      blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      break;
	    default:
	      cdio_warn ("Don't know how to handle track mode (%lu)?",
			 (long unsigned int) track_mode);
	      free(footer_buf);
	      return false;
	    }
	    
	    if (_len % blocksize != 0) {
	      cdio_warn ("length is not a multiple of blocksize " 
			 "len %lu, size %d, rem %lu", 
			 (long unsigned int) _len, blocksize, 
			 (long unsigned int) _len % blocksize);
	      if (0 == _len % CDIO_CD_FRAMESIZE) {
		cdio_warn("Adjusting blocksize to %d", CDIO_CD_FRAMESIZE);
		blocksize = CDIO_CD_FRAMESIZE;
	      } else if (0 == _len % M2RAW_SECTOR_SIZE) {
		cdio_warn("Adjusting blocksize to %d", M2RAW_SECTOR_SIZE);
		blocksize = M2RAW_SECTOR_SIZE;
	      } else if (0 == _len % CDIO_CD_FRAMESIZE_RAW) {
		cdio_warn("Adjusting blocksize to %d", CDIO_CD_FRAMESIZE_RAW);
		blocksize = CDIO_CD_FRAMESIZE_RAW;
	      }
	    }
	    
	    _len /= blocksize;
	    
	    if (_start * blocksize != _start2) {
	      cdio_warn ("%lu * %d != %lu", 
			 (long unsigned int) _start, blocksize, 
			 (long unsigned int) _start2);
	      if (_start * CDIO_CD_FRAMESIZE == _start2) {
		cdio_warn("Adjusting blocksize to %d", CDIO_CD_FRAMESIZE);
		blocksize = CDIO_CD_FRAMESIZE;
	      } else if (_start * M2RAW_SECTOR_SIZE == _start2) {
		cdio_warn("Adjusting blocksize to %d", M2RAW_SECTOR_SIZE);
		blocksize = M2RAW_SECTOR_SIZE;
	      } else if (0 == _start * CDIO_CD_FRAMESIZE_RAW == _start2) {
		cdio_warn("Adjusting blocksize to %d", CDIO_CD_FRAMESIZE_RAW);
		blocksize = CDIO_CD_FRAMESIZE_RAW;
	      }
	    }
	    
	    _start += idx * CDIO_PREGAP_SECTORS;
	    _register_mapping (_obj, _start, _len, _start2, blocksize,
			       track_format, track_green);
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
	
      case MTYP_ID: { /* "MTYP" */
	uint32_t *mtyp_p = (void *) chunk->data;
	uint32_t mtyp  = UINT32_FROM_BE (*mtyp_p);
	
	cdio_assert (UINT32_FROM_BE (chunk->len) == 4);

	cdio_debug ("MTYP: %lu", 
		    (long unsigned int) UINT32_FROM_BE (*mtyp_p));

	if (mtyp != MTYP_AUDIO_CD) {
	  cdio_warn ("Unknown MTYP value: %u", (unsigned int) mtyp);
	}
	_obj->mtyp = mtyp;
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

  free(footer_buf);
  return true;
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
    cdio_warn ("init failed");
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
     The number below was determined empirically. 
  */
  off_t real_offset= _obj->is_dao ? 0x4b000 : 0;

  unsigned int i;

  for (i=0; i<_obj->total_tracks; i++) {
    track_info_t  *this_track=&(_obj->tocent[i]);
    _obj->pos.index = i;
    if ( (this_track->sec_count*this_track->datasize) >= offset) {
      int blocks            = offset / this_track->datasize;
      int rem               = offset % this_track->datasize;
      int block_offset      = blocks * this_track->blocksize;
      real_offset          += block_offset + rem;
      _obj->pos.buff_offset = rem;
      _obj->pos.lba        += blocks;
      break;
    }
    real_offset   += this_track->sec_count*this_track->blocksize;
    offset        -= this_track->sec_count*this_track->datasize;
    _obj->pos.lba += this_track->sec_count;
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
      
      img_offset += (lsn - _map->start_lsn) * _map->blocksize;
      
      ret = cdio_stream_seek (_obj->gen.data_source, img_offset, 
			      SEEK_SET); 
      if (ret!=0) return ret;
      ret = cdio_stream_read (_obj->gen.data_source, 
			      (M2RAW_SECTOR_SIZE == _map->blocksize)
			      ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE)
			      : buf,
			      _map->blocksize, 1); 
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

/*
  Free memory resources associated with NRG object.
*/
static void 
_cdio_nrg_destroy (void *obj) 
{
  _img_private_t *env = obj;

  if (NULL == env) return;
  if (NULL != env->mapping)
    _cdio_list_free (env->mapping, true); 
  cdio_generic_stdio_free(env);
}

/*
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
  Return an array of strings giving possible NRG disk images.
 */
char **
cdio_get_devices_nrg (void)
{
  char **drives = NULL;
  unsigned int num_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  glob("*.nrg", GLOB_DOOFFS, NULL, &globbuf);
  for (i=0; i<globbuf.gl_pathc; i++) {
    cdio_add_device_list(&drives, globbuf.gl_pathv[i], &num_files);
  }
  globfree(&globbuf);
#else
  cdio_add_device_list(&drives, DEFAULT_CDIO_DEVICE, &num_files);
#endif /*HAVE_GLOB_H*/
  cdio_add_device_list(&drives, NULL, &num_files);
  return drives;
}

/*!
  Return a string containing the default CD device.
 */
char *
cdio_get_default_device_nrg(void)
{
  char **drives = cdio_get_devices_nrg();
  char *drive = (drives[0] == NULL) ? NULL : strdup(drives[0]);
  cdio_free_device_list(drives);
  return drive;
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

  if ( _obj->dtyp != DTYP_INVALID) {
    switch (_obj->dtyp) {
    case DTYP_MODE2_XA:
      return TRACK_FORMAT_XA;
    case DTYP_MODE1:
      return TRACK_FORMAT_DATA;
    default: ;
    }
  }
    
  /*if ( MTYP_AUDIO_CD == _obj->mtyp) return TRACK_FORMAT_AUDIO; */
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

  if ( MTYP_AUDIO_CD == _obj->mtyp) return false;
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
    .free               = _cdio_nrg_destroy,
    .get_arg            = _cdio_get_arg,
    .get_devices        = cdio_get_devices_nrg,
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
  _data->mtyp           = 0; 
  _data->dtyp           = DTYP_INVALID; 
  _data->first_track_num= 1;
  _data->is_dao         = false; 
  _data->is_cues        = false; /* FIXME: remove is_cues. */


  _cdio_set_arg(_data, "source", (NULL == source_name) 
		? DEFAULT_CDIO_DEVICE: source_name);

  ret = cdio_new (_data, &_funcs);
  if (ret == NULL) return NULL;

  if (_cdio_init(_data))
    return ret;
  else {
    cdio_generic_stdio_free (_data);
    return NULL;
  }

}

bool
cdio_have_nrg (void)
{
  return true;
}
