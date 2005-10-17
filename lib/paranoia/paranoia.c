/*
  $Id: paranoia.c,v 1.19 2005/10/17 15:31:08 pjcreath Exp $

  Copyright (C) 2004, 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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
/***
 * Toplevel file for the paranoia abstraction over the cdda lib 
 *
 ***/

/* immediate todo:: */
/* Allow disabling of root fixups? */ 
/* Dupe bytes are creeping into cases that require greater overlap
   than a single fragment can provide.  We need to check against a
   larger area* (+/-32 sectors of root?) to better eliminate
   dupes. Of course this leads to other problems... Is it actually a
   practically solvable problem? */
/* Bimodal overlap distributions break us. */
/* scratch detection/tolerance not implemented yet */

/***************************************************************

  Da new shtick: verification now a two-step assymetric process.
  
  A single 'verified/reconstructed' data segment cache, and then the
  multiple fragment cache 

  verify a newly read block against previous blocks; do it only this
  once. We maintain a list of 'verified sections' from these matches.

  We then glom these verified areas into a new data buffer.
  Defragmentation fixups are allowed here alone.

  We also now track where read boundaries actually happened; do not
  verify across matching boundaries.

  **************************************************************/

/***************************************************************

  Silence.  "It's BAAAAAAaaack."

  audio is now treated as great continents of values floating on a
  mantle of molten silence.  Silence is not handled by basic
  verification at all; we simply anchor sections of nonzero audio to a
  position and fill in everything else as silence.  We also note the
  audio that interfaces with silence; an edge must be 'wet'.

  **************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <unistd.h>
#include <stdio.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include <math.h>
#include <cdio/cdda.h>
#include "../cdda_interface/smallft.h"
#include "p_block.h"
#include <cdio/paranoia.h>
#include "overlap.h"
#include "gap.h"
#include "isort.h"

const char *paranoia_cb_mode2str[] = {
  "read",
  "verify",
  "fixup edge",
  "fixup atom",
  "scratch",
  "repair",
  "skip",
  "drift",
  "backoff",
  "overlap",
  "fixup dropped",
  "fixup duplicated",
  "read error"
};

static inline long 
re(root_block *root)
{
  if (!root)return(-1);
  if (!root->vector)return(-1);
  return(ce(root->vector));
}

static inline long 
rb(root_block *root)
{
  if (!root)return(-1);
  if (!root->vector)return(-1);
  return(cb(root->vector));
}

static inline 
long rs(root_block *root)
{
  if (!root)return(-1);
  if (!root->vector)return(-1);
  return(cs(root->vector));
}

static inline int16_t *
rv(root_block *root){
  if (!root)return(NULL);
  if (!root->vector)return(NULL);
  return(cv(root->vector));
}

#define rc(r) (r->vector)

/** The below variable isn't used. It's here to make it possible to
    refer to the names/values in the enumeration (as appears in code
    and which are really handled by #define's below) in expressions
    when debugging.
*/
enum  {
  FLAGS_EDGE    =0x1, /**< first/last N words of frame */
  FLAGS_UNREAD  =0x2, /**< unread, hence missing and unmatchable */
  FLAGS_VERIFIED=0x4  /**< block read and verified */
} paranoia_read_flags;

/* Please make sure the below #defines match the above enum values */
#define FLAGS_EDGE     0x01 
#define FLAGS_UNREAD   0x02 
#define FLAGS_VERIFIED 0x04


/**** matching and analysis code *****************************************/

/* ===========================================================================
 * i_paranoia_overlap() (internal)
 *
 * This function is called when buffA[offsetA] == buffB[offsetB].  This
 * function searches backward and forward to see how many consecutive
 * samples also match.
 *
 * This function is called by do_const_sync() when we're not doing any
 * verification.  Its more complicated sibling is i_paranoia_overlap2.
 *
 * This function returns the number of consecutive matching samples.
 * If (ret_begin) or (ret_end) are not NULL, it fills them with the
 * offsets of the first and last matching samples in A.
 */
static inline long 
i_paranoia_overlap(int16_t *buffA,int16_t *buffB,
		   long offsetA, long offsetB,
		   long sizeA,long sizeB,
		   long *ret_begin, long *ret_end)
{
  long beginA=offsetA,endA=offsetA;
  long beginB=offsetB,endB=offsetB;

  /* Scan backward to extend the matching run in that direction. */
  for(; beginA>=0 && beginB>=0; beginA--,beginB--)
    if (buffA[beginA] != buffB[beginB]) break;
  beginA++;
  beginB++;

  /* Scan forward to extend the matching run in that direction. */
  for(; endA<sizeA && endB<sizeB; endA++,endB++)
    if (buffA[endA] != buffB[endB]) break;

  /* Return the result of our search. */
  if (ret_begin) *ret_begin = beginA;
  if (ret_end) *ret_end = endA;
  return (endA-beginA);
}


/* ===========================================================================
 * i_paranoia_overlap2() (internal)
 *
 * This function is called when buffA[offsetA] == buffB[offsetB].  This
 * function searches backward and forward to see how many consecutive
 * samples also match.
 *
 * This function is called by do_const_sync() when we're verifying the
 * data coming off the CD.  Its less complicated sibling is
 * i_paranoia_overlap, which is a good place to look to see the simplest
 * outline of how this function works.
 *
 * This function returns the number of consecutive matching samples.
 * If (ret_begin) or (ret_end) are not NULL, it fills them with the
 * offsets of the first and last matching samples in A.
 */
static inline long 
i_paranoia_overlap2(int16_t *buffA,int16_t *buffB,
		    unsigned char *flagsA, unsigned char *flagsB,
		    long offsetA, long offsetB,
		    long sizeA,long sizeB,
		    long *ret_begin, long *ret_end)
{
  long beginA=offsetA, endA=offsetA;
  long beginB=offsetB, endB=offsetB;
  
  /* Scan backward to extend the matching run in that direction. */
  for (; beginA>=0 && beginB>=0; beginA--,beginB--) {
    if (buffA[beginA] != buffB[beginB]) break;

    /* don't allow matching across matching sector boundaries */
    /* Stop if both samples were at the edges of a low-level read.
     * ???: What implications does this have?
     * ???: Why do we include the first sample for which this is true?
     */
    if ((flagsA[beginA]&flagsB[beginB]&FLAGS_EDGE)) {
      beginA--;
      beginB--;
      break;
    }

    /* don't allow matching through known missing data */
    if ((flagsA[beginA]&FLAGS_UNREAD) || (flagsB[beginB]&FLAGS_UNREAD))
      break;
  }
  beginA++;
  beginB++;
  
  /* Scan forward to extend the matching run in that direction. */
  for (; endA<sizeA && endB<sizeB; endA++,endB++) {
    if (buffA[endA] != buffB[endB]) break;

    /* don't allow matching across matching sector boundaries */
    /* Stop if both samples were at the edges of a low-level read.
     * ???: What implications does this have?
     * ???: Why do we not stop if endA == beginA?
     */
    if ((flagsA[endA]&flagsB[endB]&FLAGS_EDGE) && endA!=beginA){
      break;
    }

    /* don't allow matching through known missing data */
    if ((flagsA[endA]&FLAGS_UNREAD) || (flagsB[endB]&FLAGS_UNREAD))
      break;
  }

  /* Return the result of our search. */
  if (ret_begin) *ret_begin = beginA;
  if (ret_end) *ret_end = endA;
  return (endA-beginA);
}


/* ===========================================================================
 * do_const_sync() (internal)
 *
 * This function is called when samples A[posA] == B[posB].  It tries to
 * build a matching run from that point, looking forward and backward to
 * see how many consecutive samples match.  Since the starting samples
 * might only be coincidentally identical, we only consider the run to
 * be a true match if it's longer than MIN_WORDS_SEARCH.
 *
 * This function returns the length of the run if a matching run was found,
 * or 0 otherwise.  If a matching run was found, (begin) and (end) are set
 * to the absolute positions of the beginning and ending samples of the
 * run in A, and (offset) is set to the jitter between the c_blocks.
 * (I.e., offset indicates the distance between what A considers sample N
 * on the CD and what B considers sample N.)
 */
static inline long int 
do_const_sync(c_block_t *A,
	      sort_info_t *B, unsigned char *flagB,
	      long posA, long posB,
	      long *begin, long *end, long *offset)
{
  unsigned char *flagA=A->flags;
  long ret=0;

  /* If we're doing any verification whatsoever, we have flags and will
   * take them into account.  Otherwise, we just do the simple equality
   * test for samples on both sides of the initial match.
   */
  if (flagB==NULL)
    ret=i_paranoia_overlap(cv(A), iv(B), posA, posB,
			   cs(A), is(B), begin, end);
  else
    if ((flagB[posB]&FLAGS_UNREAD)==0)
      ret=i_paranoia_overlap2(cv(A), iv(B), flagA, flagB, 
			      posA, posB, cs(A), is(B),
			      begin, end);
	
  /* Small matching runs could just be coincidental.  We only consider this
   * a real match if it's long enough.
   */
  if (ret > MIN_WORDS_SEARCH) {
    *offset=+(posA+cb(A))-(posB+ib(B));

    /* ???: Contrary to the original comment, this appears to be relative to
     * A, not B.
     */
    *begin+=cb(A);
    *end+=cb(A);
    return(ret);
  }
  
  return(0);
}


/* ===========================================================================
 * try_sort_sync() (internal)
 *
 * Starting from the sample in B with the absolute position (post), look
 * for a matching run in A.  This search will look in A for a first
 * matching sample within (p->dynoverlap) samples around (post).  If it
 * finds one, it will then determine how many consecutive samples match
 * both A and B from that point, looking backwards and forwards.  If
 * this search produces a matching run longer than MIN_WORDS_SEARCH, we
 * consider it a match.
 *
 * When used by stage 1, the "post" is planted with respect to the old
 * c_block being compare to the new c_block.  In stage 2, the "post" is
 * planted with respect to the verified root.
 *
 * This function returns 1 if a match is found and 0 if not.  When a match
 * is found, (begin) and (end) are set to the boundaries of the run, and
 * (offset) is set to the difference in position of the run in A and B.
 * (begin) and (end) are the absolute positions of the samples in
 * A.  (offset) counts from B's frame of reference.  I.e., an offset of
 * -2 would mean that A's absolute 3 is equivalent to B's 5.
 */

/* post is w.r.t. B.  in stage one, we post from old.  In stage 2 we
   post from root. Begin, end, offset count from B's frame of
   reference */

static inline long int 
try_sort_sync(cdrom_paranoia_t *p,
	      sort_info_t *A, unsigned char *Aflags,
	      c_block_t *B,
	      long int post,
	      long int *begin,
	      long int *end,
	      long *offset,
	      void (*callback)(long int, paranoia_cb_mode_t))
{
  
  long int dynoverlap=p->dynoverlap;
  sort_link_t *ptr=NULL;
  unsigned char *Bflags=B->flags;

  /* block flag matches 0x02 (unmatchable) */
  if (Bflags==NULL || (Bflags[post-cb(B)]&FLAGS_UNREAD)==0){
    /* always try absolute offset zero first! */
    {
      long zeropos=post-ib(A);
      if (zeropos>=0 && zeropos<is(A)) {

	/* Before we bother with the search for a matching samples,
	 * we check the simple case.  If there's no jitter at all
	 * (i.e. the absolute positions of A's and B's samples are
	 * consistent), A's sample at (post) should be identical
	 * to B's sample at the same position.
	 */
	if ( cv(B)[post-cb(B)] == iv(A)[zeropos] ) {

	  /* The first sample matched, now try to grow the matching run
	   * in both directions.  We only consider it a match if more
	   * than MIN_WORDS_SEARCH consecutive samples match.
	   */
	  if (do_const_sync(B, A, Aflags,
			    post-cb(B), zeropos,
			    begin, end, offset) ) {

	    /* ???: To be studied. */
	    offset_add_value(p,&(p->stage1),*offset,callback);
	    
	    return(1);
	  }
	}
      }
    }
  } else
    return(0);

  /* If the samples with the same absolute position didn't match, it's
   * either a bad sample, or the two c_blocks are jittered with respect
   * to each other.  Now we search through A for samples that do have
   * the same value as B's post.  The search looks from first to last
   * occurrence witin (dynoverlap) samples of (post).
   */
  ptr=sort_getmatch(A,post-ib(A),dynoverlap,cv(B)[post-cb(B)]);
  
  while (ptr){

    /* We've found a matching sample, so try to grow the matching run in
     * both directions.  If we find a long enough run (longer than
     * MIN_WORDS_SEARCH), we've found a match.
     */
    if (do_const_sync(B,A,Aflags,
		     post-cb(B),ipos(A,ptr),
		     begin,end,offset)){
      /* ???: To be studied. */
      offset_add_value(p,&(p->stage1),*offset,callback);
      return(1);
    }

    /* The matching sample was just a fluke -- there weren't enough adjacent
     * samples that matched to consider a matching run.  So now we check
     * for the next occurrence of that value in A.
     */
    ptr=sort_nextmatch(A,ptr);
  }

  /* We didn't find any matches. */
  *begin=-1;
  *end=-1;
  *offset=-1;
  return(0);
}


/* ===========================================================================
 * STAGE 1 MATCHING
 *
 * ???: Insert high-level explanation here.
 * ===========================================================================
 */

/* Top level of the first stage matcher */

/* We match each analysis point of new to the preexisting blocks
recursively.  We can also optionally maintain a list of fragments of
the preexisting block that didn't match anything, and match them back
afterward. */

#define OVERLAP_ADJ (MIN_WORDS_OVERLAP/2-1)


/* ===========================================================================
 * stage1_matched() (internal)
 *
 * This function is called whenever stage 1 verification finds two identical
 * runs of samples from different reads.  The runs must be more than
 * MIN_WORDS_SEARCH samples long.  They may be jittered (i.e. their absolute
 * positions on the CD may not match due to inaccurate seeking) with respect
 * to each other, but they have been verified to have no dropped samples
 * within them.
 *
 * This function provides feedback via the callback mechanism and marks the
 * runs as verified.  The details of the marking are somehwat subtle and
 * are described near the relevant code.
 *
 * Subsequent portions of the stage 1 code will build a verified fragment
 * from this run.  The verified fragment will eventually be merged
 * into the verified root (and its absolute position determined) in
 * stage 2.
 */
static inline void 
stage1_matched(c_block_t *old, c_block_t *new,
	       long matchbegin,long matchend,
	       long matchoffset,
	       void (*callback)(long int, paranoia_cb_mode_t))
{
  long i;
  long oldadjbegin=matchbegin-cb(old);
  long oldadjend=matchend-cb(old);
  long newadjbegin=matchbegin-matchoffset-cb(new);
  long newadjend=matchend-matchoffset-cb(new);


  /* Provide feedback via the callback about the samples we've just
   * verified.
   *
   * ???: How can matchbegin ever be < cb(old)?
   *
   * ???: Why do edge samples get logged only when there's jitter
   * between the matched runs (matchoffset != 0)?
   */
  if ( matchbegin-matchoffset<=cb(new)
       || matchbegin<=cb(old)
       || (new->flags[newadjbegin]&FLAGS_EDGE) 
       || (old->flags[oldadjbegin]&FLAGS_EDGE) ) {
    if ( matchoffset && callback )
	(*callback)(matchbegin,PARANOIA_CB_FIXUP_EDGE);
  } else
    if (callback)
      (*callback)(matchbegin,PARANOIA_CB_FIXUP_ATOM);
  
  if ( matchend-matchoffset>=ce(new) ||
       (new->flags[newadjend]&FLAGS_EDGE) ||
       matchend>=ce(old) ||
       (old->flags[oldadjend]&FLAGS_EDGE) ) {
    if ( matchoffset && callback )
      (*callback)(matchend,PARANOIA_CB_FIXUP_EDGE);
  } else
    if (callback) 
      (*callback)(matchend, PARANOIA_CB_FIXUP_ATOM);


  /* Mark verified samples as "verified," but trim the verified region
   * by OVERLAP_ADJ samples on each side.  There are several significant
   * implications of this trimming:
   *
   * 1) Why we trim at all:  We have to trim to distinguish between two
   * adjacent verified runs and one long verified run.  We encounter this
   * situation when samples have been dropped:
   *
   *   matched portion of read 1 ....)(.... matched portion of read 1
   *       read 2 adjacent run  .....)(..... read 2 adjacent run
   *                                 ||
   *                      dropped samples in read 2
   *
   * So at this point, the fact that we have two adjacent runs means
   * that we have not yet verified that the two runs really are adjacent.
   * (In fact, just the opposite:  there are two runs because they were
   * matched by separate runs, indicating that some samples didn't match
   * across the length of read 2.)
   *
   * If we verify that they are actually adjacent (e.g. if the two runs
   * are simply a result of matching runs from different reads, not from
   * dropped samples), we will indeed mark them as one long merged run.
   *
   * 2) Why we trim by this amount: We want to ensure that when we
   * verify the relationship between these two runs, we do so with
   * an overlapping fragment at least OVERLAP samples long.  Following
   * from the above example:
   *
   *                (..... matched portion of read 3 .....)
   *       read 2 adjacent run  .....)(..... read 2 adjacent run
   *
   * Assuming there were no dropped samples between the adjacent runs,
   * the matching portion of read 3 will need to be at least OVERLAP
   * samples long to mark the two runs as one long verified run.
   * If there were dropped samples, read 3 wouldn't match across the
   * two runs, proving our caution worthwhile.
   *
   * 3) Why we partially discard the work we've done:  We don't.
   * When subsequently creating verified fragments from this run,
   * we compensate for this trimming.  Thus the verified fragment will
   * contain the full length of verified samples.  Only the c_blocks
   * will reflect this trimming.
   *
   * ???: The comment below indicates that the sort cache is updated in
   * some way, but this does not appear to be the case.
   */

  /* Mark the verification flags.  Don't mark the first or
     last OVERLAP/2 elements so that overlapping fragments
     have to overlap by OVERLAP to actually merge. We also
     remove elements from the sort such that later sorts do
     not have to sift through already matched data */

  newadjbegin+=OVERLAP_ADJ;
  newadjend-=OVERLAP_ADJ;
  for(i=newadjbegin;i<newadjend;i++)
    new->flags[i]|=FLAGS_VERIFIED; /* mark verified */

  oldadjbegin+=OVERLAP_ADJ;
  oldadjend-=OVERLAP_ADJ;
  for(i=oldadjbegin;i<oldadjend;i++)
    old->flags[i]|=FLAGS_VERIFIED; /* mark verified */
    
}


/* ===========================================================================
 * i_iterate_stage1 (internal)
 *
 * This function is called by i_stage1() to compare newly read samples with
 * previously read samples, searching for contiguous runs of identical
 * samples.  Matching runs indicate that at least two reads of the CD
 * returned identical data, with no dropped samples in that run.
 * The runs may be jittered (i.e. their absolute positions on the CD may
 * not be accurate due to inaccurate seeking) at this point.  Their
 * positions will be determined in stage 2.
 *
 * This function compares the new c_block (which has been indexed in
 * p->sortcache) to a previous c_block.  It is called for each previous
 * c_block.  It searches for runs of identical samples longer than
 * MIN_WORDS_SEARCH.  Samples in matched runs are marked as verified.
 *
 * Subsequent stage 1 code builds verified fragments from the runs of
 * verified samples.  These fragments are merged into the verified root
 * in stage 2.
 *
 * This function returns the number of distinct runs verified in the new
 * c_block when compared against this old c_block.
 */
static long int 
i_iterate_stage1(cdrom_paranoia_t *p, c_block_t *old, c_block_t *new,
		 void(*callback)(long int, paranoia_cb_mode_t)) 
{
  long matchbegin = -1;
  long matchend   = -1;
  long matchoffset;

  /* ???: Why do we limit our search only to the samples with overlapping
   * absolute positions?  It could be because it eliminates some further
   * bounds checking.
   *
   * Why do we "no longer try to spread the ... search" as mentioned below?
   */
  /* we no longer try to spread the stage one search area by dynoverlap */
  long searchend   = min(ce(old), ce(new));
  long searchbegin = max(cb(old), cb(new));
  long searchsize  = searchend-searchbegin;
  sort_info_t *i = p->sortcache;
  long ret = 0;
  long int j;

  long tried = 0;
  long matched = 0;

  if (searchsize<=0)
    return(0);

  /* match return values are in terms of the new vector, not old */

  /* ???: Why 23?  */

  for (j=searchbegin; j<searchend; j+=23) {

    /* Skip past any samples verified in previous comparisons to
     * other old c_blocks.  Also, obviously, don't bother verifying
     * unread/unmatchable samples.
     */
    if ((new->flags[j-cb(new)] & (FLAGS_VERIFIED|FLAGS_UNREAD)) == 0) {
      tried++;

      /* Starting from the sample in the old c_block with the absolute
       * position j, look for a matching run in the new c_block.  This
       * search will look a certain distance around j, and if successful
       * will extend the matching run as far backward and forward as
       * it can.
       *
       * The search will only return 1 if it finds a matching run long
       * enough to be deemed significant.
       */
      if (try_sort_sync(p, i, new->flags, old, j,
			&matchbegin, &matchend, &matchoffset,
			callback) == 1) {

	matched+=matchend-matchbegin;

	/* purely cosmetic: if we're matching zeros, don't use the
           callback because they will appear to be all skewed */
	{
	  long j = matchbegin-cb(old);
	  long end = matchend-cb(old);
	  for (; j<end; j++) if (cv(old)[j]!=0) break;

	  /* Mark the matched samples in both c_blocks as verified.
	   * In reality, not all the samples are marked.  See
	   * stage1_matched() for details.
	   */
	  if (j<end) {
	    stage1_matched(old,new,matchbegin,matchend,matchoffset,callback);
	  } else {
	    stage1_matched(old,new,matchbegin,matchend,matchoffset,NULL);
	  }
	}
	ret++;

	/* Skip past this verified run to look for more matches. */
	if (matchend-1 > j)
	  j = matchend-1;
      }
    }
  } /* end for */

#ifdef NOISY 
  fprintf(stderr,"iterate_stage1: search area=%ld[%ld-%ld] tried=%ld matched=%ld spans=%ld\n",
	  searchsize,searchbegin,searchend,tried,matched,ret);
#endif

  return(ret);
}


/* ===========================================================================
 * i_stage1() (internal)
 *
 * Compare newly read samples against previously read samples, searching
 * for contiguous runs of identical samples.  Matching runs indicate that
 * at least two reads of the CD returned identical data, with no dropped
 * samples in that run.  The runs may be jittered (i.e. their absolute
 * positions on the CD may not be accurate due to inaccurate seeking) at
 * this point.  Their positions will be determined in stage 2.
 *
 * This function compares a new c_block against all other c_blocks in memory,
 * searching for sufficiently long runs of identical samples.  Since each
 * c_block represents a separate call to read_c_block, this ensures that
 * multiple reads have returned identical data.  (Additionally, read_c_block
 * varies the reads so that multiple reads are unlikely to produce identical
 * errors, so any matches between reads are considered verified.  See
 * i_read_c_block for more details.)
 *
 * Each time we find such a  run (longer than MIN_WORDS_SEARCH), we mark
 * the samples as "verified" in both c_blocks.  Runs of verified samples in
 * the new c_block are promoted into verified fragments, which will later
 * be merged into the verified root in stage 2.
 *
 * In reality, not all the verified samples are marked as "verified."
 * See stage1_matched() for an explanation.
 *
 * This function returns the number of verified fragments created by the
 * stage 1 matching.
 */
static long int
i_stage1(cdrom_paranoia_t *p, c_block_t *p_new, 
	 void (*callback)(long int, paranoia_cb_mode_t))
{
  long size=cs(p_new);
  c_block_t *ptr=c_last(p);
  int ret=0;
  long int begin=0;
  long int end;
  
  /* We're going to be comparing the new c_block against the other
   * c_blocks in memory.  Initialize the "sort cache" index to allow
   * for fast searching through the new c_block.  (The index will
   * actually be built the first time we search.)
   */
  if (ptr) 
    sort_setup( p->sortcache, cv(p_new), &cb(p_new), cs(p_new), cb(p_new), 
		ce(p_new) );

  /* Iterate from oldest to newest c_block, comparing the new c_block
   * to each, looking for a sufficiently long run of identical samples
   * (longer than MIN_WORDS_SEARCH), which will be marked as "verified"
   * in both c_blocks.
   *
   * Since the new c_block is already in the list (at the head), don't
   * compare it against itself.
   */
  while ( ptr && ptr != p_new ) {
    if (callback)
      (*callback)(cb(p_new), PARANOIA_CB_VERIFY);
    i_iterate_stage1(p,ptr,p_new,callback);

    ptr=c_prev(ptr);
  }

  /* parse the verified areas of p_new into v_fragments */

  /* Find each run of contiguous verified samples in the new c_block
   * and create a verified fragment from each run.
   */
  begin=0;
  while (begin<size) {
    for ( ; begin < size; begin++)
      if (p_new->flags[begin]&FLAGS_VERIFIED) break;
    for (end=begin; end < size; end++)
      if ((p_new->flags[end]&FLAGS_VERIFIED)==0) break;
    if (begin>=size) break;
    
    ret++;

    /* We create a new verified fragment from the contiguous run
     * of verified samples.
     *
     * We expand the "verified" range by OVERLAP_ADJ on each side
     * to compensate for trimming done to the verified range by
     * stage1_matched().  The samples were actually verified, and
     * hence belong in the verified fragment.  See stage1_matched()
     * for an explanation of the trimming.
     */
    new_v_fragment(p,p_new,cb(p_new)+max(0,begin-OVERLAP_ADJ),
		   cb(p_new)+min(size,end+OVERLAP_ADJ),
		   (end+OVERLAP_ADJ>=size && p_new->lastsector));

    begin=end;
  }

  /* Return the number of distinct verified fragments we found with
   * stage 1 matching.
   */
  return(ret);
}


/* ===========================================================================
 * STAGE 2 MATCHING
 *
 * ???: Insert high-level explanation here.
 * ===========================================================================
 */

typedef struct sync_result {
  long offset;
  long begin;
  long end;
} sync_result_t;

/* Reconcile v_fragments to root buffer.  Free if matched, fragment/fixup root
   if necessary.

   Do *not* match using zero posts
*/

static long int 
i_iterate_stage2(cdrom_paranoia_t *p,
		 v_fragment_t *v,
		 sync_result_t *r,
		 void(*callback)(long int, paranoia_cb_mode_t))
{
  root_block *root=&(p->root);
  long matchbegin=-1,matchend=-1,offset;
  long fbv,fev;
  
#ifdef NOISY
      fprintf(stderr,"Stage 2 search: fbv=%ld fev=%ld\n",fb(v),fe(v));
#endif

  if (min(fe(v) + p->dynoverlap,re(root)) -
    max(fb(v) - p->dynoverlap,rb(root)) <= 0) 
    return(0);

  if (callback)
    (*callback)(fb(v), PARANOIA_CB_VERIFY);

  /* just a bit of v; determine the correct area */
  fbv = max(fb(v), rb(root)-p->dynoverlap);

  /* we want to avoid zeroes */
  while (fbv<fe(v) && fv(v)[fbv-fb(v)]==0)
    fbv++;
  if (fbv == fe(v))
    return(0);
  fev = min(min(fbv+256, re(root)+p->dynoverlap), fe(v));
  
  {
    /* spread the search area a bit.  We post from root, so containment
       must strictly adhere to root */
    long searchend=min(fev+p->dynoverlap,re(root));
    long searchbegin=max(fbv-p->dynoverlap,rb(root));
    sort_info_t *i=p->sortcache;
    long j;
    
    sort_setup(i, fv(v), &fb(v), fs(v), fbv, fev);
    for(j=searchbegin; j<searchend; j+=23){
      while (j<searchend && rv(root)[j-rb(root)]==0)j++;
      if (j==searchend) break;

      if (try_sort_sync(p, i, NULL, rc(root), j,
			&matchbegin,&matchend,&offset,callback)){
	
	r->begin=matchbegin;
	r->end=matchend;
	r->offset=-offset;
	if (offset)if (callback)(*callback)(r->begin,PARANOIA_CB_FIXUP_EDGE);
	return(1);
      }
    }
  }
  
  return(0);
}

/* simple test for a root vector that ends in silence*/
static void 
i_silence_test(root_block *root)
{
  int16_t *vec=rv(root);
  long end=re(root)-rb(root)-1;
  long j;
  
  for(j=end-1;j>=0;j--)
    if (vec[j]!=0) break;
  if (j<0 || end-j>MIN_SILENCE_BOUNDARY) {
    if (j<0)j=0;
    root->silenceflag=1;
    root->silencebegin=rb(root)+j;
    if (root->silencebegin<root->returnedlimit)
      root->silencebegin=root->returnedlimit;
  }
}

/* match into silence vectors at offset zero if at all possible.  This
   also must be called with vectors in ascending begin order in case
   there are nonzero islands */
static long int 
i_silence_match(root_block *root, v_fragment_t *v, 
		void(*callback)(long int, paranoia_cb_mode_t))
{

  cdrom_paranoia_t *p=v->p;
  int16_t *vec=fv(v);
  long end=fs(v),begin;
  long j;

  /* does this vector begin wet? */
  if (end<MIN_SILENCE_BOUNDARY) return(0);
  for(j=0;j<end;j++)
    if (vec[j]!=0) break;
  if (j<MIN_SILENCE_BOUNDARY) return(0);
  j+=fb(v);

  /* is the new silent section ahead of the end of the old by <
     p->dynoverlap? */
  if (fb(v)>=re(root) && fb(v)-p->dynoverlap<re(root)){
    /* extend the zeroed area of root */
    long addto   = fb(v) + MIN_SILENCE_BOUNDARY - re(root);
    int16_t *vec = calloc(addto, sizeof(int16_t));
    c_append(rc(root), vec, addto);
    free(vec);
  }

  /* do we have an 'effortless' overlap? */
  begin = max(fb(v),root->silencebegin);
  end = min(j,re(root));
  
  if (begin<end){

    /* don't use it unless it will extend... */

    if (fe(v)>re(root)){
      long int voff = begin-fb(v);
      
      c_remove(rc(root),begin-rb(root),-1);
      c_append(rc(root),vec+voff,fs(v)-voff);
    }
    offset_add_value(p,&p->stage2,0,callback);

  } else {
    if (j<begin){
      /* OK, we'll have to force it a bit as the root is jittered
         forward */
      long voff = j - fb(v);

      /* don't use it unless it will extend... */
      if (begin+fs(v)-voff>re(root)) {
	c_remove(rc(root),root->silencebegin-rb(root),-1);
	c_append(rc(root),vec+voff,fs(v)-voff);
      }
      offset_add_value(p,&p->stage2,end-begin,callback);
    } else
      return(0);
  }

  /* test the new root vector for ending in silence */
  root->silenceflag = 0;
  i_silence_test(root);

  if (v->lastsector) root->lastsector=1;
  free_v_fragment(v);
  return(1);
}

static long int 
i_stage2_each(root_block *root, v_fragment_t *v,
	      void(*callback)(long int, paranoia_cb_mode_t))
{

  cdrom_paranoia_t *p=v->p;
  long dynoverlap=p->dynoverlap/2*2;
  
  if (!v || !v->one) return(0);

  if (!rv(root)){
    return(0);
  } else {
    sync_result_t r;

    if (i_iterate_stage2(p,v,&r,callback)){

      long int begin=r.begin-rb(root);
      long int end=r.end-rb(root);
      long int offset=r.begin+r.offset-fb(v)-begin;
      long int temp;
      c_block_t *l=NULL;

      /* we have a match! We don't rematch off rift, we chase the
	 match all the way to both extremes doing rift analysis. */

#ifdef NOISY
      fprintf(stderr,"Stage 2 match\n");
#endif

      /* chase backward */
      /* note that we don't extend back right now, only forward. */
      while ((begin+offset>0 && begin>0)){
	long matchA=0,matchB=0,matchC=0;
	long beginL=begin+offset;

	if (l==NULL){
	  int16_t *buff=malloc(fs(v)*sizeof(int16_t));
	  l=c_alloc(buff,fb(v),fs(v));
	  memcpy(buff,fv(v),fs(v)*sizeof(int16_t));
	}

	i_analyze_rift_r(rv(root),cv(l),
			 rs(root),cs(l),
			 begin-1,beginL-1,
			 &matchA,&matchB,&matchC);
	
#ifdef NOISY
	fprintf(stderr,"matching rootR: matchA:%ld matchB:%ld matchC:%ld\n",
		matchA,matchB,matchC);
#endif		
	
	if (matchA){
	  /* a problem with root */
	  if (matchA>0){
	    /* dropped bytes; add back from v */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DROPPED);
	    if (rb(root)+begin<p->root.returnedlimit)
	      break;
	    else{
	      c_insert(rc(root),begin,cv(l)+beginL-matchA,
		       matchA);
	      offset-=matchA;
	      begin+=matchA;
	      end+=matchA;
	    }
	  } else {
	    /* duplicate bytes; drop from root */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DUPED);
	    if (rb(root)+begin+matchA<p->root.returnedlimit) 
	      break;
	    else{
	      c_remove(rc(root),begin+matchA,-matchA);
	      offset-=matchA;
	      begin+=matchA;
	      end+=matchA;
	    }
	  }
	} else if (matchB){
	  /* a problem with the fragment */
	  if (matchB>0){
	    /* dropped bytes */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DROPPED);
	    c_insert(l,beginL,rv(root)+begin-matchB,
			 matchB);
	    offset+=matchB;
	  } else {
	    /* duplicate bytes */
	    if (callback)
	      (*callback)(begin+rb(root)-1,PARANOIA_CB_FIXUP_DUPED);
	    c_remove(l,beginL+matchB,-matchB);
	    offset+=matchB;
	  }
	} else if (matchC){
	  /* Uhh... problem with both */
	  
	  /* Set 'disagree' flags in root */
	  if (rb(root)+begin-matchC<p->root.returnedlimit)
	    break;
	  c_overwrite(rc(root),begin-matchC,
			cv(l)+beginL-matchC,matchC);
	  
	} else {
	  /* do we have a mismatch due to silence beginning/end case? */
	  /* in the 'chase back' case, we don't do anything. */

	  /* Did not determine nature of difficulty... 
	     report and bail */
	    
	  /*RRR(*callback)(post,PARANOIA_CB_XXX);*/
	  break;
	}
	/* not the most efficient way, but it will do for now */
	beginL=begin+offset;
	i_paranoia_overlap(rv(root),cv(l),
			   begin,beginL,
			   rs(root),cs(l),
			   &begin,&end);	
      }
      
      /* chase forward */
      temp=l ? cs(l) : fs(v);
      while (end+offset<temp && end<rs(root)){
	long matchA=0,matchB=0,matchC=0;
	long beginL=begin+offset;
	long endL=end+offset;
	
	if (l==NULL){
	  int16_t *buff=malloc(fs(v)*sizeof(int16_t));
	  l=c_alloc(buff,fb(v),fs(v));
	  memcpy(buff,fv(v),fs(v)*sizeof(int16_t));
	}

	i_analyze_rift_f(rv(root),cv(l),
			 rs(root),cs(l),
			 end,endL,
			 &matchA,&matchB,&matchC);
	
#ifdef NOISY	
	fprintf(stderr,"matching rootF: matchA:%ld matchB:%ld matchC:%ld\n",
		matchA,matchB,matchC);
#endif
	
	if (matchA){
	  /* a problem with root */
	  if (matchA>0){
	    /* dropped bytes; add back from v */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DROPPED);
	    if (end+rb(root)<p->root.returnedlimit)
	      break;
	    c_insert(rc(root),end,cv(l)+endL,matchA);
	  } else {
	    /* duplicate bytes; drop from root */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DUPED);
	    if (end+rb(root)<p->root.returnedlimit)
	      break;
	    c_remove(rc(root),end,-matchA);
	  }
	} else if (matchB){
	  /* a problem with the fragment */
	  if (matchB>0){
	    /* dropped bytes */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DROPPED);
	    c_insert(l,endL,rv(root)+end,matchB);
	  } else {
	    /* duplicate bytes */
	    if (callback)(*callback)(end+rb(root),PARANOIA_CB_FIXUP_DUPED);
	    c_remove(l,endL,-matchB);
	  }
	} else if (matchC){
	  /* Uhh... problem with both */
	  
	  /* Set 'disagree' flags in root */
	  if (end+rb(root)<p->root.returnedlimit)
	    break;
	  c_overwrite(rc(root),end,cv(l)+endL,matchC);
	} else {
	  analyze_rift_silence_f(rv(root),cv(l),
				 rs(root),cs(l),
				 end,endL,
				 &matchA,&matchB);
	  if (matchA){
	    /* silence in root */
	    /* Can only do this if we haven't already returned data */
	    if (end+rb(root)>=p->root.returnedlimit){
	      c_remove(rc(root),end,-1);
	    }

	  } else if (matchB){
	    /* silence in fragment; lose it */
	    
	    if (l)i_cblock_destructor(l);
	    free_v_fragment(v);
	    return(1);

	  } else {
	    /* Could not determine nature of difficulty... 
	       report and bail */
	    
	    /*RRR(*callback)(post,PARANOIA_CB_XXX);*/
	  }
	  break;
	}
	/* not the most efficient way, but it will do for now */
	i_paranoia_overlap(rv(root),cv(l),
			   begin,beginL,
			   rs(root),cs(l),
			   NULL,&end);
      }

      /* if this extends our range, let's glom */
      {
	long sizeA=rs(root);
	long sizeB;
	long vecbegin;
	int16_t *vector;
	  
	if (l){
	  sizeB=cs(l);
	  vector=cv(l);
	  vecbegin=cb(l);
	} else {
	  sizeB=fs(v);
	  vector=fv(v);
	  vecbegin=fb(v);
	}

	if (sizeB-offset>sizeA || v->lastsector){	  
	  if (v->lastsector){
	    root->lastsector=1;
	  }
	  
	  if (end<sizeA)c_remove(rc(root),end,-1);
	  
	  if (sizeB-offset-end)c_append(rc(root),vector+end+offset,
					 sizeB-offset-end);
	  
	  i_silence_test(root);

	  /* add offset into dynoverlap stats */
	  offset_add_value(p,&p->stage2,offset+vecbegin-rb(root),callback);
	}
      }
      if (l)i_cblock_destructor(l);
      free_v_fragment(v);
      return(1);
      
    } else {
      /* D'oh.  No match.  What to do with the fragment? */
      if (fe(v)+dynoverlap<re(root) && !root->silenceflag){
	/* It *should* have matched.  No good; free it. */
	free_v_fragment(v);
      }
      /* otherwise, we likely want this for an upcoming match */
      /* we don't free the sort info (if it was collected) */
      return(0);
      
    }
  }
}

static int 
i_init_root(root_block *root, v_fragment_t *v,long int begin,
		       void(*callback)(long int, paranoia_cb_mode_t))
{
  if (fb(v)<=begin && fe(v)>begin){
    
    root->lastsector=v->lastsector;
    root->returnedlimit=begin;

    if (rv(root)){
      i_cblock_destructor(rc(root));
      rc(root)=NULL;
    }

    {
      int16_t *buff=malloc(fs(v)*sizeof(int16_t));
      memcpy(buff,fv(v),fs(v)*sizeof(int16_t));
      root->vector=c_alloc(buff,fb(v),fs(v));
    }    

    i_silence_test(root);

    return(1);
  } else
    return(0);
}

static int 
vsort(const void *a,const void *b)
{
  return((*(v_fragment_t **)a)->begin-(*(v_fragment_t **)b)->begin);
}

static int 
i_stage2(cdrom_paranoia_t *p, long int beginword, long int endword,
	 void (*callback)(long int, paranoia_cb_mode_t))
{

  int flag=1,ret=0;
  root_block *root=&(p->root);

#ifdef NOISY
  fprintf(stderr,"Fragments:%ld\n",p->fragments->active);
  fflush(stderr);
#endif

  /* even when the 'silence flag' is lit, we try to do non-silence
     matching in the event that there are still audio vectors with
     content to be sunk before the silence */

  while (flag) {
    /* loop through all the current fragments */
    v_fragment_t *first=v_first(p);
    long active=p->fragments->active,count=0;
    v_fragment_t **list = calloc(active, sizeof(v_fragment_t *));

    while (first){
      v_fragment_t *next=v_next(first);
      list[count++]=first;
      first=next;
    }

    flag=0;
    if (count){
      /* sorted in ascending order of beginning */
      qsort(list,active,sizeof(v_fragment_t *),&vsort);
      
      /* we try a nonzero based match even if in silent mode in
	 the case that there are still cached vectors to sink
	 behind continent->ocean boundary */

      for(count=0;count<active;count++){
	first=list[count];
	if (first->one){
	  if (rv(root)==NULL){
	    if (i_init_root(&(p->root),first,beginword,callback)){
	      free_v_fragment(first);
	      flag=1;
	      ret++;
	    }
	  } else {
	    if (i_stage2_each(root,first,callback)){
	      ret++;
	      flag=1;
	    }
	  }
	}
      }

      /* silence handling */
      if (!flag && p->root.silenceflag){
	for(count=0;count<active;count++){
	  first=list[count];
	  if (first->one){
	    if (rv(root)!=NULL){
	      if (i_silence_match(root,first,callback)){
		ret++;
		flag=1;
	      }
	    }
	  }
	}
      }
    }
    free(list);
  }
  return(ret);
}

static void 
i_end_case(cdrom_paranoia_t *p,long endword, 
	   void(*callback)(long int, paranoia_cb_mode_t))
{

  root_block *root=&p->root;

  /* have an 'end' flag; if we've just read in the last sector in a
     session, set the flag.  If we verify to the end of a fragment
     which has the end flag set, we're done (set a done flag).  Pad
     zeroes to the end of the read */
  
  if (root->lastsector==0)return;
  if (endword<re(root))return;
  
  {
    long addto=endword-re(root);
    char *temp=calloc(addto,sizeof(char)*2);

    c_append(rc(root),(void *)temp,addto);
    free(temp);

    /* trash da cache */
    paranoia_resetcache(p);

  }
}

/* We want to add a sector. Look through the caches for something that
   spans.  Also look at the flags on the c_block... if this is an
   obliterated sector, get a bit of a chunk past the obliteration. */

/* Not terribly smart right now, actually.  We can probably find
   *some* match with a cache block somewhere.  Take it and continue it
   through the skip */

static void 
verify_skip_case(cdrom_paranoia_t *p,
		 void(*callback)(long int, paranoia_cb_mode_t))
{

  root_block *root=&(p->root);
  c_block_t *graft=NULL;
  int vflag=0;
  int gend=0;
  long post;
  
#ifdef NOISY
	fprintf(stderr,"\nskipping\n");
#endif

  if (rv(root)==NULL){
    post=0;
  } else {
    post=re(root);
  }
  if (post==-1)post=0;

  if (callback)(*callback)(post,PARANOIA_CB_SKIP);
  
  /* We want to add a sector.  Look for a c_block that spans,
     preferrably a verified area */

  {
    c_block_t *c=c_first(p);
    while (c){
      long cbegin=cb(c);
      long cend=ce(c);
      if (cbegin<=post && cend>post){
	long vend=post;

	if (c->flags[post-cbegin]&FLAGS_VERIFIED){
	  /* verified area! */
	  while (vend<cend && (c->flags[vend-cbegin]&FLAGS_VERIFIED))vend++;
	  if (!vflag || vend>vflag){
	    graft=c;
	    gend=vend;
	  }
	  vflag=1;
	} else {
	  /* not a verified area */
	  if (!vflag){
	    while (vend<cend && (c->flags[vend-cbegin]&FLAGS_VERIFIED)==0)vend++;
	    if (graft==NULL || gend>vend){
	      /* smallest unverified area */
	      graft=c;
	      gend=vend;
	    }
	  }
	}
      }
      c=c_next(c);
    }

    if (graft){
      long cbegin=cb(graft);
      long cend=ce(graft);

      while (gend<cend && (graft->flags[gend-cbegin]&FLAGS_VERIFIED))gend++;
      gend=min(gend+OVERLAP_ADJ,cend);

      if (rv(root)==NULL){
	int16_t *buff=malloc(cs(graft));
	memcpy(buff,cv(graft),cs(graft));
	rc(root)=c_alloc(buff,cb(graft),cs(graft));
      } else {
	c_append(rc(root),cv(graft)+post-cbegin,
		 gend-post);
      }

      root->returnedlimit=re(root);
      return;
    }
  }

  /* No?  Fine.  Great.  Write in some zeroes :-P */
  {
    void *temp=calloc(CDIO_CD_FRAMESIZE_RAW,sizeof(int16_t));

    if (rv(root)==NULL){
      rc(root)=c_alloc(temp,post,CDIO_CD_FRAMESIZE_RAW);
    } else {
      c_append(rc(root),temp,CDIO_CD_FRAMESIZE_RAW);
      free(temp);
    }
    root->returnedlimit=re(root);
  }
}    

/**** toplevel ****************************************/

void 
paranoia_free(cdrom_paranoia_t *p)
{
  paranoia_resetall(p);
  sort_free(p->sortcache);
  free_list(p->cache, 1);
  free_list(p->fragments, 1);
  free(p);
}

void 
paranoia_modeset(cdrom_paranoia_t *p, int enable)
{
  p->enable=enable;
}

lsn_t
paranoia_seek(cdrom_paranoia_t *p, off_t seek, int mode)
{
  long sector;
  long ret;
  switch(mode){
  case SEEK_SET:
    sector=seek;
    break;
  case SEEK_END:
    sector=cdda_disc_lastsector(p->d)+seek;
    break;
  default:
    sector=p->cursor+seek;
    break;
  }
  
  if (cdda_sector_gettrack(p->d,sector)==-1)return(-1);

  i_cblock_destructor(p->root.vector);
  p->root.vector=NULL;
  p->root.lastsector=0;
  p->root.returnedlimit=0;

  ret=p->cursor;
  p->cursor=sector;

  i_paranoia_firstlast(p);
  
  /* Evil hack to fix pregap patch for NEC drives! To be rooted out in a10 */
  p->current_firstsector=sector;

  return(ret);
}



/* ===========================================================================
 * read_c_block() (internal)
 *
 * This funtion reads many (p->readahead) sectors, encompassing at least
 * the requested words.
 *
 * It returns a c_block which encapsulates these sectors' data and sector
 * number.  The sectors come come from multiple low-level read requests.
 *
 * This function reads many sectors in order to exhaust any caching on the
 * drive itself, as caching would simply return the same incorrect data
 * over and over.  Paranoia depends on truly re-reading portions of the
 * disc to make sure the reads are accurate and correct any inaccuracies.
 *
 * Which precise sectors are read varies ("jiggles") between calls to
 * read_c_block, to prevent consistent errors across multiple reads
 * from being misinterpreted as correct data.
 *
 * The size of each low-level read is determined by the underlying driver
 * (p->d->nsectors), which allows the driver to specify how many sectors
 * can be read in a single request.  Historically, the Linux kernel could
 * only read 8 sectors at a time, with likely dropped samples between each
 * read request.  Other operating systems may have different limitations.
 *
 * This function is called by paranoia_read_limited(), which breaks the
 * c_block of read data into runs of samples that are likely to be
 * contiguous, verifies them and stores them in verified fragments, and
 * eventually merges the fragments into the verified root.
 *
 * This function returns the last c_block read or NULL on error.
 */

static c_block_t *
i_read_c_block(cdrom_paranoia_t *p,long beginword,long endword,
	       void(*callback)(long, paranoia_cb_mode_t))
{

/* why do it this way?  We need to read lots of sectors to kludge
   around stupid read ahead buffers on cheap drives, as well as avoid
   expensive back-seeking. We also want to 'jiggle' the start address
   to try to break borderline drives more noticeably (and make broken
   drives with unaddressable sectors behave more often). */
      
  long readat,firstread;
  long totaltoread=p->readahead;
  long sectatonce=p->d->nsectors;
  long driftcomp=(float)p->dyndrift/CD_FRAMEWORDS+.5;
  c_block_t *new=NULL;
  root_block *root=&p->root;
  int16_t *buffer=NULL;
  unsigned char *flags=NULL;
  long sofar;
  long dynoverlap=(p->dynoverlap+CD_FRAMEWORDS-1)/CD_FRAMEWORDS; 
  long anyflag=0;


  /* Calculate the first sector to read.  This calculation takes
   * into account the need to jitter the starting point of the read
   * to reveal consistent errors as well as the low reliability of
   * the edge words of a read.
   *
   * ???: Document more clearly how dynoverlap and MIN_SECTOR_BACKUP
   * are calculated and used.
   */

  /* What is the first sector to read?  want some pre-buffer if
     we're not at the extreme beginning of the disc */
  
  if (p->enable&(PARANOIA_MODE_VERIFY|PARANOIA_MODE_OVERLAP)){
    
    /* we want to jitter the read alignment boundary */
    long target;
    if (rv(root)==NULL || rb(root)>beginword)
      target=p->cursor-dynoverlap; 
    else
      target=re(root)/(CD_FRAMEWORDS)-dynoverlap;
	
    if (target+MIN_SECTOR_BACKUP>p->lastread && target<=p->lastread)
      target=p->lastread-MIN_SECTOR_BACKUP;

    /* we want to jitter the read alignment boundary, as some
       drives, beginning from a specific point, will tend to
       lose bytes between sectors in the same place.  Also, as
       our vectors are being made up of multiple reads, we want
       the overlap boundaries to move.... */
    
    readat=(target&(~((long)JIGGLE_MODULO-1)))+p->jitter;
    if (readat>target)readat-=JIGGLE_MODULO;
    p->jitter++;
    if (p->jitter>=JIGGLE_MODULO)
      p->jitter=0;
     
  } else {
    readat=p->cursor; 
  }
  
  readat+=driftcomp;

  /* Create a new, empty c_block and add it to the head of the
   * list of c_blocks in memory.  It will be empty until the end of
   * this subroutine.
   */
  if (p->enable&(PARANOIA_MODE_OVERLAP|PARANOIA_MODE_VERIFY)) {
    flags=calloc(totaltoread*CD_FRAMEWORDS, 1);
    new=new_c_block(p);
    recover_cache(p);
  } else {
    /* in the case of root it's just the buffer */
    paranoia_resetall(p);	
    new=new_c_block(p);
  }

  buffer=calloc(totaltoread*CDIO_CD_FRAMESIZE_RAW, 1);
  sofar=0;
  firstread=-1;


  /* Issue each of the low-level reads until we've read enough sectors
   * to exhaust the drive's cache.
   *
   * p->readahead   = total number of sectors to read
   * p->d->nsectors = number of sectors to read per request
   *
   * The driver determines this latter number, which is the maximum
   * number of sectors the kernel can reliably read per request.  In
   * old Linux kernels, there was a hard limit of 8 sectors per read.
   * While this limit has since been removed, certain motherboards
   * can't handle DMA requests larger than 64K.  And other operating
   * systems may have similar limitations.  So the method of splitting
   * up reads is still useful.
   */

  /* actual read loop */

  while (sofar<totaltoread){
    long secread=sectatonce;  /* number of sectors to read this request */
    long adjread=readat;      /* first sector to read for this request */
    long thisread;            /* how many sectors were read this request */

    /* don't under/overflow the audio session */
    if (adjread<p->current_firstsector){
      secread-=p->current_firstsector-adjread;
      adjread=p->current_firstsector;
    }
    if (adjread+secread-1>p->current_lastsector)
      secread=p->current_lastsector-adjread+1;
    
    if (sofar+secread>totaltoread)secread=totaltoread-sofar;
    
    if (secread>0){
      
      if (firstread<0) firstread = adjread;

      /* Issue the low-level read to the driver.
       */

      thisread = cdda_read(p->d, buffer+sofar*CD_FRAMEWORDS, adjread, secread);


      /* If the low-level read returned too few sectors, pad the result
       * with null data and mark it as invalid (FLAGS_UNREAD).  We pad
       * because we're going to be appending further reads to the current
       * c_block.
       *
       * ???: Why not re-read?  It might be to keep you from getting
       * hung up on a bad sector.  Or it might be to avoid interrupting
       * the streaming as much as possible.
       */
      if ( thisread < secread) {

	if (thisread<0) thisread=0;

	/* Uhhh... right.  Make something up. But don't make us seek
           backward! */

	if (callback)
	  (*callback)((adjread+thisread)*CD_FRAMEWORDS, PARANOIA_CB_READERR);  
	memset(buffer+(sofar+thisread)*CD_FRAMEWORDS,0,
	       CDIO_CD_FRAMESIZE_RAW*(secread-thisread));
	if (flags)
          memset(flags+(sofar+thisread)*CD_FRAMEWORDS, FLAGS_UNREAD,
	         CD_FRAMEWORDS*(secread-thisread));
      }
      if (thisread!=0)anyflag=1;


      /* Because samples are likely to be dropped between read requests,
       * mark the samples near the the boundaries of the read requests
       * as suspicious (FLAGS_EDGE).  This means that any span of samples
       * against which these adjacent read requests are compared must
       * overlap beyond the edges and into the more trustworthy data.
       * Such overlapping spans are accordingly at least MIN_WORDS_OVERLAP
       * words long (and naturally longer if any samples were dropped
       * between the read requests).
       *
       *          (EEEEE...overlapping span...EEEEE)
       * (read 1 ...........EEEEE)   (EEEEE...... read 2 ......EEEEE) ...
       *         dropped samples --^
       */
      if (flags && sofar!=0){
	/* Don't verify across overlaps that are too close to one
           another */
	int i=0;
	for(i=-MIN_WORDS_OVERLAP/2;i<MIN_WORDS_OVERLAP/2;i++)
	  flags[sofar*CD_FRAMEWORDS+i]|=FLAGS_EDGE;
      }


      /* Move the read cursor ahead by the number of sectors we attempted
       * to read.
       *
       * ???: Again, why not move it ahead by the number actually read?
       */
      p->lastread=adjread+secread;
      
      if (adjread+secread-1==p->current_lastsector)
	new->lastsector=-1;
      
      if (callback)(*callback)((adjread+secread-1)*CD_FRAMEWORDS,PARANOIA_CB_READ);
      
      sofar+=secread;
      readat=adjread+secread; 
    } else /* secread <= 0 */
      if (readat<p->current_firstsector)
	readat+=sectatonce; /* due to being before the readable area */
      else
	break; /* due to being past the readable area */


    /* Keep issuing read requests until we've read enough sectors to
     * exhaust the drive's cache.
     */

  } /* end while */


  /* If we managed to read any sectors at all (anyflag), fill in the
   * previously allocated c_block with the read data.  Otherwise, free
   * our buffers, dispose of the c_block, and return NULL.
   */
  if (anyflag) {
    new->vector=buffer;
    new->begin=firstread*CD_FRAMEWORDS-p->dyndrift;
    new->size=sofar*CD_FRAMEWORDS;
    new->flags=flags;
  } else {
    if (new)free_c_block(new);
    free(buffer);
    free(flags);
    new=NULL;
  }
  return(new);
}


/** ==========================================================================
 * cdio_paranoia_read(), cdio_paranoia_read_limited()
 *
 * These functions "read" the next sector of audio data and returns
 * a pointer to a full sector of verified samples (2352 bytes).
 *
 * The returned buffer is *not* to be freed by the caller.  It will
 *   persist only until the next call to paranoia_read() for this p 
*/

int16_t *
cdio_paranoia_read(cdrom_paranoia_t *p, 
		   void(*callback)(long, paranoia_cb_mode_t))
{
  return paranoia_read_limited(p, callback, 20);
}

/* I added max_retry functionality this way in order to avoid
   breaking any old apps using the nerw libs.  cdparanoia 9.8 will
   need the updated libs, but nothing else will require it. */
int16_t *
cdio_paranoia_read_limited(cdrom_paranoia_t *p, 
			   void(*callback)(long int, paranoia_cb_mode_t),
			   int max_retries)
{
  long int beginword  =  p->cursor*(CD_FRAMEWORDS);
  long int endword    =  beginword+CD_FRAMEWORDS;
  long int retry_count=  0;
  long int lastend    = -2;
  root_block *root    = &p->root;

  if (beginword > p->root.returnedlimit)
    p->root.returnedlimit=beginword;
  lastend=re(root);


  /* Since paranoia reads and verifies chunks of data at a time
   * (which it needs to counteract dropped samples and inaccurate
   * seeking), the requested samples may already be in memory,
   * in the verified "root".
   *
   * The root is where paranoia stores samples that have been
   * verified and whose position has been accurately determined.
   */
  
  /* First, is the sector we want already in the root? */
  while (rv(root)==NULL ||
	rb(root)>beginword || 
	(re(root)<endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS) &&
	 p->enable&(PARANOIA_MODE_VERIFY|PARANOIA_MODE_OVERLAP)) ||
	re(root)<endword){
    
    /* Nope; we need to build or extend the root verified range */

    /* We may have already read the necessary samples and placed
     * them into verified fragments, but not yet merged them into
     * the verified root.  We'll check that before we actually
     * try to read data from the drive.
     */

    if (p->enable&(PARANOIA_MODE_VERIFY|PARANOIA_MODE_OVERLAP)){

      /* We need to make sure our memory consumption doesn't grow
       * to the size of the whole CD.  But at the same time, we
       * need to hang onto some of the verified data (even perhaps
       * data that's already been returned by paranoia_read()) in
       * order to verify and accurately position future samples.
       *
       * Therefore, we free some of the verified data that we
       * no longer need.
       */
      i_paranoia_trim(p,beginword,endword);
      recover_cache(p);

      if (rb(root)!=-1 && p->root.lastsector)
	i_end_case(p, endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
			callback);
      else

	/* Merge as many verified fragments into the verified root
	 * as we need to satisfy the pending request.  We may
	 * not have all the fragments we need, in which case we'll
	 * read data from the CD further below.
	 */
	i_stage2(p, beginword,
		      endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
		      callback);
    } else
      i_end_case(p,endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
		 callback); /* only trips if we're already done */

    /* If we were able to fill the verified root with data already
     * in memory, we don't need to read any more data from the drive.
     */
    if (!(rb(root)==-1 || rb(root)>beginword || 
	 re(root)<endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS))) 
      break;
    
    /* Hmm, need more.  Read another block */

    {
      /* Read many sectors, encompassing at least the requested words.
       *
       * The returned c_block encapsulates these sectors' data and
       * sector number.  The sectors come come from multiple low-level
       * read requests, and words which were near the boundaries of
       * those read requests are marked with FLAGS_EDGE.
       */
      c_block_t *new=i_read_c_block(p,beginword,endword,callback);
      
      if (new){
	if (p->enable&(PARANOIA_MODE_OVERLAP|PARANOIA_MODE_VERIFY)){
      
	  /* If we need to verify these samples, send them to
	   * stage 1 verification, which will add verified samples
	   * to the set of verified fragments.  Verified fragments
	   * will be merged into the verified root during stage 2
	   * overlap analysis.
	   */
	  if (p->enable&PARANOIA_MODE_VERIFY)
	    i_stage1(p,new,callback);

	  /* If we're only doing overlapping reads (no stage 1
	   * verification), consider each low-level read in the
	   * c_block to be a verified fragment.  We exclude the
	   * edges from these fragments to enforce the requirement
	   * that we overlap the reads by the minimum amount.
	   * These fragments will be merged into the verified
	   * root during stage 2 overlap analysis.
	   */
	  else{
	    /* just make v_fragments from the boundary information. */
	    long begin=0,end=0;
	    
	    while (begin<cs(new)){
	      while (end<cs(new) && (new->flags[begin]&FLAGS_EDGE))begin++;
	      end=begin+1;
	      while (end<cs(new) && (new->flags[end]&FLAGS_EDGE)==0)end++;
	      {
		new_v_fragment(p,new,begin+cb(new),
			       end+cb(new),
			       (new->lastsector && cb(new)+end==ce(new)));
	      }
	      begin=end;
	    }
	  }
	  
	} else {

	  /* If we're not doing any overlapping reads or verification
	   * of data, skip over the stage 1 and stage 2 verification and
	   * promote this c_block directly to the current "verified" root.
	   */

	  if (p->root.vector)i_cblock_destructor(p->root.vector);
	  free_elem(new->e,0);
	  p->root.vector=new;

	  i_end_case(p,endword+(MAX_SECTOR_OVERLAP*CD_FRAMEWORDS),
			  callback);
      
	}
      }
    }

    /* Are we doing lots of retries?  **************************************/

    /* ???: To be studied
     */

    /* Check unaddressable sectors first.  There's no backoff here; 
       jiggle and minimum backseek handle that for us */
    
    if (rb(root)!=-1 && lastend+588<re(root)){ /* If we've not grown
						 half a sector */
      lastend=re(root);
      retry_count=0;
    } else {
      /* increase overlap or bail */
      retry_count++;
      
      /* The better way to do this is to look at how many actual
	 matches we're getting and what kind of gap */

      if (retry_count%5==0){
	if (p->dynoverlap==MAX_SECTOR_OVERLAP*CD_FRAMEWORDS ||
	   retry_count==max_retries){
	  if (!(p->enable&PARANOIA_MODE_NEVERSKIP))
	    verify_skip_case(p,callback);
	  retry_count=0;
	} else {
	  if (p->stage1.offpoints!=-1){ /* hack */
	    p->dynoverlap*=1.5;
	    if (p->dynoverlap>MAX_SECTOR_OVERLAP*CD_FRAMEWORDS)
	      p->dynoverlap=MAX_SECTOR_OVERLAP*CD_FRAMEWORDS;
	    if (callback)
	      (*callback)(p->dynoverlap,PARANOIA_CB_OVERLAP);
	  }
	}
      }
    }

    /* Having read data from the drive and placed it into verified
     * fragments, we now loop back to try to extend the root with
     * the newly loaded data.  Alternatively, if the root already
     * contains the needed data, we'll just fall through.
     */

  } /* end while */
  p->cursor++;

  /* Return a pointer into the verified root.  Thus, the caller
   * must NOT free the returned pointer!
   */
  return(rv(root)+(beginword-rb(root)));
}

/* a temporary hack */
void 
cdio_paranoia_overlapset(cdrom_paranoia_t *p, long int overlap)
{
  p->dynoverlap=overlap*CD_FRAMEWORDS;
  p->stage1.offpoints=-1; 
}
