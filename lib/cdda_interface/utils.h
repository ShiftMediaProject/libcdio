/*
  $Id: utils.h,v 1.1 2004/12/18 17:29:32 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
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

#include <endian.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <cdio/types.h>

/* I wonder how many alignment issues this is gonna trip in the
   future...  it shouldn't trip any...  I guess we'll find out :) */

static inline int bigendianp(void){
  int test=1;
  char *hack=(char *)(&test);
  if(hack[0])return(0);
  return(1);
}

static inline int32_t swap32(int32_t x){
  return((((u_int32_t)x & 0x000000ffU) << 24) | 
	 (((u_int32_t)x & 0x0000ff00U) <<  8) | 
	 (((u_int32_t)x & 0x00ff0000U) >>  8) | 
	 (((u_int32_t)x & 0xff000000U) >> 24));
}

static inline int16_t swap16(int16_t x){
  return((((u_int16_t)x & 0x00ffU) <<  8) | 
	 (((u_int16_t)x & 0xff00U) >>  8));
}

#if BYTE_ORDER == LITTLE_ENDIAN

static inline int32_t be32_to_cpu(int32_t x){
  return(swap32(x));
}

static inline int16_t be16_to_cpu(int16_t x){
  return(swap16(x));
}

static inline int32_t le32_to_cpu(int32_t x){
  return(x);
}

static inline int16_t le16_to_cpu(int16_t x){
  return(x);
}

#else

static inline int32_t be32_to_cpu(int32_t x){
  return(x);
}

static inline int16_t be16_to_cpu(int16_t x){
  return(x);
}

static inline int32_t le32_to_cpu(int32_t x){
  return(swap32(x));
}

static inline int16_t le16_to_cpu(int16_t x){
  return(swap16(x));
}


#endif

static inline int32_t cpu_to_be32(int32_t x){
  return(be32_to_cpu(x));
}

static inline int32_t cpu_to_le32(int32_t x){
  return(le32_to_cpu(x));
}

static inline int16_t cpu_to_be16(int16_t x){
  return(be16_to_cpu(x));
}

static inline int16_t cpu_to_le16(int16_t x){
  return(le16_to_cpu(x));
}

static inline char *copystring(const char *s){
  if(s){
    char *ret=malloc((strlen(s)+9)*sizeof(char)); /* +9 to get around a linux
						     libc 5 bug. below too */
    strcpy(ret,s);
    return(ret);
  }
  return(NULL);
}

static inline char *catstring(char *buff,const char *s){
  if(s){
    if(buff)
      buff=realloc(buff,strlen(buff)+strlen(s)+9);
    else
      buff=calloc(strlen(s)+9,1);
    strcat(buff,s);
  }
  return(buff);
}

void cderror(cdrom_drive_t *d, const char *s);

void cdmessage(cdrom_drive_t *d,const char *s);

void idperror(int messagedest,char **messages,const char *f, const char *s);

void idmessage(int messagedest,char **messages,const char *f, const char *s);

