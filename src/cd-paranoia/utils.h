/*
  $Id: utils.h,v 1.5 2008/04/11 15:44:00 karl Exp $

  Copyright (C) 2004, 2005, 2008 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 1998 Monty <xiphmont@mit.edu>
  
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define copystring(s) (s) ? s : NULL;
  
static inline char *
catstring(char *buff, const char *s)
{
  if(s){
    if(buff)
      buff=realloc(buff,strlen(buff)+strlen(s)+1);
    else
      buff=calloc(strlen(s)+1,1);
    strcat(buff,s);
  }
  return(buff);
}

