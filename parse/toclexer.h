/*
    $Id: toclexer.h,v 1.1 2005/01/31 10:20:51 rocky Exp $

    Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>

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

/* Common header between TOC lexer and parser. */
#include <stdio.h>
#include <stdlib.h>
#include "string.h"

typedef int token_t;

extern FILE *toc_in;

typedef union {
  long unsigned int val;   /* For returning numbers.  */
  char  const *psz_str;    /* For strings.  */
} tocval_t;

#define YYSTYPE tocval_t
  
YYSTYPE toclval;

/* Call to the TOC scanner */
token_t toclex (void);



     
