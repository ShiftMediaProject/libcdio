/* -*- c -*-
    $Id: device_const.swg,v 1.1 2006/11/13 05:12:42 rocky Exp $

    Copyright (C) 2006 Rocky Bernstein <rocky@cpan.org>

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
/* See <cdio/device.h> for more extensive documentation. */

/**** ALL OF THESE ARE CONSTANTS *** */
%immutable;

/* Drive types returned by cdio_get_drive_cap() */
%constant long int DRIVE_CAP_ERROR = CDIO_DRIVE_CAP_ERROR;
%constant long int DRIVE_CAP_UNKNOWN = CDIO_DRIVE_CAP_UNKNOWN;
%constant long int DRIVE_CAP_MISC_CLOSE_TRAY = CDIO_DRIVE_CAP_MISC_CLOSE_TRAY;
%constant long int DRIVE_CAP_MISC_EJECT = CDIO_DRIVE_CAP_MISC_EJECT;
%constant long int DRIVE_CAP_MISC_LOCK	 = CDIO_DRIVE_CAP_MISC_LOCK	;
%constant long int DRIVE_CAP_MISC_SELECT_SPEED = CDIO_DRIVE_CAP_MISC_SELECT_SPEED;
%constant long int DRIVE_CAP_MISC_SELECT_DISC = CDIO_DRIVE_CAP_MISC_SELECT_DISC;
%constant long int DRIVE_CAP_MISC_MULTI_SESSION = CDIO_DRIVE_CAP_MISC_MULTI_SESSION;
%constant long int DRIVE_CAP_MISC_MEDIA_CHANGED = CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED;
%constant long int DRIVE_CAP_MISC_RESET = CDIO_DRIVE_CAP_MISC_RESET;
%constant long int DRIVE_CAP_MISC_FILE = CDIO_DRIVE_CAP_MISC_FILE;

/* Reading masks.. */
%constant long int DRIVE_CAP_READ_AUDIO = CDIO_DRIVE_CAP_READ_AUDIO;
%constant long int DRIVE_CAP_READ_CD_DA = CDIO_DRIVE_CAP_READ_CD_DA;
%constant long int DRIVE_CAP_READ_CD_G = CDIO_DRIVE_CAP_READ_CD_G;
%constant long int DRIVE_CAP_READ_CD_R = CDIO_DRIVE_CAP_READ_CD_R;
%constant long int DRIVE_CAP_READ_CD_RW = CDIO_DRIVE_CAP_READ_CD_RW;
%constant long int DRIVE_CAP_READ_DVD_R = CDIO_DRIVE_CAP_READ_DVD_R;
%constant long int DRIVE_CAP_READ_DVD_PR = CDIO_DRIVE_CAP_READ_DVD_PR;
%constant long int DRIVE_CAP_READ_DVD_RAM = CDIO_DRIVE_CAP_READ_DVD_RAM;
%constant long int DRIVE_CAP_READ_DVD_ROM = CDIO_DRIVE_CAP_READ_DVD_ROM;
%constant long int DRIVE_CAP_READ_DVD_RW = CDIO_DRIVE_CAP_READ_DVD_RW;
%constant long int DRIVE_CAP_READ_DVD_RPW = CDIO_DRIVE_CAP_READ_DVD_RPW;
%constant long int DRIVE_CAP_READ_C2_ERRS = CDIO_DRIVE_CAP_READ_C2_ERRS;
%constant long int DRIVE_CAP_READ_MODE2_FORM1 = CDIO_DRIVE_CAP_READ_MODE2_FORM1;
%constant long int DRIVE_CAP_READ_MODE2_FORM2 = CDIO_DRIVE_CAP_READ_MODE2_FORM2;
%constant long int DRIVE_CAP_READ_MCN = CDIO_DRIVE_CAP_READ_MCN;
%constant long int DRIVE_CAP_READ_ISRC = CDIO_DRIVE_CAP_READ_ISRC;

/* Writing masks.. */
%constant long int DRIVE_CAP_WRITE_CD_R = CDIO_DRIVE_CAP_WRITE_CD_R;
%constant long int DRIVE_CAP_WRITE_CD_RW = CDIO_DRIVE_CAP_WRITE_CD_RW;
%constant long int DRIVE_CAP_WRITE_DVD_R = CDIO_DRIVE_CAP_WRITE_DVD_R;
%constant long int DRIVE_CAP_WRITE_DVD_PR = CDIO_DRIVE_CAP_WRITE_DVD_PR;
%constant long int DRIVE_CAP_WRITE_DVD_RAM = CDIO_DRIVE_CAP_WRITE_DVD_RAM;
%constant long int DRIVE_CAP_WRITE_DVD_RW  = CDIO_DRIVE_CAP_WRITE_DVD_RW ;
%constant long int DRIVE_CAP_WRITE_DVD_RPW = CDIO_DRIVE_CAP_WRITE_DVD_RPW;
%constant long int DRIVE_CAP_WRITE_MT_RAINIER = CDIO_DRIVE_CAP_WRITE_MT_RAINIER;
%constant long int DRIVE_CAP_WRITE_BURN_PROOF = CDIO_DRIVE_CAP_WRITE_BURN_PROOF;

/*** Masks derived from above... ***/
/* Has some sort of CD writer ability.  */
%constant long int DRIVE_CAP_WRITE_CD = CDIO_DRIVE_CAP_WRITE_CD;
/* Has some sort of DVD writer ability */
%constant long int DRIVE_CAP_WRITE_DVD = CDIO_DRIVE_CAP_WRITE_DVD;
%constant long int DRIVE_CAP_WRITE = CDIO_DRIVE_CAP_WRITE;

/*! Size of fields returned by an INQUIRY command */
%constant long int MMC_HW_VENDOR_LEN = CDIO_MMC_HW_VENDOR_LEN;
%constant long int MMC_HW_MODEL_LEN = CDIO_MMC_HW_MODEL_LEN;
%constant long int MMC_HW_REVISION_LEN = CDIO_MMC_HW_REVISION_LEN;

/**! Flags specifying the category of device to open or is opened. */
%constant long int SRC_IS_DISK_IMAGE_MASK = CDIO_SRC_IS_DISK_IMAGE_MASK;
%constant long int SRC_IS_DEVICE_MASK = CDIO_SRC_IS_DEVICE_MASK;
%constant long int SRC_IS_SCSI_MASK = CDIO_SRC_IS_SCSI_MASK;
%constant long int SRC_IS_NATIVE_MASK = CDIO_SRC_IS_NATIVE_MASK;

/* driver_id_t enums. */
%constant long int DRIVER_UNKNOWN = DRIVER_UNKNOWN;
%constant long int DRIVER_AIX = DRIVER_AIX;
%constant long int DRIVER_BSDI = DRIVER_BSDI;
%constant long int DRIVER_FREEBSD = DRIVER_FREEBSD;
%constant long int DRIVER_LINUX = DRIVER_LINUX;
%constant long int DRIVER_SOLARIS = DRIVER_SOLARIS;
%constant long int DRIVER_OSX = DRIVER_OSX;
%constant long int DRIVER_WIN32 = DRIVER_WIN32;
%constant long int DRIVER_CDRDAO = DRIVER_CDRDAO;
%constant long int DRIVER_BINCUE = DRIVER_BINCUE;
%constant long int DRIVER_NRG = DRIVER_NRG;
%constant long int DRIVER_DEVICE = DRIVER_DEVICE;

%constant long int MIN_DRIVER = CDIO_MIN_DRIVER;
%constant long int MIN_DEVICE_DRIVER = CDIO_MIN_DEVICE_DRIVER;
%constant long int MAX_DRIVER = CDIO_MAX_DRIVER;
%constant long int MAX_DEVICE_DRIVER = CDIO_MAX_DEVICE_DRIVER;


%constant long int DRIVER_OP_SUCCESS = DRIVER_OP_SUCCESS;
%constant long int DRIVER_OP_ERROR = DRIVER_OP_ERROR;
%constant long int DRIVER_OP_UNSUPPORTED = DRIVER_OP_UNSUPPORTED;
%constant long int DRIVER_OP_UNINIT = DRIVER_OP_UNINIT;
%constant long int DRIVER_OP_NOT_PERMITTED = DRIVER_OP_NOT_PERMITTED;
%constant long int DRIVER_OP_BAD_PARAMETER = DRIVER_OP_BAD_PARAMETER;
%constant long int DRIVER_OP_BAD_POINTER = DRIVER_OP_BAD_POINTER;
%constant long int DRIVER_OP_NO_DRIVER  = DRIVER_OP_NO_DRIVER;

%constant unsigned int FS_AUDIO = CDIO_FS_AUDIO;
%constant unsigned int FS_HIGH_SIERRA = CDIO_FS_HIGH_SIERRA;
%constant unsigned int FS_ISO_9660 = CDIO_FS_ISO_9660;
%constant unsigned int FS_INTERACTIVE = CDIO_FS_INTERACTIVE;
%constant unsigned int FS_HFS = CDIO_FS_HFS;
%constant unsigned int FS_UFS = CDIO_FS_UFS;
%constant unsigned int FS_EXT2 = CDIO_FS_EXT2;
%constant unsigned int FS_ISO_HFS = CDIO_FS_ISO_HFS;
%constant unsigned int FS_ISO_9660_INTERACTIVE = CDIO_FS_ISO_9660_INTERACTIVE;
%constant unsigned int FS_3DO = CDIO_FS_3DO;
%constant unsigned int FS_XISO = CDIO_FS_XISO;
%constant unsigned int FS_UDFX = CDIO_FS_UDFX;
%constant unsigned int FS_UDF = CDIO_FS_UDF;
%constant unsigned int FS_ISO_UDF = CDIO_FS_ISO_UDF;

%constant unsigned int FS_ANAL_XA = CDIO_FS_ANAL_XA;
%constant unsigned int FS_ANAL_MULTISESSION = CDIO_FS_ANAL_MULTISESSION;
%constant unsigned int FS_ANAL_PHOTO_CD = CDIO_FS_ANAL_PHOTO_CD;
%constant unsigned int FS_ANAL_HIDDEN_TRACK = CDIO_FS_ANAL_HIDDEN_TRACK;
%constant unsigned int FS_ANAL_CDTV = CDIO_FS_ANAL_CDTV;
%constant unsigned int FS_ANAL_BOOTABLE = CDIO_FS_ANAL_BOOTABLE;
%constant unsigned int FS_ANAL_VIDEOCD = CDIO_FS_ANAL_VIDEOCD;
%constant unsigned int FS_ANAL_ROCKRIDGE = CDIO_FS_ANAL_ROCKRIDGE;
%constant unsigned int FS_ANAL_JOLIET = CDIO_FS_ANAL_JOLIET;
%constant unsigned int FS_ANAL_SVCD = CDIO_FS_ANAL_SVCD;
%constant unsigned int FS_ANAL_CVD = CDIO_FS_ANAL_CVD;
%constant unsigned int FS_ANAL_XISO = CDIO_FS_ANAL_XISO;
%constant unsigned int FS_MATCH_ALL = CDIO_FS_MATCH_ALL;
%constant unsigned int FS_UNKNOWN = CDIO_FS_UNKNOWN;

%mutable;
