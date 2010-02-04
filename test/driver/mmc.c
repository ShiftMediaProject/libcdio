/* -*- C -*-
  Copyright (C) 2009 Thomas Schmitt <scdbackup@gmx.net>
  Copyright (C) 2010 Rocky Bernstein <rocky@gnu.org>

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
   Regression test for MMC commands.
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <cdio/cdio.h>
#include <cdio/logging.h>
#include <cdio/mmc.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#define SKIP_TEST 77

/* The compiler warns if no prototypes are given before function definition */

static int tmmc_eject_load_cycle(CdIo_t *p_cdio, int flag);

static int tmmc_eject_test_load(CdIo_t *p_cdio, int flag);

static driver_return_code_t tmmc_get_disc_erasable(const CdIo_t *p_cdio, 
						   const char *psz_source,
						   int verbose);

static int tmmc_handle_outcome(CdIo_t *p_cdio, int i_status,
			       int *sense_avail, 
			       unsigned char sense_reply[18], int flag);

static void tmmc_print_status_sense(int i_status, int sense_valid,
				    unsigned char sense_reply[18], int flag);

static int tmmc_load_eject(CdIo_t *p_cdio, int *sense_avail,
			   unsigned char sense_reply[18], int flag);
static int tmmc_mode_select(CdIo_t *p_cdio,
			    int *sense_avail, unsigned char sense_reply[18],
			    unsigned char *buf, int buf_fill, int flag);

static int tmmc_mode_sense(CdIo_t *p_cdio, int *sense_avail,
			   unsigned char sense_reply[18],
			   int page_code, int subpage_code, int alloc_len,
			   unsigned char *buf, int *buf_fill, int flag);

static int tmmc_test_unit_ready(CdIo_t *p_cdio, int *sense_avail,
				unsigned char sense_reply[18], int flag);
static int tmmc_wait_for_drive(CdIo_t *p_cdio, int max_tries, int flag);

static int tmmc_rwr_mode_page(CdIo_t *p_cdio, int flag);

static int tmmc_test(char *drive_path, int flag);


/* ------------------------- Helper functions ---------------------------- */


static driver_return_code_t
tmmc_get_disc_erasable(const CdIo_t *p_cdio, const char *psz_source,
		       int verbose) 
{
    driver_return_code_t drc;
    bool b_erasable = mmc_get_disc_erasable(p_cdio, &drc);
    if (verbose)
	printf("Disc is %serasable.\n", b_erasable ? "" : "not ");
    /* Try also with NULL. */
    b_erasable = mmc_get_disc_erasable(p_cdio, NULL);
    return drc;
}


/* @param flag bit0= verbose
*/
static void
tmmc_print_status_sense(int i_status, int sense_valid,
                        unsigned char sense[18], int flag)
{
  if (!(flag & 1))
    return;
  fprintf(stderr, "return= %d , sense(%d)", i_status, sense_valid);
  if (sense_valid >= 14)
    fprintf(stderr, ":  KEY= %1.1X , ASC= %2.2X , ASCQ= %2.2X",
            sense[2] & 0x0f, sense[12], sense[13]);
  if (flag & 1)
    fprintf(stderr, "\n");  
}


/* @param flag         bit0= verbose
*/
static int
tmmc_handle_outcome(CdIo_t *p_cdio, int i_status,
                    int *sense_avail, unsigned char sense_reply[18], int flag)
{
  unsigned char *sense = NULL;

  *sense_avail = mmc_last_cmd_sense(p_cdio, &sense);
  tmmc_print_status_sense(i_status, *sense_avail, sense, flag & 1);
  if (*sense_avail >= 18)
    memcpy(sense_reply, sense, 18);
  else
    memset(sense_reply, 0, 18);
  if (sense != NULL)
    free(sense);
  return i_status;
}


/* --------------------------- MMC commands ------------------------------ */


/* UNOBTRUSIVE */
/* @param flag         bit0= verbose
   @param sense_avail  Number of available sense bytes
                       (18 get copied if all 18 exist)
   @param sense_reply  eventual sense bytes
   @return             return value of mmc_run_cmd()
*/
static int
tmmc_test_unit_ready(CdIo_t *p_cdio,
                     int *sense_avail, unsigned char sense_reply[18], int flag)
{
  int i_status;
  mmc_cdb_t cdb = {{0, }};
  char buf[1]; /* just to have an address to pass to mmc_run_cmd() */

  memset(cdb.field, 0, 6);
  cdb.field[0] = CDIO_MMC_TEST_UNIT_READY;  /* SPC-3 6.33 */

  if (flag & 1)
    fprintf(stderr, "tmmc_test_unit_ready ... ");
  i_status = mmc_run_cmd(p_cdio, 10000, &cdb, SCSI_MMC_DATA_NONE, 0, buf);

  return tmmc_handle_outcome(p_cdio, i_status, sense_avail, sense_reply,
                             flag & 1);
}


/* OBTRUSIVE. PHYSICAL EFFECT: DANGER OF HUMAN INJURY */
/* @param flag bit0= verbose
               bit1= Asynchronous operation
               bit2= Load (else Eject)
   @return     return value of mmc_run_cmd()
*/
static int
tmmc_load_eject(CdIo_t *p_cdio, int *sense_avail,
                unsigned char sense_reply[18], int flag)
{
  int i_status;
  mmc_cdb_t cdb = {{0, }};
  char buf[1]; /* just to have an address to pass to mmc_run_cmd() */

  memset(cdb.field, 0, 6);
  cdb.field[0] = 0x1b;                 /* START/STOP UNIT, SBC-2 5.17 */
  cdb.field[1] = !!(flag & 2);         /* bit0= Immed */
  cdb.field[4] = (flag & 4) ? 3 : 2;   /* bit0= Start , bit1= Load/Eject */

  if (flag & 1)
    fprintf(stderr, "tmmc_load_eject(0x%X) ... ", (unsigned int) flag);
  i_status = mmc_run_cmd(p_cdio, 10000, &cdb, SCSI_MMC_DATA_NONE, 0, buf);

  return tmmc_handle_outcome(p_cdio, i_status, sense_avail, sense_reply,
                             flag & 1);
}


/* BARELY OBTRUSIVE: MIGHT SPOIL BURN RUNS */
/* Fetch a mode page or a part of it from the drive.
   @param alloc_len  The number of bytes to be requested from the drive and to
                     be copied into parameter buf.
                     This has to include the 8 bytes of header and may not
                     be less than 10.
   @param buf        Will contain at most alloc_len many bytes. The first 8 are
                     a Mode Parameter Header as of SPC-3 7.4.3, table 240.
                     The further bytes are the mode page, typically as of
                     MMC-5 7.2. There are meanwhile deprecated mode pages which
                     appear only in older versions of MMC.
   @param buf_fill   Will return the number of actually read bytes resp. the
                     number of available bytes. See flag bit1.
   @param flag       bit0= verbose
                     bit1= Peek mode:
                           Reply number of available bytes in *buf_fill and not
                           the number of actually read bytes.
   @return           return value of mmc_run_cmd(),
                     or other driver_return_code_t
*/
static driver_return_code_t
tmmc_mode_sense(CdIo_t *p_cdio, int *sense_avail,unsigned char sense_reply[18],
                int page_code, int subpage_code, int alloc_len,
                unsigned char *buf, int *buf_fill, int flag)
{
  driver_return_code_t i_status;
  mmc_cdb_t cdb = {{0, }};

  if (alloc_len < 10)
    return DRIVER_OP_BAD_PARAMETER;

  mmc_mode_sense_10(p_cdio, buf, alloc_len, page_code);
  if (flag & 1)
    fprintf(stderr, "tmmc_mode_sense(0x%X, %X, %d) ... ",
	    (unsigned int) page_code, (unsigned int) subpage_code, alloc_len);
  i_status = mmc_run_cmd(p_cdio, 10000, &cdb, SCSI_MMC_DATA_READ,
                         alloc_len, buf);
  tmmc_handle_outcome(p_cdio, i_status, sense_avail, sense_reply, flag & 1);
  if (DRIVER_OP_SUCCESS != i_status)
    return i_status;
  if (flag & 2)
    *buf_fill = buf[9] + 10;                /* MMC-5 7.2.3 */
  else
    *buf_fill = buf[0] * 256 + buf[1] + 2;  /* SPC-3 7.4.3, table 240 */
  return i_status;
}


/* OBTRUSIVE. SPOILS BURN RUNS and might return minor failure with
   -ROM drives */
/* Send a mode page to the drive.
   @param buf        Contains the payload bytes. The first 8 shall be a Mode
                     Parameter Header as of SPC-3 7.4.3, table 240.
                     The further bytes are the mode page, typically as of
                     MMC-5 7.2. There are meanwhile deprecated mode pages which
                     appear only in older versions of MMC.
   @param buf_fill   The number of bytes in buf.
   @param flag       bit0= verbose
   @return           return value of mmc_run_cmd(),
                     or other driver_return_code_t
*/
static int
tmmc_mode_select(CdIo_t *p_cdio,
                 int *sense_avail, unsigned char sense_reply[18],
                 unsigned char *buf, int buf_fill, int flag)
{
  int i_status, i;
  mmc_cdb_t cdb = {{0, }};

  if (buf_fill < 10)
    return DRIVER_OP_BAD_PARAMETER;

  if (flag & 1) {
    printf("tmmc_mode_select to drive: %d bytes\n", buf_fill);
    for (i = 0; i < buf_fill; i++) {
      printf("%2.2X ", (unsigned int) buf[i]);
      if ((i % 20) == 19)
        printf("\n");
    }
    if ((i % 20))
      printf("\n");
  }

  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_MODE_SELECT_10); /* SPC-3 6.8 */
  cdb.field[1] = 0x10;                 /* PF = 1 : official SCSI mode page */
  CDIO_MMC_SET_READ_LENGTH16(cdb.field, buf_fill);
  if (flag & 1)
    fprintf(stderr, "tmmc_mode_select(0x%X, %d, %d) ... ",
	    (unsigned int) buf[8], (unsigned int) buf[9], buf_fill);
  i_status = mmc_run_cmd(p_cdio, 10000, &cdb, SCSI_MMC_DATA_WRITE,
                         buf_fill, buf);
  return tmmc_handle_outcome(p_cdio, i_status, sense_avail, sense_reply,
                             flag & 1);
}


/* --------------------------- Larger gestures ----------------------------- */


/* UNOBTRUSIVE */
/* Watch drive by test unit ready loop until ready, no media or timeout.
   @param flag bit0= verbose
               bit1= expect media (do not end on no-media sense)
   @return     1= all seems well , 0= minor failure , -1= severe failure
*/
static int
tmmc_wait_for_drive(CdIo_t *p_cdio, int max_tries, int flag)
{
  int ret, i, sense_avail;
  unsigned char sense_reply[18];
  mmc_request_sense_t *p_sense_reply = (mmc_request_sense_t *) sense_reply;
  
  for (i = 0; i < max_tries; i++) {
    ret = tmmc_test_unit_ready(p_cdio, &sense_avail, sense_reply, flag & 1);
    if (ret == 0) /* Unit is ready */
      return 1;
    if (sense_avail < 18)
      return -1;
    if (p_sense_reply->sense_key == 2 && sense_reply[12] == 0x04) {

      /* Not ready */;

    } else if (p_sense_reply->sense_key == 6 && sense_reply[12] == 0x28
               && sense_reply[13] == 0x00) {

      /* Media change notice = try again */;

    } else if (p_sense_reply->sense_key == 2 && sense_reply[12] == 0x3a) {

      /* Medium not present */;

      if (!(flag & 2))
        return 1;
    } else if (p_sense_reply->sense_key == 0 && sense_reply[12] == 0) {

      /* Error with no sense */;

      return -1;
  break;
    } else {

      /* Other error */;

      return 0;
    } 
    sleep(1);
  }
  fprintf(stderr, "tmmc_eject_load_cycle: Drive not ready after %d retries\n",
          max_tries);
  return -1;
}


/* OBTRUSIVE , PHYSICAL EFFECT , DANGER OF HUMAN INJURY */
/* Eject, wait, load asynchronously, and watch by test unit ready loop.
   @param flag bit0= verbose
               bit1= expect media (do not end on no-media sense)
   @return    1= all seems well , 0= minor failure , -1= severe failure
*/
static int
tmmc_eject_load_cycle(CdIo_t *p_cdio, int flag)
{
  int ret, sense_avail;
  unsigned char sense_reply[18];

  /* Eject synchronously */
  fprintf(stderr,
    "tmmc_eject_load_cycle: WARNING: EJECTING THE TRAY !\n");
  sleep(2);
  tmmc_load_eject(p_cdio, &sense_avail, sense_reply, 0 | (flag & 1));

  fprintf(stderr,
    "tmmc_eject_load_cycle: waiting for 5 seconds. DO NOT TOUCH THE TRAY !\n");
  sleep(3);

  /* Load asynchronously */
  fprintf(stderr,
    "tmmc_eject_load_cycle: WARNING: LOADING THE TRAY !\n");
  sleep(2);
  tmmc_load_eject(p_cdio, &sense_avail, sense_reply, 4 | 2 | (flag & 1));

  /* Wait for drive attention */
  ret = tmmc_wait_for_drive(p_cdio, 30, flag & 3);
  return ret;
}


/* OBTRUSIVE , PHYSICAL EFFECT , DANGER OF HUMAN INJURY */
/* Eject, wait, test, load. All synchronously.
   @param flag bit0= verbose
   @return    1= all seems well , 0= minor failure , -1= severe failure
*/
static int
tmmc_eject_test_load(CdIo_t *p_cdio, int flag)
{
  int ret, sense_avail;
  unsigned char sense_reply[18];

  /* Eject synchronously */
  fprintf(stderr,
    "tmmc_eject_test_load: WARNING: EJECTING THE TRAY !\n");
  sleep(2);
  tmmc_load_eject(p_cdio, &sense_avail, sense_reply, 0 | (flag & 1));

  fprintf(stderr,
    "tmmc_eject_test_load: waiting for 5 seconds. DO NOT TOUCH THE TRAY !\n");
  sleep(3);

  ret = tmmc_test_unit_ready(p_cdio, &sense_avail, sense_reply, flag & 1);
  if (ret == 0) {
    fprintf(stderr,
            "tmmc_eject_test_load: Drive ready although tray ejected.\n");
    fprintf(stderr,
            "tmmc_eject_test_load: Test aborted. Tray will stay ejected.\n");
    return -1;
  }
  if (ret == 0 || sense_avail < 18) {
    fprintf(stderr,
 "tmmc_eject_test_load: Only %d sense reply bytes returned. Expected >= 18.\n",
            sense_avail);
    fprintf(stderr,
            "tmmc_eject_test_load: Test aborted. Tray will stay ejected.\n");
    return -1;
  }

  /* Load synchronously */
  fprintf(stderr,
    "tmmc_eject_test_load: WARNING: LOADING THE TRAY !\n");
  sleep(2);
  tmmc_load_eject(p_cdio, &sense_avail, sense_reply, 4 | (flag & 1));

  return 1;
}


/* OBTRUSIVE */
/* Read Mode Page 05h "Write Parameters", change a value, write the page,
   check effect, restore old value, check again.
   @param flag bit0= verbose
   @return    1= all seems well , 0= minor failure , -1= severe failure
*/
static int
tmmc_rwr_mode_page(CdIo_t *p_cdio, int flag)
{
  int ret, sense_avail, page_code = 5, subpage_code = 0, alloc_len, buf_fill;
  int write_type, final_return = 1, new_write_type, old_buf_fill;
  unsigned char sense_reply[18];
  unsigned char buf[265], old_buf[265];        /* page size is max. 255 + 10 */
  static char w_types[4][8] = {"Packet", "TAO", "SAO", "Raw"};

  alloc_len = 10;
  ret = tmmc_mode_sense(p_cdio, &sense_avail, sense_reply,
                        page_code, subpage_code, alloc_len,
                        buf, &buf_fill, 2 | (flag & 1));
  if (ret != 0) {
    fprintf(stderr, "tmmc_rwr_mode_page: Cannot obtain mode page 05h.\n");
    return 0;
  }
  alloc_len = (buf_fill <= sizeof(buf)) ? buf_fill : sizeof(buf);
  ret = tmmc_mode_sense(p_cdio, &sense_avail, sense_reply,
                        page_code, subpage_code, alloc_len,
                        buf, &buf_fill, (flag & 1));
  if (ret != 0) {
    fprintf(stderr, "tmmc_rwr_mode_page: Cannot obtain mode page 05h.\n");
    return 0;
  }
  memcpy(old_buf, buf, sizeof(buf));
  old_buf_fill = buf_fill;

  write_type = buf[10] & 0x0f; 
  if (flag & 1)
    fprintf(stderr, "tmmc_rwr_mode_page: Write type = %d (%s)\n",
            write_type, write_type < 4 ? w_types[write_type] : "Reserved");

  /* Choose a conservative CD writer setting.
  */
  memset(buf, 0, 8);
  if (write_type == 1) { /* is TAO -> make SAO */ 
    buf[10] = (buf[10] & 0xf0) | 2;   /* SAO */
    new_write_type = 2;
  } else {
    buf[10] = (buf[10] & 0xf0) | 1;   /* TAO */
    new_write_type = 1;
  }
  buf[11] = 4;                        /* Track Mode 4, no multi-session,
                                         variable Packet size */
  buf[12] = 8;                        /* Data Block Type : CD-ROM Mode 1 */
  buf[16] = 0;                        /* Session Format : "CD-DA or CD-ROM" */

  ret = tmmc_mode_select(p_cdio, &sense_avail, sense_reply,
                         buf, buf_fill, flag & 1);
  if (ret != 0) {
    fprintf(stderr, "tmmc_rwr_mode_page: Cannot set mode page 05h.\n");
    if (ret == DRIVER_OP_NOT_PERMITTED) {
      fprintf(stderr,
            "tmmc_rwr_mode_page: DRIVER_OP_NOT_PERMITTED with MODE SELECT.\n");
      return -1;
    }
    return 0;
  }

  /* Read mode page and check whether effect visible in buf[10] */
  ret = tmmc_mode_sense(p_cdio, &sense_avail, sense_reply,
                        page_code, subpage_code, alloc_len,
                        buf, &buf_fill, (flag & 1));
  if (ret != 0) {
    fprintf(stderr, "tmmc_rwr_mode_page: Cannot obtain mode page 05h.\n");
    final_return = final_return > 0 ? 0 : final_return;
  } else if ((buf[10] & 0xf) != new_write_type) {
    fprintf(stderr,
   "tmmc_rwr_mode_page: Mode page did not get into effect. (due %d , is %d)\n",
            new_write_type, buf[10] & 0xf);
    /* One of my DVD burners does this if no media is loaded */
    final_return = final_return > 0 ? 0 : final_return;
  } 

  /* Restore old mode page */
  ret = tmmc_mode_select(p_cdio, &sense_avail, sense_reply,
                         old_buf, old_buf_fill, flag & 1);
  if (ret != 0) {
    fprintf(stderr, "tmmc_rwr_mode_page: Cannot set mode page 05h.\n");
    if (ret == DRIVER_OP_NOT_PERMITTED) {
      fprintf(stderr,
            "tmmc_rwr_mode_page: DRIVER_OP_NOT_PERMITTED with MODE SELECT.\n");
      return -1;
    }
    final_return = final_return > 0 ? 0 : final_return;
  }

  /* Read mode page and check whether old_buf is in effect again */
  ret = tmmc_mode_sense(p_cdio, &sense_avail, sense_reply,
                        page_code, subpage_code, alloc_len,
                        buf, &buf_fill, (flag & 1));
  if (ret != 0) {
    fprintf(stderr, "tmmc_rwr_mode_page: Cannot obtain mode page 05h.\n");
    return final_return > 0 ? 0 : final_return;
  } else if (memcmp(buf, old_buf, buf_fill) != 0) {
    fprintf(stderr,
            "tmmc_rwr_mode_page: Mode page was not restored to old state.\n");
    final_return = -1;
  } 
  if (flag & 1)
    printf("tmmc_rwr_mode_page: Mode page 2Ah restored to previous state\n");
  return final_return;
}


/* ----------- Test of MMC driver enhancements , december 2009 ------------- */


/* OBTRUSIVE */
/* 
   This function bundles tests for the capability to perform MMC commands
   of payload direction SCSI_MMC_DATA_WRITE and to detect the sense reply of
   MMC commands, which indicates error causes or important drive events.

   The default configuration is not very obtrusive to the drive, although the
   drive should not be used for other operations at the same time.
   There are static variables in this function which enable additional
   more obtrusive tests or simulate the lack of write capabilities.
   See the statements about obtrusiveness and undesired side effects above
   the descriptions of the single functions.

   @param drive_path  a drive address suitable for cdio_open_am()
   @param flag        bit0= verbose
   @return            0= no severe failure
                      else an proposal for an exit() value is returned
*/
static int
tmmc_test(char *drive_path, int flag)
{
  int sense_avail = 0, ret, sense_valid, buf_fill, alloc_len = 10, verbose;
  int old_log_level;
  unsigned char sense[18], buf[10];
  CdIo_t *p_cdio;


  /* If non-0 then the tray will get ejected and loaded again */
  static int with_tray_dance = 0;

  /* If non-0 then an asynchronous test loop will wait 30 s for loaded media */
  static int test_cycle_with_media = 0;

  /* If non-0 then a lack of w-permission will be emulated by using the
     insufficient access mode "IOCTL" */
  static int emul_lack_of_wperm = 0;

  /* If non-0 then demand that cdio_get_arg(,"scsi-tuple") return non-NULL */
  static int demand_scsi_tuple = 0;

  const char *scsi_tuple;

  old_log_level = cdio_loglevel_default;
  verbose = flag & 1;

  p_cdio = cdio_open_am(drive_path, DRIVER_DEVICE, "MMC_RDWR_EXCL");
  if (!p_cdio) 
    ret = SKIP_TEST - 16;
  else  {
    const char *psz_access_mode = cdio_get_arg(p_cdio, "access-mode");

    if (0 != strncmp(psz_access_mode,
                     "MMC_RDWR_EXCL", strlen("MMC_RDWR_EXCL"))) {
      fprintf(stderr,
              "Got %s; Should get back %s, the access mode requested.\n",
              psz_access_mode, "MMC_RDWR_EXCL");
      {ret = 1; goto ex;}
    }

    /* The further code produces some intentional failures which should not be
       reported by mmc_run_cmd() as INFO messages.
    */
    cdio_loglevel_default = CDIO_LOG_WARN;

  /* Test availability of info for cdrecord style adresses .
  */
  scsi_tuple = cdio_get_arg(p_cdio, "scsi-tuple");
  if (scsi_tuple == NULL) {
    if (demand_scsi_tuple) {
      fprintf(stderr, "Error: cdio_get_arg(\"scsi-tuple\") returns NULL.\n");
      {ret = 6; goto ex;}
    } else if (flag & 1)
      fprintf(stderr, "cdio_get_arg(\"scsi-tuple\") returns NULL.\n");
  } else if (flag & 1)
    printf("Drive '%s' has cdio_get_arg(\"scsi-tuple\") = '%s'\n",
           drive_path, scsi_tuple);


  /* Test availability of sense reply in case of unready drive.
     E.g. if the tray is already ejected.
  */
  ret = tmmc_test_unit_ready(p_cdio, &sense_avail, sense, !!verbose);
  if (ret != 0 && sense_avail < 18) {
    fprintf(stderr,
            "Error: Drive not ready. Only %d sense bytes. Expected >= 18.\n",
            sense_avail);
    {ret = 2; goto ex;}
  }

  /* Cause sense reply failure by requesting inappropriate mode page 3Eh */
  ret = tmmc_mode_sense(p_cdio, &sense_avail, sense,
                       0x3e, 0, alloc_len, buf, &buf_fill, !!verbose);
  if (ret != 0 && sense_avail < 18) {
    fprintf(stderr,
            "Error: An illegal command yields only %d sense bytes. Expected >= 18.\n",
            sense_avail);
    {ret = 2; goto ex;}
  } 

  /* Test availability of sense reply in case of unready drive.
     E.g. if the tray is already ejected.
  */
  ret = tmmc_test_unit_ready(p_cdio, &sense_avail, sense, !!verbose);
  if (ret != 0 && sense_avail < 18) {
    fprintf(stderr,
	    "Error: Drive not ready. Only %d sense bytes. Expected >= 18.\n",
	    sense_avail);
    {ret = 2; goto ex;}
  }

  /* Cause sense reply failure by requesting inappropriate mode page 3Eh */
  ret = tmmc_mode_sense(p_cdio, &sense_avail, sense,
			0x3e, 0, alloc_len, buf, &buf_fill, !!verbose);
  if (ret != 0 && sense_avail < 18) {
    fprintf(stderr,
	    "Error: Deliberately illegal command yields only %d sense bytes. Expected >= 18.\n",
	    sense_avail);
    {ret = 2; goto ex;}
  }
    
    
  if (emul_lack_of_wperm) { /* To check behavior with lack of w-permission */
    fprintf(stderr,
	    "tmmc_test: SIMULATING LACK OF WRITE CAPABILITIES by access mode IOCTL\n");
    cdio_destroy(p_cdio);
    p_cdio = cdio_open_am(drive_path, DRIVER_DEVICE, "IOCTL");
  }
    
    
  /* Test write permission */ /* Try whether a mode page 2Ah can be set */
  ret = tmmc_rwr_mode_page(p_cdio, !!verbose);
  if (ret <= 0) {
    if (ret < 0) {
      fprintf(stderr, "Error: tmmc_rwr_mode_page() had severe failure.\n");
      {ret = 3; goto ex;}
    }
    fprintf(stderr, "Warning: tmmc_rwr_mode_page() had minor failure.\n");
  }
  
  
  if (with_tray_dance) {
    /* More surely provoke a non-trivial sense reply */
    if (test_cycle_with_media) {
      /* Eject, wait, load, watch by test unit ready loop */
      ret = tmmc_eject_load_cycle(p_cdio, 2 | !!verbose);
      if (ret <= 0) {
	if (ret < 0) {
	  fprintf(stderr,
		  "Error: tmmc_eject_load_cycle() had severe failure.\n");
	  {ret = 4; goto ex;}
	}
	fprintf(stderr,
		"Warning: tmmc_eject_load_cycle() had minor failure.\n");
      }
    } else {
      /* Eject, test for proper unreadiness, load */
      ret = tmmc_eject_test_load(p_cdio, !!verbose);
      if (ret <= 0) {
	if (ret < 0) {
	  fprintf(stderr,
		  "Error: tmmc_eject_test_load() had severe failure.\n");
	  {ret = 5; goto ex;}
	}
	fprintf(stderr,
		"Warning: tmmc_eject_test_load() had minor failure.\n");
      }
      /* Wait for drive attention */
      tmmc_wait_for_drive(p_cdio, 15, 2 | !!verbose);
    }
  }
  
  /* How are we, finally ? */
  ret = tmmc_test_unit_ready(p_cdio, &sense_valid, sense, !!verbose);
  if ((flag & 1) && ret != 0 && sense[2] == 2 && sense[12] == 0x3a)
    fprintf(stderr, "tmmc_test: Note: No loaded media detected.\n");
  ret = 0;
  }

ex:;
  cdio_loglevel_default = old_log_level;
  cdio_destroy(p_cdio);
  return ret;
}


/* ---------------------------      main       ----------------------------- */


int
main(int argc, const char *argv[])
{
  const char *scsi_tuple;
  CdIo_t *p_cdio;
  char **ppsz_drives=NULL;
  const char *psz_source = NULL;
  int ret;
  int exitrc = 0;
  bool b_verbose = (argc > 1);
  driver_return_code_t drc;
  

  cdio_loglevel_default = b_verbose ? CDIO_LOG_DEBUG : CDIO_LOG_INFO;

  /* snprintf(psz_nrgfile, sizeof(psz_nrgfile)-1,
	     "%s/%s", TEST_DIR, cue_file[i]);
  */
  ppsz_drives = cdio_get_devices(DRIVER_DEVICE);
  if (!ppsz_drives) {
    printf("Can't find a CD-ROM drive. Skipping test.\n");
    exit(SKIP_TEST);
  }
  
  p_cdio = cdio_open(ppsz_drives[0], DRIVER_DEVICE);
  if (p_cdio) {
    const char *psz_have_mmc = cdio_get_arg(p_cdio, "mmc-supported?");
    
    psz_source = cdio_get_arg(p_cdio, "source");
    if (0 != strncmp(psz_source, ppsz_drives[0],
                     strlen(ppsz_drives[0]))) {
      fprintf(stderr, 
              "Got %s; should get back %s, the name we opened.\n",
              psz_source, ppsz_drives[0]);
      exit(1);
    }

    drc = tmmc_get_disc_erasable(p_cdio, psz_source, b_verbose);
    if (DRIVER_OP_SUCCESS != drc) {
	printf("Got status %d back from get_disc_erasable(%s)\n",
	       drc, psz_source);
    }
	
    if ( psz_have_mmc 
	 && 0 == strncmp("true", psz_have_mmc, sizeof("true")) ) {
	scsi_tuple = cdio_get_arg(p_cdio, "scsi-tuple");
	if (scsi_tuple == NULL) {
	    fprintf(stderr, "cdio_get_arg(\"scsi-tuple\") returns NULL.\n");
	    exit(3);
	} else if (cdio_loglevel_default == CDIO_LOG_DEBUG)
	    printf("Drive '%s' has cdio_get_arg(\"scsi-tuple\") = '%s'\n",
		   psz_source, scsi_tuple);

	/* Test the MMC enhancements of version 0.83 in december 2009 */
	ret = tmmc_test(ppsz_drives[0], 
			cdio_loglevel_default == CDIO_LOG_DEBUG);
	if (ret != 0) exit(ret + 16);
    }

    cdio_destroy(p_cdio);

  } else {
    fprintf(stderr, "cdio_open('%s') failed\n", ppsz_drives[0]);
    exit(2);
  }

  cdio_free_device_list(ppsz_drives);

  return exitrc;
}


