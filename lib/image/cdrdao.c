/*
    $Id: cdrdao.c,v 1.9 2004/06/01 11:15:58 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
    toc reading routine adapted from cuetools
    Copyright (C) 2003 Svend Sanjay Sorensen <ssorensen@fastmail.fm>

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

/* This code implements low-level access functions for a CD images
   residing inside a disk file (*.bin) and its associated cue sheet.
   (*.cue).
*/

static const char _rcsid[] = "$Id: cdrdao.c,v 1.9 2004/06/01 11:15:58 rocky Exp $";

#include "cdio_assert.h"
#include "cdio_private.h"
#include "_cdio_stdio.h"

#include <cdio/logging.h>
#include <cdio/sector.h>
#include <cdio/util.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#ifdef HAVE_GLOB_H
#include <glob.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <ctype.h>

/* FIXME: should put in a common definition somewhere. */
#ifdef HAVE_MEMSET
#define BZERO(ptr, size) memset(ptr, 0, size)
#elif  HAVE_BZERO
#define BZERO(ptr, size) bzero(ptr, size)
#else 
  Error -- you need either memset or bzero
#endif

/* reader */

#define DEFAULT_CDIO_DEVICE "videocd.bin"
#define DEFAULT_CDIO_CDRDAO    "videocd.toc"

typedef struct {
  track_t        track_num;   /* Probably is index+1 */
  msf_t          start_msf;
  lba_t          start_lba;
  lba_t          length;
  lba_t          pregap;
  int            start_index;
  int            sec_count;  /* Number of sectors in this track. Does not
				include pregap */
  int            num_indices;
  int            flags;      /* "DCP", "4CH", "PRE" */
  char          *isrc;            /* IRSC Code (5.22.4) exactly 12 bytes. */
  char          *filename;        /* name given inside TOC file. */
  CdioDataSource *data_source;
  track_format_t track_format;
  bool           track_green;
  uint16_t  datasize;        /* How much is in the portion we return back? */
  uint16_t  datastart;       /* Offset from begining that data starts */
  uint16_t  endsize;         /* How much stuff at the end to skip over. This
			       stuff may have error correction (EDC, or ECC).*/
  uint16_t  blocksize;       /* total block size = start + size + end */

  
} track_info_t;

typedef struct {
  /* Things common to all drivers like this. 
     This must be first. */
  generic_img_private_t gen; 
  internal_position_t pos; 
  
  bool sector_2336;              /* Playstation (PSX) uses 2336-byte sectors */

  char         *toc_name;
  char         *mcn;             /* Media Catalog Number (5.22.3) 
				    exactly 13 bytes */
  track_info_t  tocent[100];     /* entry info for each track */
  track_t       i_tracks;    /* number of tracks in image */
  track_t       i_first_track; /* track number of first track */
  track_format_t mode;
  bool          b_have_cdrdao;
} _img_private_t;

static uint32_t _stat_size_cdrdao (void *user_data);
static bool parse_tocfile (_img_private_t *cd, const char *toc_name);

#include "image_common.h"

/* returns 0 if field is a CD-TEXT keyword, returns non-zero otherwise */
static int 
cdtext_is_keyword (char *key)
{
  const char *cdtext_keywords[] = 
    {
      "ARRANGER",     /* name(s) of the arranger(s) */
      "COMPOSER",     /* name(s) of the composer(s) */
      "DISC ID",      /* disc identification information */
      "GENRE",        /* genre identification and genre information */
      "ISRC",         /* ISRC code of each track */
      "MESSAGE",      /* message(s) from the content provider and/or artist */
      "PERFORMER",    /* name(s) of the performer(s) */
      "SIZE_INFO",    /* size information of the block */
      "SONGWRITER",   /* name(s) of the songwriter(s) */
      "TITLE",        /* title of album name or track titles */
      "TOC_INFO",     /* table of contents information */
      "TOC_INFO2"     /* second table of contents information */
      "UPC_EAN",
    };
  
  char *item;
  
  item = bsearch(key, 
		 cdtext_keywords, 12,
		 sizeof (char *), 
		 (int (*)(const void *, const void *))
		 strcmp);
  return (NULL != item) ? 0 : 1;
}

static lba_t
msf_to_lba (int minutes, int seconds, int frames)
{
  return ((minutes * CDIO_CD_SECS_PER_MIN + seconds) * CDIO_CD_FRAMES_PER_SEC 
	  + frames);
}

static lba_t
msf_lba_from_mmssff (char *s)
{
  int field;
  lba_t ret;
  char c;
  
  if (0 == strcmp (s, "0"))
    return 0;
  
  c = *s++;
  if(c >= '0' && c <= '9')
    field = (c - '0');
  else
    return CDIO_INVALID_LBA;
  while(':' != (c = *s++)) {
    if(c >= '0' && c <= '9')
      field = field * 10 + (c - '0');
    else
      return CDIO_INVALID_LBA;
  }
  
  ret = msf_to_lba (field, 0, 0);
  
  c = *s++;
  if(c >= '0' && c <= '9')
    field = (c - '0');
  else
    return CDIO_INVALID_LBA;
  if(':' != (c = *s++)) {
    if(c >= '0' && c <= '9') {
      field = field * 10 + (c - '0');
      c = *s++;
      if(c != ':')
	return CDIO_INVALID_LBA;
    }
    else
      return CDIO_INVALID_LBA;
  }
  
  if(field >= CDIO_CD_SECS_PER_MIN)
    return CDIO_INVALID_LBA;
  
  ret += msf_to_lba (0, field, 0);
  
  c = *s++;
  if (isdigit(c))
    field = (c - '0');
  else
    return -1;
  if('\0' != (c = *s++)) {
    if (isdigit(c)) {
      field = field * 10 + (c - '0');
      c = *s++;
    }
    else
      return CDIO_INVALID_LBA;
  }
  
  if('\0' != c)
    return -1;
  
  if(field >= CDIO_CD_FRAMES_PER_SEC)
    return CDIO_INVALID_LBA;
  
  ret += field;
  
  return ret;
}

/*!
  Initialize image structures.
 */
static bool
_init_cdrdao (_img_private_t *env)
{
  lsn_t lead_lsn;

  if (env->gen.init)
    return false;

  /* Have to set init before calling _stat_size_cdrdao() or we will
     get into infinite recursion calling passing right here.
   */
  env->gen.init      = true;  
  env->i_first_track = 1;
  env->mcn           = NULL;

  /* Read in TOC sheet. */
  if ( !parse_tocfile(env, env->toc_name) ) return false;
  
  lead_lsn = _stat_size_cdrdao( (_img_private_t *) env);

  if (-1 == lead_lsn) 
    return false;

  /* Fake out leadout track and sector count for last track*/
  cdio_lsn_to_msf (lead_lsn, &env->tocent[env->i_tracks].start_msf);
  env->tocent[env->i_tracks].start_lba = cdio_lsn_to_lba(lead_lsn);
  env->tocent[env->i_tracks-env->i_first_track].sec_count = 
    cdio_lsn_to_lba(lead_lsn - env->tocent[env->i_tracks-1].start_lba);

  return true;
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  Would be libc's seek() but we have to adjust for the extra track header 
  information in each sector.
*/
static off_t
_lseek_cdrdao (void *user_data, off_t offset, int whence)
{
  _img_private_t *env = user_data;

  /* real_offset is the real byte offset inside the disk image
     The number below was determined empirically. I'm guessing
     the 1st 24 bytes of a bin file are used for something.
  */
  off_t real_offset=0;

  unsigned int i;

  env->pos.lba = 0;
  for (i=0; i<env->i_tracks; i++) {
    track_info_t  *this_track=&(env->tocent[i]);
    env->pos.index = i;
    if ( (this_track->sec_count*this_track->datasize) >= offset) {
      int blocks            = offset / this_track->datasize;
      int rem               = offset % this_track->datasize;
      int block_offset      = blocks * this_track->blocksize;
      real_offset          += block_offset + rem;
      env->pos.buff_offset = rem;
      env->pos.lba        += blocks;
      break;
    }
    real_offset   += this_track->sec_count*this_track->blocksize;
    offset        -= this_track->sec_count*this_track->datasize;
    env->pos.lba += this_track->sec_count;
  }

  if (i==env->i_tracks) {
    cdio_warn ("seeking outside range of disk image");
    return -1;
  } else {
    real_offset += env->tocent[i].datastart;
    return cdio_stream_seek(env->tocent[i].data_source, real_offset, whence);
  }
}

/*!
  Reads into buf the next size bytes.
  Returns -1 on error. 
  FIXME: 
   At present we assume a read doesn't cross sector or track
   boundaries.
*/
static ssize_t
_read_cdrdao (void *user_data, void *data, size_t size)
{
  _img_private_t *env = user_data;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  char *p = data;
  ssize_t final_size=0;
  ssize_t this_size;
  track_info_t  *this_track=&(env->tocent[env->pos.index]);
  ssize_t skip_size = this_track->datastart + this_track->endsize;

  while (size > 0) {
    int rem = this_track->datasize - env->pos.buff_offset;
    if (size <= rem) {
      this_size = cdio_stream_read(this_track->data_source, buf, size, 1);
      final_size += this_size;
      memcpy (p, buf, this_size);
      break;
    }

    /* Finish off reading this sector. */
    cdio_warn ("Reading across block boundaries not finished");

    size -= rem;
    this_size = cdio_stream_read(this_track->data_source, buf, rem, 1);
    final_size += this_size;
    memcpy (p, buf, this_size);
    p += this_size;
    this_size = cdio_stream_read(this_track->data_source, buf, rem, 1);
    
    /* Skip over stuff at end of this sector and the beginning of the next.
     */
    cdio_stream_read(this_track->data_source, buf, skip_size, 1);

    /* Get ready to read another sector. */
    env->pos.buff_offset=0;
    env->pos.lba++;

    /* Have gone into next track. */
    if (env->pos.lba >= env->tocent[env->pos.index+1].start_lba) {
      env->pos.index++;
      this_track=&(env->tocent[env->pos.index]);
      skip_size = this_track->datastart + this_track->endsize;
    }
  }
  return final_size;
}

/*!
   Return the size of the CD in logical block address (LBA) units.
 */
static uint32_t 
_stat_size_cdrdao (void *user_data)
{
  _img_private_t *env = user_data;
  long size;
  int blocksize = env->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  size = cdio_stream_stat (env->tocent[0].data_source);

  if (size % blocksize)
    {
      cdio_warn ("image %s size (%ld) not multiple of blocksize (%d)", 
		 env->tocent[0].filename, size, blocksize);
      if (size % M2RAW_SECTOR_SIZE == 0)
	cdio_warn ("this may be a 2336-type disc image");
      else if (size % CDIO_CD_FRAMESIZE_RAW == 0)
	cdio_warn ("this may be a 2352-type disc image");
      /* exit (EXIT_FAILURE); */
    }

  size /= blocksize;

  return size;
}

#define MAXLINE 512
#define UNIMPLIMENTED_MSG \
  cdio_log(log_level, "%s line %d: unimplimented keyword: %s",  \
	   toc_name, line_num, keyword)


static bool
parse_tocfile (_img_private_t *cd, const char *toc_name)
{
  char line[MAXLINE];
  FILE *fp;
  unsigned int line_num=0;
  char *keyword, *field;
  int i =  -1;
  cdio_log_level_t log_level = (NULL == cd) ? CDIO_LOG_INFO : CDIO_LOG_WARN;

  if (NULL == toc_name) 
    return false;
  
  fp = fopen (toc_name, "r");
  if (fp == NULL) {
    cdio_log(log_level, "error opening %s for reading: %s", 
	     toc_name, strerror(errno));
    return false;
  }

  while ((fgets(line, MAXLINE, fp)) != NULL) {

    line_num++;

    /* strip comment from line */
    /* todo: // in quoted strings? */
    /* //comment */
    if (NULL != (field = strstr (line, "//")))
      *field = '\0';
    
    if (NULL != (keyword = strtok (line, " \t\n\r"))) {
      /* CATALOG "ddddddddddddd" */
      if (0 == strcmp ("CATALOG", keyword)) {
	if (-1 == i) {
	  if (NULL != (field = strtok (NULL, "\"\t\n\r"))) {
	    if (13 != strlen(field)) {
	      cdio_log(log_level, 
		       "%s line %d after word CATALOG:", 
		       toc_name, line_num);
	      cdio_log(log_level, 
		       "Token %s has length %ld. Should be 13 digits.", 
		       field, (long int) strlen(field));
	      
	      goto err_exit;
	    } else {
	      /* Check that we have all digits*/
	      unsigned int i;
	      for (i=0; i<13; i++) {
		if (!isdigit(field[i])) {
		    cdio_log(log_level, 
			     "%s line %d after word CATALOG:", 
			     toc_name, line_num);
		    cdio_log(log_level, 
			     "Token %s is not all digits.", field);
		    goto err_exit;
		}
	      }
	      if (NULL != cd) cd->mcn = strdup (field); 
	    }
	  } else {
	    cdio_log(log_level, 
		     "%s line %d after word CATALOG:", 
		     toc_name, line_num);
	    cdio_log(log_level, "Expecting 13 digits; nothing seen.");
	    goto err_exit;
	  }
	} else {
	  goto err_exit;
	}
	
	/* CD_DA | CD_ROM | CD_ROM_XA */
      } else if (0 == strcmp ("CD_DA", keyword)) {
	if (-1 == i) {
	  if (NULL != cd)
	    cd->mode = TRACK_FORMAT_AUDIO;
	} else {
	  goto not_in_global_section;
	}
      } else if (0 == strcmp ("CD_ROM", keyword)) {
	if (-1 == i) {
	  if (NULL != cd)
	    cd->mode = TRACK_FORMAT_DATA;
	} else {
	  goto not_in_global_section;
	}
	
      } else if (0 == strcmp ("CD_ROM_XA", keyword)) {
	if (-1 == i) {
	  if (NULL != cd)
	    cd->mode = TRACK_FORMAT_XA;
	} else {
	  goto not_in_global_section;
	}
	
	/* TRACK <track-mode> [<sub-channel-mode>] */
      } else if (0 == strcmp ("TRACK", keyword)) {
	i++;
	/* cd->tocent[i].cdtext = NULL; */
	if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	  if (0 == strcmp ("AUDIO", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_AUDIO;
	      cd->tocent[i].blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      cd->tocent[i].datasize     = CDIO_CD_FRAMESIZE_RAW;
	      cd->tocent[i].datastart    = 0;
	      cd->tocent[i].endsize      = 0;
	    }
	  } else if (0 == strcmp ("MODE1", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_DATA;
	      cd->tocent[i].blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      cd->tocent[i].datastart    = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      cd->tocent[i].datasize     = CDIO_CD_FRAMESIZE; 
	      cd->tocent[i].endsize      = CDIO_CD_EDC_SIZE 
		+ CDIO_CD_M1F1_ZERO_SIZE + CDIO_CD_ECC_SIZE;
	    }
	  } else if (0 == strcmp ("MODE1_RAW", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_DATA;
	      cd->tocent[i].blocksize = CDIO_CD_FRAMESIZE_RAW;
	      cd->tocent[i].datastart = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      cd->tocent[i].datasize  = CDIO_CD_FRAMESIZE; 
	      cd->tocent[i].endsize   = CDIO_CD_EDC_SIZE 
		+ CDIO_CD_M1F1_ZERO_SIZE + CDIO_CD_ECC_SIZE;
	    }
	  } else if (0 == strcmp ("MODE2", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_XA;
	      cd->tocent[i].datastart = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      cd->tocent[i].datasize = M2RAW_SECTOR_SIZE;
	      cd->tocent[i].endsize   = 0;
	    }
	  } else if (0 == strcmp ("MODE2_FORM1", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_XA;
	      cd->tocent[i].datastart = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE;
	      cd->tocent[i].datasize  = CDIO_CD_FRAMESIZE_RAW;  
	      cd->tocent[i].endsize   = 0;
	    }
	  } else if (0 == strcmp ("MODE2_FORM2", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_XA;
	      cd->tocent[i].datastart    = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;
	      cd->tocent[i].datasize     = CDIO_CD_FRAMESIZE;
	      cd->tocent[i].endsize      = CDIO_CD_SYNC_SIZE 
		+ CDIO_CD_ECC_SIZE;
	    }
	  } else if (0 == strcmp ("MODE2_FORM_MIX", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_XA;
	      cd->tocent[i].datasize     = M2RAW_SECTOR_SIZE;
	      cd->tocent[i].blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      cd->tocent[i].datastart    = CDIO_CD_SYNC_SIZE + 
		CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;
	      cd->tocent[i].track_green  = true;
	      cd->tocent[i].endsize      = 0;
	    }
	  } else if (0 == strcmp ("MODE2_RAW", field)) {
	    if (NULL != cd) {
	      cd->tocent[i].track_format = TRACK_FORMAT_XA;
	      cd->tocent[i].blocksize    = CDIO_CD_FRAMESIZE_RAW;
	      cd->tocent[i].datastart    = CDIO_CD_SYNC_SIZE + 
		CDIO_CD_HEADER_SIZE + CDIO_CD_SUBHEADER_SIZE;
	      cd->tocent[i].datasize     = CDIO_CD_FRAMESIZE;
	      cd->tocent[i].track_green  = true;
	      cd->tocent[i].endsize      = 0;
	    }
	  } else {
	    cdio_log(log_level, "%s line %d after TRACK:",
		     toc_name, line_num);
	    cdio_log(log_level, "'%s' not a valid mode.", field);
	    goto err_exit;
	  }
	}
	if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	  /* todo: set sub-channel-mode */
	  if (0 == strcmp ("RW", field))
	    ;
	  else if (0 == strcmp ("RW_RAW", field))
	    ;
	}
	if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	  goto format_error;
	}
	
	/* track flags */
	/* [NO] COPY | [NO] PRE_EMPHASIS */
      } else if (0 == strcmp ("NO", keyword)) {
	if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	  if (0 == strcmp ("COPY", field)) {
	    if (NULL != cd) 
	      cd->tocent[i].flags &= ~CDIO_TRACK_FLAG_COPY_PERMITTED;
	    
	  } else if (0 == strcmp ("PRE_EMPHASIS", field))
	    if (NULL != cd) {
	      cd->tocent[i].flags &= ~CDIO_TRACK_FLAG_PRE_EMPHASIS;
	      goto err_exit;
	    }
	} else {
	  goto format_error;
	}
	if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	  goto format_error;
	}
      } else if (0 == strcmp ("COPY", keyword)) {
	if (NULL != cd)
	  cd->tocent[i].flags |= CDIO_TRACK_FLAG_COPY_PERMITTED;
      } else if (0 == strcmp ("PRE_EMPHASIS", keyword)) {
	if (NULL != cd)
	  cd->tocent[i].flags |= CDIO_TRACK_FLAG_PRE_EMPHASIS;
	/* TWO_CHANNEL_AUDIO */
      } else if (0 == strcmp ("TWO_CHANNEL_AUDIO", keyword)) {
	if (NULL != cd)
	  cd->tocent[i].flags &= ~CDIO_TRACK_FLAG_FOUR_CHANNEL_AUDIO;
	/* FOUR_CHANNEL_AUDIO */
      } else if (0 == strcmp ("FOUR_CHANNEL_AUDIO", keyword)) {
	if (NULL != cd)
	  cd->tocent[i].flags |= CDIO_TRACK_FLAG_FOUR_CHANNEL_AUDIO;
	
	/* ISRC "CCOOOYYSSSSS" */
      } else if (0 == strcmp ("ISRC", keyword)) {
	if (NULL != (field = strtok (NULL, "\"\t\n\r"))) {
	  if (NULL != cd) 
	    cd->tocent[i].isrc = strdup(field);
	} else {
	  goto format_error;
	}
	
	/* SILENCE <length> */
      } else if (0 == strcmp ("SILENCE", keyword)) {
	UNIMPLIMENTED_MSG;
	
	/* ZERO <length> */
      } else if (0 == strcmp ("ZERO", keyword)) {
	UNIMPLIMENTED_MSG;
	
	/* [FILE|AUDIOFILE] "<filename>" <start> [<length>] */
      } else if (0 == strcmp ("FILE", keyword) 
		 || 0 == strcmp ("AUDIOFILE", keyword)) {
	if (0 <= i) {
	  if (NULL != (field = strtok (NULL, "\"\t\n\r"))) {
	    if (NULL != cd) {
	      cd->tocent[i].filename = strdup (field);
	      /* Todo: do something about reusing existing files. */
	      if (!(cd->tocent[i].data_source = cdio_stdio_new (field))) {
		cdio_warn ("%s line %d: can't open file `%s' for reading", 
			   toc_name, line_num, field);
		goto err_exit;
	      }
	    }
	  }
	  
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    lba_t lba = cdio_lsn_to_lba(msf_lba_from_mmssff (field));
	    if (CDIO_INVALID_LBA == lba) {
	      cdio_log(log_level, "%s line %d: invalid MSF string %s", 
		       toc_name, line_num, field);
	      goto err_exit;
	    }
	    
	    if (NULL != cd) {
	      cd->tocent[i].start_lba = lba;
	      cdio_lba_to_msf(lba, &(cd->tocent[i].start_msf));
	    }
	  }
	  if (NULL != (field = strtok (NULL, " \t\n\r")))
	    if (NULL != cd)
	      cd->tocent[i].length = msf_lba_from_mmssff (field);
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    goto format_error;
	  }
	} else {
	  goto not_in_global_section;
	}
	
	/* DATAFILE "<filename>" <start> [<length>] */
      } else if (0 == strcmp ("DATAFILE", keyword)) {
	goto unimplimented_error;
	
	/* FIFO "<fifo path>" [<length>] */
      } else if (0 == strcmp ("FIFO", keyword)) {
	goto unimplimented_error;
	
	/* START MM:SS:FF */
      } else if (0 == strcmp ("START", keyword)) {
	if (0 <= i) {
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    /* todo: line is too long! */
	    if (NULL != cd) {
	      cd->tocent[i].start_lba += msf_lba_from_mmssff (field);
	      cdio_lba_to_msf(cd->tocent[i].start_lba, 
			      &(cd->tocent[i].start_msf));
	    }
	  }
	  
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    goto format_error;
	  }
	} else {
	  goto not_in_global_section;
	}
	
	/* PREGAP MM:SS:FF */
      } else if (0 == strcmp ("PREGAP", keyword)) {
	if (0 <= i) {
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    if (NULL != cd) 
	      cd->tocent[i].pregap = msf_lba_from_mmssff (field);
	  } else {
	    goto format_error;
	  }
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    goto format_error;
	  } 
	} else {
	  goto not_in_global_section;
	}
	  
	  /* INDEX MM:SS:FF */
      } else if (0 == strcmp ("INDEX", keyword)) {
	if (0 <= i) {
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    if (NULL != cd) {
#if 0
	      if (1 == cd->tocent[i].nindex) {
		cd->tocent[i].indexes[1] = cd->tocent[i].indexes[0];
		cd->tocent[i].nindex++;
	      }
	      cd->tocent[i].indexes[cd->tocent[i].nindex++] = 
		msf_lba_from_mmssff (field) + cd->tocent[i].indexes[0];
#else 
	      ;
	      
#endif
	    }
	  } else {
	    goto format_error;
	  }
	  if (NULL != (field = strtok (NULL, " \t\n\r"))) {
	    goto format_error;
	  }
	}  else {
	  goto not_in_global_section;
	}
	  
	  /* CD_TEXT { ... } */
	  /* todo: opening { must be on same line as CD_TEXT */
      } else if (0 == strcmp ("CD_TEXT", keyword)) {
      } else if (0 == strcmp ("LANGUAGE_MAP", keyword)) {
      } else if (0 == strcmp ("LANGUAGE", keyword)) {
      } else if (0 == strcmp ("}", keyword)) {
      } else if (0 == cdtext_is_keyword (keyword)) {
	if (-1 == i) {
	  if (NULL != cd) {
#if 0
	    if (NULL == cd->cdtext)
	      cd->cdtext = cdtext_init ();
	    cdtext_set (keyword, strtok (NULL, "\"\t\n\r"), cd->cdtext);
#else 
	    ;
	    
#endif
	  }
	} else {
	  if (NULL != cd) {
#if 0
	    if (NULL == cd->tocent[i].cdtext)
	      cd->tocent[i].cdtext = cdtext_init ();
	    cdtext_set (keyword, strtok (NULL, "\"\t\n\r"), 
			cd->tocent[i].cdtext);
#else 
	    ;
	    
#endif
	  }
	}

	  /* unrecognized line */
      } else {
	cdio_log(log_level, "%s line %d: warning: unrecognized keyword: %s", 
		 toc_name, line_num, keyword);
	goto err_exit;
      }
    }
  }
    
  if (NULL != cd) cd->i_tracks = i+1;
  return true;

 unimplimented_error:
  UNIMPLIMENTED_MSG;
  goto err_exit;
  
 format_error:
  cdio_log(log_level, "%s line %d after keyword %s", 
	   toc_name, line_num, keyword);
  goto err_exit;
  
 not_in_global_section:
  cdio_log(log_level, "%s line %d: keyword %s only allowed in global section", 
	   toc_name, line_num, keyword);

 err_exit: 
  fclose (fp);
  return false;
}

/*!
   Reads a single audio sector from CD device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_audio_sectors_cdrdao (void *user_data, void *data, lsn_t lsn, 
			  unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int ret;

  /* Why the adjustment of 272, I don't know. It seems to work though */
  if (lsn != 0) {
    ret = cdio_stream_seek (env->tocent[0].data_source, 
			    (lsn * CDIO_CD_FRAMESIZE_RAW) - 272, SEEK_SET);
    if (ret!=0) return ret;

    ret = cdio_stream_read (env->tocent[0].data_source, data, 
			    CDIO_CD_FRAMESIZE_RAW, nblocks);
  } else {
    /* We need to pad out the first 272 bytes with 0's */
    BZERO(data, 272);
    
    ret = cdio_stream_seek (env->tocent[0].data_source, 0, SEEK_SET);

    if (ret!=0) return ret;

    ret = cdio_stream_read (env->tocent[0].data_source, (uint8_t *) data+272, 
			    CDIO_CD_FRAMESIZE_RAW - 272, nblocks);
  }

  /* ret is number of bytes if okay, but we need to return 0 okay. */
  return ret == 0;
}

/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_mode1_sector_cdrdao (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *env = user_data;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };
  int blocksize = env->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  ret = cdio_stream_seek (env->tocent[0].data_source, lsn * blocksize, 
			  SEEK_SET);
  if (ret!=0) return ret;

  /* FIXME: Not completely sure the below is correct. */
  ret = cdio_stream_read (env->tocent[0].data_source,
			  env->sector_2336 
			  ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE) 
			  : buf,
			  blocksize, 1);
  if (ret==0) return ret;

  memcpy (data, buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE, 
	  b_form2 ? M2RAW_SECTOR_SIZE: CDIO_CD_FRAMESIZE);

  return 0;
}

/*!
   Reads nblocks of mode1 sectors from cd device into data starting
   from lsn.
   Returns 0 if no error. 
 */
static int
_read_mode1_sectors_cdrdao (void *user_data, void *data, lsn_t lsn, 
			    bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode1_sector_cdrdao (env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

/*!
   Reads a single mode1 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
static int
_read_mode2_sector_cdrdao (void *user_data, void *data, lsn_t lsn, 
			 bool b_form2)
{
  _img_private_t *env = user_data;
  int ret;
  char buf[CDIO_CD_FRAMESIZE_RAW] = { 0, };

  /* NOTE: The logic below seems a bit wrong and convoluted
     to me, but passes the regression tests. (Perhaps it is why we get
     valgrind errors in vcdxrip). Leave it the way it was for now.
     Review this sector 2336 stuff later.
  */

  int blocksize = env->sector_2336 
    ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE_RAW;

  ret = cdio_stream_seek (env->tocent[0].data_source, lsn * blocksize, 
			  SEEK_SET);
  if (ret!=0) return ret;

  ret = cdio_stream_read (env->tocent[0].data_source,
			  env->sector_2336 
			  ? (buf + CDIO_CD_SYNC_SIZE + CDIO_CD_HEADER_SIZE) 
			  : buf,
			  blocksize, 1);
  if (ret==0) return ret;


  /* See NOTE above. */
  if (b_form2)
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
_read_mode2_sectors_cdrdao (void *user_data, void *data, lsn_t lsn, 
			    bool b_form2, unsigned int nblocks)
{
  _img_private_t *env = user_data;
  int i;
  int retval;
  unsigned int blocksize = b_form2 ? M2RAW_SECTOR_SIZE : CDIO_CD_FRAMESIZE;

  for (i = 0; i < nblocks; i++) {
    if ( (retval = _read_mode2_sector_cdrdao (env, 
					    ((char *)data) + (blocksize * i),
					    lsn + i, b_form2)) )
      return retval;
  }
  return 0;
}

#define free_if_notnull(obj) \
  if (NULL != obj) { free(obj); obj=NULL; };

static void 
_free_cdrdao (void *obj) 
{
  _img_private_t *env = obj;
  int i;

  if (NULL == env) return;
  free_if_notnull(env->mcn);
  for (i=0; i<env->i_tracks; i++) {
    if (env->tocent[i].data_source)
      cdio_stdio_destroy (env->tocent[i].data_source);
    free_if_notnull(env->tocent[i].isrc);
    free_if_notnull(env->tocent[i].filename);
  }
  free_if_notnull(env->toc_name);
  cdio_generic_stdio_free(env);
  free(env);
}

/*!
  Eject media -- there's nothing to do here except free resources.
  We always return 2.
 */
static int
_eject_media_cdrdao(void *obj)
{
  _free_cdrdao (obj);
  return 2;
}

/*!
  Set the arg "key" with "value" in the source device.
  Currently "source" to set the source device in I/O operations 
  is the only valid key.

  0 is returned if no error was found, and nonzero if there as an error.
*/
static int
_set_arg_cdrdao (void *user_data, const char key[], const char value[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "sector"))
    {
      if (!strcmp (value, "2336"))
	env->sector_2336 = true;
      else if (!strcmp (value, "2352"))
	env->sector_2336 = false;
      else
	return -2;
    }
  else if (!strcmp (key, "toc"))
    {
      free_if_notnull (env->toc_name);

      if (!value)
	return -2;

      env->toc_name = strdup (value);
    }
  else
    return -1;

  return 0;
}

/*!
  Return the value associated with the key "arg".
*/
static const char *
_get_arg_cdrdao (void *user_data, const char key[])
{
  _img_private_t *env = user_data;

  if (!strcmp (key, "source")) {
    return env->tocent[0].filename;
  } else if (!strcmp (key, "toc")) {
    return env->toc_name;
  } else if (!strcmp(key, "access-mode")) {
    return "image";
  } 
  return NULL;
}

/*!
  Return an array of strings giving possible TOC disk images.
 */
char **
cdio_get_devices_cdrdao (void)
{
  char **drives = NULL;
  unsigned int num_files=0;
#ifdef HAVE_GLOB_H
  unsigned int i;
  glob_t globbuf;
  globbuf.gl_offs = 0;
  glob("*.toc", GLOB_DOOFFS, NULL, &globbuf);
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
cdio_get_default_device_cdrdao(void)
{
  char **drives = cdio_get_devices_nrg();
  char *drive = (drives[0] == NULL) ? NULL : strdup(drives[0]);
  cdio_free_device_list(drives);
  return drive;
}

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
static cdio_drive_cap_t
_get_drive_cap_cdrdao (const void *user_data) {

  /* There may be more in the future but these we can handle now. 
     Also, we know we can't handle 
     LOCK, OPEN_TRAY, CLOSE_TRAY, SELECT_SPEED, SELECT_DISC
  */
  return CDIO_DRIVE_CAP_FILE | CDIO_DRIVE_CAP_MCN | CDIO_DRIVE_CAP_CD_AUDIO ;
}

/*!
  Return the number of tracks in the current medium.
  CDIO_INVALID_TRACK is returned on error.
*/
static track_format_t
_get_track_format_cdrdao(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  
  if (!env->gen.init) _init_cdrdao(env);

  if (i_track > env->i_tracks || i_track == 0) 
    return TRACK_FORMAT_ERROR;

  return env->tocent[i_track-env->i_first_track].track_format;
}

/*!
  Return true if we have XA data (green, mode2 form1) or
  XA data (green, mode2 form2). That is track begins:
  sync - header - subheader
  12     4      -  8
  
  FIXME: there's gotta be a better design for this and get_track_format?
*/
static bool
_get_track_green_cdrdao(void *user_data, track_t i_track) 
{
  _img_private_t *env = user_data;
  
  if (!env->gen.init) _init_cdrdao(env);

  if (i_track > env->i_tracks || i_track == 0) 
    return false;

  return env->tocent[i_track-env->i_first_track].track_green;
}

/*!  
  Return the starting LSN track number
  i_track in obj.  Track numbers start at 1.
  The "leadout" track is specified either by
  using i_track LEADOUT_TRACK or the total tracks+1.
  False is returned if there is no track entry.
*/
static lba_t
_get_lba_track_cdrdao(void *user_data, track_t i_track)
{
  _img_private_t *env = user_data;
  _init_cdrdao (env);

  if (i_track == CDIO_CDROM_LEADOUT_TRACK) i_track = env->i_tracks+1;

  if (i_track <= env->i_tracks+1 && i_track != 0) {
    return env->tocent[i_track-1].start_lba;
  } else 
    return CDIO_INVALID_LBA;
}

/*! 
  Check that a TOC file is valid. We parse the entire file.

*/
bool
cdio_is_tocfile(const char *toc_name) 
{
  int   i;
  
  if (toc_name == NULL) return false;

  i=strlen(toc_name)-strlen("toc");
  
  if (i>0) {
    if ( (toc_name[i]=='t' && toc_name[i+1]=='o' && toc_name[i+2]=='c') 
	 || (toc_name[i]=='T' && toc_name[i+1]=='O' && toc_name[i+2]=='C') ) {
      return parse_tocfile(NULL, toc_name);
    }
  }
  return false;
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_am_cdrdao (const char *psz_source_name, const char *psz_access_mode)
{
  if (psz_access_mode != NULL && strcmp(psz_access_mode, "image"))
    cdio_warn ("there is only one access mode, 'image' for cdrdao. Arg %s ignored",
	       psz_access_mode);
  return cdio_open_cdrdao(psz_source_name);
}

/*!
  Initialization routine. This is the only thing that doesn't
  get called via a function pointer. In fact *we* are the
  ones to set that up.
 */
CdIo *
cdio_open_cdrdao (const char *toc_name)
{
  CdIo *ret;
  _img_private_t *_data;

  cdio_funcs _funcs = {
    .eject_media        = _eject_media_cdrdao,
    .free               = _free_cdrdao,
    .get_arg            = _get_arg_cdrdao,
    .get_devices        = cdio_get_devices_cdrdao,
    .get_default_device = cdio_get_default_device_cdrdao,
    .get_drive_cap      = _get_drive_cap_cdrdao,
    .get_first_track_num= _get_first_track_num_image,
    .get_mcn            = _get_mcn_image,
    .get_num_tracks     = _get_num_tracks_image,
    .get_track_format   = _get_track_format_cdrdao,
    .get_track_green    = _get_track_green_cdrdao,
    .get_track_lba      = _get_lba_track_cdrdao, 
    .get_track_msf      = _get_track_msf_image,
    .lseek              = _lseek_cdrdao,
    .read               = _read_cdrdao,
    .read_audio_sectors = _read_audio_sectors_cdrdao,
    .read_mode1_sector  = _read_mode1_sector_cdrdao,
    .read_mode1_sectors = _read_mode1_sectors_cdrdao,
    .read_mode2_sector  = _read_mode2_sector_cdrdao,
    .read_mode2_sectors = _read_mode2_sectors_cdrdao,
    .set_arg            = _set_arg_cdrdao,
    .stat_size          = _stat_size_cdrdao
  };

  if (NULL == toc_name) return NULL;
  
  _data                    = _cdio_malloc (sizeof (_img_private_t));
  (_data)->gen.init        = false;
  (_data)->sector_2336     = false;
  (_data)->toc_name        = NULL;
  (_data)->gen.data_source = NULL;
  (_data)->gen.source_name = NULL;

  ret = cdio_new (_data, &_funcs);

  if (ret == NULL) return NULL;

  if (!cdio_is_tocfile(toc_name)) {
    cdio_debug ("source name %s is not recognized as a TOC file", toc_name);
    return NULL;
  }
  
  _set_arg_cdrdao (_data, "toc", toc_name);

  if (_init_cdrdao(_data)) {
    return ret;
  } else {
    _free_cdrdao(_data);
    free(ret);
    return NULL;
  }
}

bool
cdio_have_cdrdao (void)
{
  return true;
}
