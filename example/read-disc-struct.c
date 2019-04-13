/*
  Copyright (C) 2019 Thomas Schmitt

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
/* READ DVD STRUCTURE */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <cdio/cdio.h>
#include <cdio/mmc.h>

static void
hexdump (FILE *stream,  uint8_t * buffer, unsigned int len)
{
    unsigned int i;
    for (i = 0; i < len; i++, buffer++)
    {
	if (i % 16 == 0)
	    fprintf (stream, "0x%04x: ", i);
	fprintf (stream, "%02x", *buffer);
	if (i % 2 == 1)
	    fprintf (stream, " ");
	if (i % 16 == 15) {
	    uint8_t *p;
	    fprintf (stream, "  ");
	    for (p=buffer-15; p <= buffer; p++) {
		fprintf(stream, "%c", isprint(*p) ?  *p : '.');
	    }
	    fprintf (stream, "\n");
	}
    }
    fprintf (stream, "\n");
    fflush (stream);
}

int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio;

  const char *psz_drive = NULL;

  /* Put in "/dev/sr0" here. */
  if (argc > 1) psz_drive = argv[1];

  p_cdio = cdio_open (psz_drive, DRIVER_DEVICE);

  if (NULL == p_cdio) {
    printf("Couldn't find DVD\n");
    return 77;
  } else {
    int i_status;              /* Result of MMC command */
    uint8_t buf[200] = { 0, }; /* Place to hold returned data */
    mmc_cdb_t cdb = {{0, }};   /* Command Descriptor Buffer */
    int i;

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_READ_DVD_STRUCTURE);
    CDIO_MMC_SET_READ_LENGTH16(cdb.field, sizeof(buf));

    for (i=0; i<=16; i++) {
	cdb.field[7] = i; /* The format field */
	i_status = mmc_run_cmd(p_cdio, 0, &cdb, SCSI_MMC_DATA_READ,
			       sizeof(buf), &buf);
	if (i_status == 0) {
	    hexdump(stdout, buf, 200);
	    printf("============================================\n");
	} else {
	    printf("Didn't get DVD Structure.\n");
	}
    }
  }


  cdio_destroy(p_cdio);

  return 0;
}
