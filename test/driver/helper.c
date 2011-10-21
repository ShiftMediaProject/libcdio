/* -*- C -*-
  Copyright (C) 2010, 2011 Rocky Bernstein <rocky@gnu.org>
  
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
#ifdef HAVE_CONFIG_H
# include "config.h"
# define __CDIO_CONFIG_H__ 1
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <cdio/cdio.h>
#include "helper.h"

void check_access_mode(CdIo_t *p_cdio, const char *psz_expected_access_mode)
{
    const char *psz_access_mode = cdio_get_arg(p_cdio, "access-mode");
    if ( psz_access_mode == NULL || 
	 (0 != strcmp(psz_expected_access_mode, psz_access_mode)) ) {
	fprintf(stderr, 
		"cdio_get_arg(\"access-mode?\") should return \"%s\"; got: \"%s\".\n",
		psz_expected_access_mode, psz_access_mode);
	exit(10);
    }
}

void check_get_arg_source(CdIo_t *p_cdio, const char *psz_expected_source)
  {
      const char *psz_source = cdio_get_arg(p_cdio, "source");
      if ( psz_source == NULL || 
	   (0 != strcmp(psz_expected_source, psz_source)) ) {
	  fprintf(stderr, 
		  "cdio_get_arg(\"source\") should return \"%s\"; got: \"%s\".\n",
		  psz_expected_source, psz_source);
	  exit(40);
      }
  }

/* i_expected: 1 => expect false 
               2 => expect true
               3 => expect true or false
 */
void check_mmc_supported(CdIo_t *p_cdio, int i_expected)  {
    const char *psz_response = cdio_get_arg(p_cdio, "mmc-supported?");
    if ( psz_response == NULL ) {
	fprintf(stderr, 
		"cdio_get_arg(\"mmc-supported?\") returned NULL\n");
	exit(30);
    } else if ( (0 == (i_expected & 1)) && 
		(0 == strncmp("false", psz_response, sizeof("false"))) ) {
	fprintf(stderr, 
		"cdio_get_arg(\"mmc-supported?\") should not return \"false\"");
	exit(31);
    } else if ( (0 == (i_expected & 2)) && 
		(0 == strncmp("true", psz_response, sizeof("true"))) ) {
	fprintf(stderr, 
		"cdio_get_arg(\"mmc-supported?\") should not return \"true\"");
	exit(32);
    }
}
