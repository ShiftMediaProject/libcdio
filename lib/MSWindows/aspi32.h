/* Win32 aspi specific */
/*
    $Id: aspi32.h,v 1.1 2004/03/05 12:32:45 rocky Exp $

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

#define ASPI_HAID           0
#define ASPI_TARGET         0
#define DTYPE_CDROM         0x05

#define SENSE_LEN           0x0E
#define SC_HA_INQUIRY       0x00
#define SC_GET_DEV_TYPE     0x01
#define SC_EXEC_SCSI_CMD    0x02
#define SC_GET_DISK_INFO    0x06
#define SS_COMP             0x01
#define SS_PENDING          0x00
#define SS_NO_ADAPTERS      0xE8
#define SRB_DIR_IN          0x08
#define SRB_DIR_OUT         0x10
#define SRB_EVENT_NOTIFY    0x40

#define SECTOR_TYPE_MODE2 0x14
#define READ_CD_USERDATA_MODE2 0x10

#define READ_TOC 0x43
#define READ_TOC_FORMAT_TOC 0x0

#pragma pack(1)

struct SRB_GetDiskInfo
{
    unsigned char   SRB_Cmd;
    unsigned char   SRB_Status;
    unsigned char   SRB_HaId;
    unsigned char   SRB_Flags;
    unsigned long   SRB_Hdr_Rsvd;
    unsigned char   SRB_Target;
    unsigned char   SRB_Lun;
    unsigned char   SRB_DriveFlags;
    unsigned char   SRB_Int13HDriveInfo;
    unsigned char   SRB_Heads;
    unsigned char   SRB_Sectors;
    unsigned char   SRB_Rsvd1[22];
};

struct SRB_GDEVBlock
{
    unsigned char SRB_Cmd;
    unsigned char SRB_Status;
    unsigned char SRB_HaId;
    unsigned char SRB_Flags;
    unsigned long SRB_Hdr_Rsvd;
    unsigned char SRB_Target;
    unsigned char SRB_Lun;
    unsigned char SRB_DeviceType;
    unsigned char SRB_Rsvd1;
};

struct SRB_ExecSCSICmd
{
    unsigned char   SRB_Cmd;
    unsigned char   SRB_Status;
    unsigned char   SRB_HaId;
    unsigned char   SRB_Flags;
    unsigned long   SRB_Hdr_Rsvd;
    unsigned char   SRB_Target;
    unsigned char   SRB_Lun;
    unsigned short  SRB_Rsvd1;
    unsigned long   SRB_BufLen;
    unsigned char   *SRB_BufPointer;
    unsigned char   SRB_SenseLen;
    unsigned char   SRB_CDBLen;
    unsigned char   SRB_HaStat;
    unsigned char   SRB_TargStat;
    unsigned long   *SRB_PostProc;
    unsigned char   SRB_Rsvd2[20];
    unsigned char   CDBByte[16];
    unsigned char   SenseArea[SENSE_LEN+2];
};

/*****************************************************************************
          %%% SRB - HOST ADAPTER INQUIRY - SC_HA_INQUIRY (0) %%%
*****************************************************************************/

typedef struct                     // Offset
{                                  // HX/DEC
    BYTE        SRB_Cmd;           // 00/000 ASPI command code = SC_HA_INQUIRY
    BYTE        SRB_Status;        // 01/001 ASPI command status byte
    BYTE        SRB_HaId;          // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;         // 03/003 ASPI request flags
    DWORD       SRB_Hdr_Rsvd;      // 04/004 Reserved, MUST = 0
    BYTE        HA_Count;          // 08/008 Number of host adapters present
    BYTE        HA_SCSI_ID;        // 09/009 SCSI ID of host adapter
    BYTE        HA_ManagerId[16];  // 0A/010 String describing the manager
    BYTE        HA_Identifier[16]; // 1A/026 String describing the host adapter
    BYTE        HA_Unique[16];     // 2A/042 Host Adapter Unique parameters
    WORD        HA_Rsvd1;          // 3A/058 Reserved, MUST = 0
}
SRB_HAInquiry;

const char * wnaspi32_is_cdrom(const char drive_letter);

/*!
  Initialize CD device.
 */
bool wnaspi32_init_win32 (_img_private_t *env);

/*!
   Reads an audio device into data starting from lsn.
   Returns 0 if no error. 
 */
int wnaspi32_read_audio_sectors (_img_private_t *env, void *data, lsn_t lsn, 
			       unsigned int nblocks);
/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int wnaspi32_read_mode2_sector (_img_private_t *env, void *data, lsn_t lsn);

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool wnaspi32_read_toc (_img_private_t *env);

/*!  
  Get format of track. 
*/
track_format_t wnaspi32_get_track_format(_img_private_t *env, 
					 track_t track_num);
