/*
  $Id: rock.c,v 1.1 2005/02/13 22:03:00 rocky Exp $
 
  Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
  Adapted from GNU/Linux fs/isofs/rock.c (C) 1992, 1993 Eric Youngdale
 
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
/* Rock Ridge Extensions to iso9660 */


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif

#include <cdio/iso9660.h>
#include <cdio/logging.h>
#include <cdio/bytesex.h>

/* These functions are designed to read the system areas of a directory record
 * and extract relevant information.  There are different functions provided
 * depending upon what information we need at the time.  One function fills
 * out an inode structure, a second one extracts a filename, a third one
 * returns a symbolic link name, and a fourth one returns the extent number
 * for the file. */

#define SIG(A,B) ((A) | ((B) << 8)) /* isonum_721() */


/* This is a way of ensuring that we have something in the system
   use fields that is compatible with Rock Ridge */
#define CHECK_SP(FAIL)	       			\
      if(rr->u.SP.magic[0] != 0xbe) FAIL;	\
      if(rr->u.SP.magic[1] != 0xef) FAIL;       \
      p_stat->s_rock_offset = rr->u.SP.skip;
/* We define a series of macros because each function must do exactly the
   same thing in certain places.  We use the macros to ensure that everything
   is done correctly */

#define CONTINUE_DECLS \
  int cont_extent = 0, cont_offset = 0, cont_size = 0;   \
  void *buffer = NULL

#define CHECK_CE	       			\
      {cont_extent = from_733(*rr->u.CE.extent); \
      cont_offset = from_733(*rr->u.CE.offset); \
      cont_size = from_733(*rr->u.CE.size);}

#define SETUP_ROCK_RIDGE(DE,CHR,LEN)	      		      	\
  {								\
    LEN= sizeof(iso9660_dir_t) + DE->filename_len;		\
    if(LEN & 1) LEN++;						\
    CHR = ((unsigned char *) DE) + LEN;				\
    LEN = *((unsigned char *) DE) - LEN;			\
    if (0xff != p_stat->s_rock_offset)				\
      {								\
	LEN -= p_stat->s_rock_offset;				\
	CHR += p_stat->s_rock_offset;				\
	if (LEN<0) LEN=0;					\
      }								\
  }

/*! return length of name field; 0: not found, -1: to be ignored */
int 
get_rock_ridge_filename(iso9660_dir_t * de, /*out*/ char * retname, 
			/*out*/ iso9660_stat_t *p_stat)
{
  int len;
  unsigned char *chr;
  CONTINUE_DECLS;
  int retnamlen = 0, truncate=0;

  if (!p_stat || !p_stat->b_rock) return 0;
  *retname = 0;

  SETUP_ROCK_RIDGE(de, chr, len);
  /* repeat:*/
  {
    iso_extension_record_t * rr;
    int sig;
    
    while (len > 1){ /* There may be one byte for padding somewhere */
      rr = (iso_extension_record_t *) chr;
      if (rr->len == 0) goto out; /* Something got screwed up here */
      sig = from_721(*chr);
      chr += rr->len; 
      len -= rr->len;

      switch(sig){
      case SIG('S','P'):
	CHECK_SP(goto out);
	break;
      case SIG('C','E'):
	CHECK_CE;
	break;
      case SIG('N','M'):
	if (truncate) break;
        /*
	 * If the flags are 2 or 4, this indicates '.' or '..'.
	 * We don't want to do anything with this, because it
	 * screws up the code that calls us.  We don't really
	 * care anyways, since we can just use the non-RR
	 * name.
	 */
	if (rr->u.NM.flags & 6) {
	  break;
	}

	if (rr->u.NM.flags & ~1) {
	  cdio_info("Unsupported NM flag settings (%d)",rr->u.NM.flags);
	  break;
	}
	if((strlen(retname) + rr->len - 5) >= 254) {
	  truncate = 1;
	  break;
	}
	strncat(retname, rr->u.NM.name, rr->len - 5);
	retnamlen += rr->len - 5;
	break;
      case SIG('R','E'):
	free(buffer);
	return -1;
      default:
	break;
      }
    }
  }
  free(buffer);
  return retnamlen; /* If 0, this file did not have a NM field */
 out:
  free(buffer);
  return 0;
}

static int
parse_rock_ridge_stat_internal(iso9660_dir_t *de,
			       iso9660_stat_t *p_stat, int regard_xa)
{
  int len;
  unsigned char * chr;
  int symlink_len = 0;
  CONTINUE_DECLS;

  if (!p_stat->b_rock) return 0;

  SETUP_ROCK_RIDGE(de, chr, len);
  if (regard_xa)
    {
      chr+=14;
      len-=14;
      if (len<0) len=0;
    }
  
  /* repeat:*/
  {
    int sig;
    iso_extension_record_t * rr;
    int rootflag;
    
    while (len > 1){ /* There may be one byte for padding somewhere */
      rr = (iso_extension_record_t *) chr;
      if (rr->len == 0) goto out; /* Something got screwed up here */
      sig = from_721(*chr);
      chr += rr->len; 
      len -= rr->len;
      
      switch(sig){
      case SIG('S','P'):
	CHECK_SP(goto out);
	break;
      case SIG('C','E'):
	CHECK_CE;
	break;
      case SIG('E','R'):
	p_stat->b_rock = 1;
	cdio_debug("ISO 9660 Extensions: ");
	{ int p;
	  for(p=0;p<rr->u.ER.len_id;p++) cdio_debug("%c",rr->u.ER.data[p]);
	}
	break;
      case SIG('P','X'):
	p_stat->st_mode   = from_733(rr->u.PX.st_mode);
	p_stat->st_nlinks = from_733(rr->u.PX.st_nlinks);
	p_stat->st_uid    = from_733(rr->u.PX.st_uid);
	p_stat->st_gid    = from_733(rr->u.PX.st_gid);
	break;
#ifdef DEV_FINISHED
      case SIG('P','N'):
	{ int high, low;
	  high = from_733(*rr->u.PN.dev_high);
	  low = from_733(*rr->u.PN.dev_low);
	  /*
	   * The Rock Ridge standard specifies that if sizeof(dev_t) <= 4,
	   * then the high field is unused, and the device number is completely
	   * stored in the low field.  Some writers may ignore this subtlety,
	   * and as a result we test to see if the entire device number is
	   * stored in the low field, and use that.
	   */
	  if((low & ~0xff) && high == 0) {
	    p_stat->i_rdev = MKDEV(low >> 8, low & 0xff);
	  } else {
	    p_stat->i_rdev = MKDEV(high, low);
	  }
	}
	break;
#endif
#if TIME_FINISHED
      case SIG('T','F'): 
	{
	  int cnt = 0; /* Rock ridge never appears on a High Sierra disk */
	  /* Some RRIP writers incorrectly place ctime in the
	     ISO_ROCK_TF_CREATE field.  Try to handle this correctly for
	     either case. */
	  if(rr->u.TF.flags & ISO_ROCK_TF_CREATE) { 
	    p_stat->i_ctime.tv_sec = iso_date(rr->u.TF.times[cnt++].time, 0);
	    p_stat->i_ctime.tv_nsec = 0;
	  }
	  if(rr->u.TF.flags & ISO_ROCK_TF_MODIFY) {
	    p_stat->i_mtime.tv_sec = iso_date(rr->u.TF.times[cnt++].time, 0);
	    p_stat->i_mtime.tv_nsec = 0;
	  }
	  if(rr->u.TF.flags & ISO_ROCK_TF_ACCESS) {
	    p_stat->i_atime.tv_sec = iso_date(rr->u.TF.times[cnt++].time, 0);
	    p_stat->i_atime.tv_nsec = 0;
	  }
	  if(rr->u.TF.flags & ISO_ROCK_TF_ATTRIBUTES) { 
	    p_stat->i_ctime.tv_sec = iso_date(rr->u.TF.times[cnt++].time, 0);
	    p_stat->i_ctime.tv_nsec = 0;
	  } 
	  break;
	}
#endif /* TIME_FINISHED */
      case SIG('S','L'):
	{int slen;
	  iso_rock_sl_part_t *slp;
	  iso_rock_sl_part_t *oldslp;
	  slen = rr->len - 5;
	  slp = &rr->u.SL.link;
	  p_stat->i_size = symlink_len;
	  while (slen > 1){
	    rootflag = 0;
	    switch(slp->flags &~1){
	    case 0:
	      p_stat->i_size += slp->len;
	      break;
	    case 2:
	      p_stat->i_size += 1;
	      break;
	    case 4:
	      p_stat->i_size += 2;
	      break;
	    case 8:
	      rootflag = 1;
	      p_stat->i_size += 1;
	      break;
	    default:
	      cdio_info("Symlink component flag not implemented");
	    }
	    slen -= slp->len + 2;
	    oldslp = slp;
	    slp = (iso_rock_sl_part_t *) (((char *) slp) + slp->len + 2);
	    
	    if(slen < 2) {
	      if(    ((rr->u.SL.flags & 1) != 0) 
		     && ((oldslp->flags & 1) == 0) ) p_stat->i_size += 1;
	      break;
	    }
	    
	    /*
	     * If this component record isn't continued, then append a '/'.
	     */
	    if (!rootflag && (oldslp->flags & 1) == 0)
	      p_stat->i_size += 1;
	  }
	}
	symlink_len = p_stat->i_size;
	break;
      case SIG('R','E'):
	cdio_warn("Attempt to read p_stat for relocated directory");
	goto out;
#if FINISHED
      case SIG('C','L'): 
	{
	  iso9660_stat_t * reloc;
	  ISOFS_I(p_stat)->i_first_extent = from_733(rr->u.CL.location);
	  reloc = isofs_iget(p_stat->i_sb, p_stat->i_first_extent, 0);
	  if (!reloc)
	    goto out;
	  p_stat->st_mode   = reloc->st_mode;
	  p_stat->st_nlinks = reloc->st_nlinks;
	  p_stat->st_uid    = reloc->st_uid;
	  p_stat->st_gid    = reloc->st_gid;
	  p_stat->i_rdev   = reloc->i_rdev;
	  p_stat->i_size   = reloc->i_size;
	  p_stat->i_blocks = reloc->i_blocks;
	  p_stat->i_atime  = reloc->i_atime;
	  p_stat->i_ctime  = reloc->i_ctime;
	  p_stat->i_mtime  = reloc->i_mtime;
	  iput(reloc);
	}
	break;
#endif
      default:
	break;
      }
    }
  }
 out:
  free(buffer);
  return 0;
}

int 
parse_rock_ridge_stat(iso9660_dir_t *de, /*out*/ iso9660_stat_t *p_stat)
{
  int result;

  if (!p_stat) return 0;
  
  result = parse_rock_ridge_stat_internal(de, p_stat, 0);
  /* if Rock-Ridge flag was reset and we didn't look for attributes
   * behind eventual XA attributes, have a look there */
  if (0xFF == p_stat->s_rock_offset && p_stat->b_rock) {
    result = parse_rock_ridge_stat_internal(de, p_stat, 14);
  }
  return result;
}
