/*  private MMC helper routines.

    $Id: scsi_mmc_private.h,v 1.2 2004/07/26 03:39:55 rocky Exp $

    Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>

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

#include <cdio/scsi_mmc.h>

typedef 
int (*scsi_mmc_run_cmd_fn_t) ( const void *p_user_data, int i_timeout,
			       unsigned int i_cdb, 
			       const scsi_mmc_cdb_t *p_cdb, 
			       scsi_mmc_direction_t e_direction, 
			       unsigned int i_buf, /*in/out*/ void *p_buf );
			     
int set_bsize_mmc ( const void *p_env, 
		    const scsi_mmc_run_cmd_fn_t *run_scsi_mmc_cmd, 
		    unsigned int bsize);
