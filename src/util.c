/*
  $Id: util.c,v 1.22 2004/08/10 03:47:57 rocky Exp $

  Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
  
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

/* Miscellaneous things common to standalone programs. */

#include "util.h"
#include <cdio/scsi_mmc.h>

cdio_log_handler_t gl_default_cdio_log_handler = NULL;
char *source_name = NULL;
char *program_name;

void 
myexit(CdIo *cdio, int rc) 
{
  if (NULL != cdio) cdio_destroy(cdio);
  if (NULL != program_name) free(program_name);
  if (NULL != source_name)  free(source_name);
  exit(rc);
}

void
print_version (char *program_name, const char *version, 
	       int no_header, bool version_only)
{
  
  driver_id_t driver_id;

  if (no_header == 0)
    printf( "%s version %s\nCopyright (c) 2003, 2004 R. Bernstein\n",
	    program_name, version);
  printf( _("This is free software; see the source for copying conditions.\n\
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.\n\
"));

  if (version_only) {
    char *default_device;
    for (driver_id=DRIVER_UNKNOWN+1; driver_id<=CDIO_MAX_DRIVER; driver_id++) {
      if (cdio_have_driver(driver_id)) {
	printf("Have driver: %s\n", cdio_driver_describe(driver_id));
      }
    }
    default_device=cdio_get_default_device(NULL);
    if (default_device)
      printf("Default CD-ROM device: %s\n", default_device);
    else
      printf("No CD-ROM device found.\n");
    free(program_name);
    exit(100);
  }
  
}

#define DEV_PREFIX "/dev/"
char *
fillout_device_name(const char *device_name) 
{
#if defined(HAVE_WIN32_CDROM)
  return strdup(device_name);
#else
  unsigned int prefix_len = strlen(DEV_PREFIX);
  if (0 == strncmp(device_name, DEV_PREFIX, prefix_len))
    return strdup(device_name);
  else {
    char *full_device_name = (char*) malloc(strlen(device_name)+prefix_len);
    sprintf(full_device_name, DEV_PREFIX "%s", device_name);
    return full_device_name;
  }
#endif
}

/*! Prints out SCSI-MMC drive features  */
void 
print_mmc_drive_features(CdIo *p_cdio)
{
  
  int i_status;                  /* Result of SCSI MMC command */
  uint8_t buf[500] = { 0, };         /* Place to hold returned data */
  scsi_mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Block */
  
  CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_GET_CONFIGURATION);
  CDIO_MMC_SET_READ_LENGTH8(cdb.field, sizeof(buf));
  cdb.field[1] = CDIO_MMC_GET_CONF_ALL_FEATURES;
  cdb.field[3] = 0x0;
  
  i_status = scsi_mmc_run_cmd(p_cdio, 0, 
			      &cdb, SCSI_MMC_DATA_READ, 
			      sizeof(buf), &buf);
  if (i_status == 0) {
    uint8_t *p;
    uint32_t i_data;
    uint8_t *p_max = buf + 65530;
    
    i_data  = (unsigned int) CDIO_MMC_GET_LEN32(buf);

    /* set to first sense feature code, and then walk through the masks */
    p = buf + 8;
    while( (p < &(buf[i_data])) && (p < p_max) ) {
      uint16_t i_feature;
      uint8_t i_feature_additional = p[3];
      
      i_feature = CDIO_MMC_GET_LEN16(p);
      switch( i_feature )
	{
	  uint8_t *q;
	case CDIO_MMC_FEATURE_PROFILE_LIST:
	  printf("Profile List Feature\n");
	  for ( q = p+4 ; q < p + i_feature_additional ; q += 4 ) {
	    int i_profile=CDIO_MMC_GET_LEN16(q);
	    switch (i_profile) {
	    case CDIO_MMC_FEATURE_PROF_NON_REMOVABLE:
	      printf("\tRe-writable disk, capable of changing behavior");
	      break;
	    case CDIO_MMC_FEATURE_PROF_REMOVABLE:
	      printf("\tdisk Re-writable; with removable media");
	      break;
	    case CDIO_MMC_FEATURE_PROF_MO_ERASABLE:
	      printf("\tErasable Magneto-Optical disk with sector erase capability");
	      break;
	    case CDIO_MMC_FEATURE_PROF_MO_WRITE_ONCE:
	      printf("\tWrite Once Magneto-Optical write once");
	      break;
	    case CDIO_MMC_FEATURE_PROF_AS_MO:
	      printf("\tAdvance Storage Magneto-Optical");
	      break;
	    case CDIO_MMC_FEATURE_PROF_CD_ROM:
	      printf("\tRead only Compact Disc capable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_CD_R:
	      printf("\tWrite once Compact Disc capable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_CD_RW:
	      printf("\tCD-RW Re-writable Compact Disc capable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_ROM:
	      printf("\tRead only DVD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_R_SEQ:
	      printf("\tRe-recordable DVD using Sequential recording");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_RAM:
	      printf("\tRe-writable DVD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_RW_RO:
	      printf("\tRe-recordable DVD using Restricted Overwrite");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_RW_SEQ:
	      printf("\tRe-recordable DVD using Sequential recording");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_PRW:
	      printf("\tDVD+RW - DVD ReWritable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DVD_PR:
	      printf("\tDVD+R - DVD Recordable");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DDCD_ROM:
	      printf("\tRead only DDCD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DDCD_R:
	      printf("\tDDCD-R Write only DDCD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_DDCD_RW:
	      printf("\tRe-Write only DDCD");
	      break;
	    case CDIO_MMC_FEATURE_PROF_NON_CONFORM:
	      printf("\tThe Logical Unit does not conform to any Profile.");
	      break;
	    default: 
	      printf("\tUnknown Profile %x", i_profile);
	      break;
	    }
	    if (q[2] & 1) {
	      printf(" - on");
	    }
	    printf("\n");
	  }
	  printf("\n");
	  
	  break;
	case CDIO_MMC_FEATURE_CORE: 
	  {
	    uint8_t *q = p+4;
	    uint32_t 	i_interface_standard = CDIO_MMC_GET_LEN32(q);
	    printf("Core Feature\n");
	    switch(i_interface_standard) {
	    case 0: 
	      printf("\tunspecified interface\n");
	      break;
	    case 1: 
	      printf("\tSCSI interface\n");
	      break;
	    case 2: 
	      printf("\tATAPI interface\n");
	      break;
	    case 3: 
	      printf("\tIEEE 1394 interface\n");
	      break;
	    case 4:
	      printf("\tIEEE 1394A interface\n");
	      break;
	    case 5:
	      printf("\tFibre Channel interface\n");
	    }
	    printf("\n");
	    break;
	  }
	case CDIO_MMC_FEATURE_REMOVABLE_MEDIUM:
	  printf("Removable Medium Feature\n");
	  switch(p[4] >> 5) {
	  case 0:
	    printf("\tCaddy/Slot type loading mechanism\n");
	    break;
	  case 1:
	    printf("\tTray type loading mechanism\n");
	    break;
	  case 2:
	    printf("\tPop-up type loading mechanism\n");
	    break;
	  case 4:
	    printf("\tEmbedded changer with individually changeable discs\n");
	    break;
	  case 5:
	    printf("\tEmbedded changer using a magazine mechanism\n");
	    break;
	  default:
	    printf("\tUnknown changer mechanism\n");
	  }
	  
	  printf("\tcan%s eject the medium or magazine via the normal "
		 "START/STOP command\n", 
		 (p[4] & 8) ? "": "not");
	  printf("\tcan%s be locked into the Logical Unit\n", 
		 (p[4] & 1) ? "": "not");
	  printf("\n");
	  break;
	case CDIO_MMC_FEATURE_WRITE_PROTECT:
	  printf("Write Protect Feature\n");
	  break;
	case CDIO_MMC_FEATURE_RANDOM_READABLE:
	  printf("Random Readable Feature\n");
	  break;
	case CDIO_MMC_FEATURE_MULTI_READ:
	  printf("Multi-Read Feature\n");
	  break;
	case CDIO_MMC_FEATURE_CD_READ:
	  printf("CD Read Feature\n");
	  printf("\tC2 Error pointers are %ssupported\n", 
		 (p[4] & 2) ? "": "not ");
	  printf("\tCD-Text is %ssupported\n", 
		 (p[4] & 1) ? "": "not ");
	  printf("\n");
	  break;
	case CDIO_MMC_FEATURE_DVD_READ:
	  printf("DVD Read Feature\n");
	  break;
	case CDIO_MMC_FEATURE_RANDOM_WRITABLE:
	  printf("Random Writable Feature\n");
	  break;
	case CDIO_MMC_FEATURE_INCR_WRITE:
	  printf("Incremental Streaming Writable Feature\n");
	  break;
	case CDIO_MMC_FEATURE_SECTOR_ERASE:
	  printf("Sector Erasable Feature\n");
	  break;
	case CDIO_MMC_FEATURE_FORMATABLE:
	  printf("Formattable Feature\n");
	  break;
	case CDIO_MMC_FEATURE_DEFECT_MGMT:
	  printf("Management Ability of the Logical Unit/media system "
		 "to provide an apparently defect-free space.\n");
	  break;
	case CDIO_MMC_FEATURE_WRITE_ONCE:
	  printf("Write Once Feature\n");
	  break;
	case CDIO_MMC_FEATURE_RESTRICT_OVERW:
	  printf("Restricted Overwrite Feature\n");
	  break;
	case CDIO_MMC_FEATURE_CD_RW_CAV:
	  printf("CD-RW CAV Write Feature\n");
	  break;
	case CDIO_MMC_FEATURE_MRW:
	  printf("MRW Feature\n");
	  break;
	case CDIO_MMC_FEATURE_DVD_PRW:
	  printf("DVD+RW Feature\n");
	  break;
	case CDIO_MMC_FEATURE_DVD_PR:
	  printf("DVD+R Feature\n");
	  break;
	case CDIO_MMC_FEATURE_CD_TAO:
	  printf("CD Track at Once Feature\n");
	  break;
	case CDIO_MMC_FEATURE_CD_SAO:
	  printf("CD Mastering (Session at Once) Feature\n");
	  break;
	case CDIO_MMC_FEATURE_POWER_MGMT:
	  printf("Initiator and device directed power management\n");
	  break;
	case CDIO_MMC_FEATURE_CDDA_EXT_PLAY:
	  printf("CD Audio External Play Feature\n");
	  printf("\tSCAN command is %ssupported\n", 
		 (p[4] & 4) ? "": "not ");
	  printf("\taudio channels can %sbe muted separately\n", 
		 (p[4] & 2) ? "": "not ");
	  printf("\taudio channels can %shave separate volume levels\n", 
		 (p[4] & 1) ? "": "not ");
	  {
	    uint8_t *q = p+6;
	    uint16_t i_vol_levels = CDIO_MMC_GET_LEN16(q);
	    printf("\t%d volume levels can be set\n", i_vol_levels);
	  }
	  printf("\n");
	  break;
	case CDIO_MMC_FEATURE_MCODE_UPGRADE:
	  printf("Ability for the device to accept new microcode via "
		 "the interface\n");
	  break;
	case CDIO_MMC_FEATURE_TIME_OUT:
	  printf("Ability to respond to all commands within a "
		 "specific time\n");
	  break;
	case CDIO_MMC_FEATURE_DVD_CSS:
	  printf("Ability to perform DVD CSS/CPPM authentication and"
		 " RPC\n");
#if 0
	  printf("\tMedium does%s have Content Scrambling (CSS/CPPM)\n", 
		 (p[2] & 1) ? "": "not ");
#endif
	  printf("\tCSS version %d\n", p[7]);
	  printf("\t\n");
	  break;
	case CDIO_MMC_FEATURE_RT_STREAMING:
	  printf("\tAbility to read and write using Initiator requested performance parameters\n");
	  break;
	case CDIO_MMC_FEATURE_LU_SN: {
	  uint8_t i_serial = *(p+3);
	  char serial[257] = { '\0', };
	      
	  printf("The Logical Unit has a unique identifier:\n");
	  memcpy(serial, p+4, i_serial);
	  printf("\t%s\n\n", serial);

	  break;
	}
	default: 
	  if ( 0 != (i_feature & 0xFF00) ) {
	    printf("Vendor-specific feature code %x\n", i_feature);
	  } else {
	    printf("Unknown feature code %x\n", i_feature);
	  }
	  
	}
      p += i_feature_additional + 4;
    }
  } else {
    printf("Didn't get all feature codes\n");
  }
}


/* Prints out drive capabilities */
void 
print_drive_capabilities(cdio_drive_read_cap_t  i_read_cap,
			 cdio_drive_write_cap_t i_write_cap,
			 cdio_drive_misc_cap_t  i_misc_cap)
{
  if (CDIO_DRIVE_CAP_ERROR == i_misc_cap) {
    printf("Error in getting drive hardware properties\n");
  } else {
    printf(_("Hardware                    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_FILE  
	   ? "Disk Image"  : "CD-ROM or DVD");
    printf(_("Can eject                   : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_EJECT         ? "Yes" : "No");
    printf(_("Can close tray              : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_CLOSE_TRAY    ? "Yes" : "No");
    printf(_("Can disable manual eject    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_LOCK          ? "Yes" : "No");
    printf(_("Can select juke-box disc    : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_DISC   ? "Yes" : "No");

    printf(_("Can set drive speed         : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_SPEED  ? "Yes" : "No");
    printf(_("Can detect if CD changed    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED ? "Yes" : "No");
    printf(_("Can read multiple sessions  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MULTI_SESSION ? "Yes" : "No");
    printf(_("Can hard reset device       : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_RESET         ? "Yes" : "No");
  }
  
    
  if (CDIO_DRIVE_CAP_ERROR == i_read_cap) {
      printf("Error in getting drive reading properties\n");
  } else {
    printf("Reading....\n");
    printf(_("  Can play audio            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_AUDIO      ? "Yes" : "No");
    printf(_("  Can read  CD-R            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_R       ? "Yes" : "No");
    printf(_("  Can read  CD-RW           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW      ? "Yes" : "No");
    printf(_("  Can read  DVD-ROM         : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM    ? "Yes" : "No");
  }
  

  if (CDIO_DRIVE_CAP_ERROR == i_write_cap) {
      printf("Error in getting drive writing properties\n");
  } else {
    printf("\nWriting....\n");
    printf(_("  Can write CD-RW           : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_CD_RW     ? "Yes" : "No");
    printf(_("  Can write DVD-R           : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_R    ? "Yes" : "No");
    printf(_("  Can write DVD-RAM         : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RAM  ? "Yes" : "No");
    printf(_("  Can write DVD-RW          : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RW   ? "Yes" : "No");
    printf(_("  Can write DVD+RW          : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_WRITE_DVD_RPW  ? "Yes" : "No");
  }
}
