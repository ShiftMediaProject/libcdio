/*
  Copyright (C) 2011 Rocky Bernstein <rocky@gnu.org>
  
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

/* 
   Regression test config.h and cdio_unconfig
   and cdio_free_device_list()
*/
#include "config.h"
#define __CDIO_CONFIG_H__ 1

#include "cdio/cdio_config.h" /* should do nothing */
#include "cdio/cdio.h"

#include <stdio.h>

int
main(int argc, const char *argv[])
{

#ifndef PACKAGE
    printf("config.h should have set PACKAGE preprocessor symbol\n");
#endif

#include "cdio/cdio_unconfig.h"

#ifdef PACKAGE
    printf("cdio_unconfig should have removed PACKAGE preprocessor symbol\n");
    return 2;
#else 
    return 0;
#endif
}
