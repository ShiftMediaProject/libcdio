/*
  $Id: report.c,v 1.3 2008/04/11 15:44:00 karl Exp $

  Copyright (C) 2004, 2008 Rocky Bernstein <rocky@gnu.org>
  Copyright (C) 1998 Monty xiphmont@mit.edu
  
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

/******************************************************************
 * 
 * reporting/logging routines
 *
 ******************************************************************/

#include <stdio.h>
#include "config.h"
#include <cdio/cdda.h>
#include "report.h"

int quiet=0;
int verbose=CDDA_MESSAGE_FORGETIT;

void 
report(const char *s)
{
  if (!quiet) {
    fprintf(stderr,s);
    fputc('\n',stderr);
  }
}

void 
report2(const char *s, char *s2)
{
  if (!quiet) {
    fprintf(stderr,s,s2);
    fputc('\n',stderr);
  }
}

void 
report3(const char *s, char *s2, char *s3)
{
  if (!quiet) {
    fprintf(stderr,s,s2,s3);
    fputc('\n',stderr);
  }
}
