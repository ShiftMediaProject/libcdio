 /* 
  Copyright (c) 2005 Rocky Bernstein <rocky@panix.com>
  Copyright (c) 2001-2002  Ben Fennema <bfennema@falcon.csc.calpoly.edu>
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
/*
 * Some portions taken from FreeBSD ecma167-udf.h which states:
 * Copyright (c) 2001, 2002 Scott Long <scottl@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*!
 * \file ecma_167.h
 *
 * \brief Definitions based on ECMA-167 3rd edition (June 1997)
 * See http://www.ecma.ch
*/

#ifndef _ECMA_167_H
#define _ECMA_167_H 1

#include <cdio/types.h>

/** Tag identifiers */
typedef enum {
  TAGID_PRI_VOL          =   1,
  TAGID_ANCHOR           =   2,
  TAGID_VOL              =   3,
  TAGID_IMP_VOL          =   4,
  TAGID_PARTITION        =   5,
  TAGID_LOGVOL           =   6,
  TAGID_UNALLOC_SPACE    =   7,
  TAGID_TERM             =   8,
  TAGID_LOGVOL_INTEGRITY =   9,
  TAGID_FSD              = 256,
  TAGID_FID              = 257,
  TAGID_FENTRY           = 261
} tag_id_enum_t ;

PRAGMA_BEGIN_PACKED

/** Character set specification (ECMA 167r3 1/7.2.1) */
struct udf_charspec_s
{
  uint8_t	charset_type;
  uint8_t	charset_info[63];
} GNUC_PACKED;

typedef struct udf_charspec_s udf_charspec_t;


/** Character Set Type (ECMA 167r3 1/7.2.1.1) */
#define CHARSPEC_TYPE_CS0		0x00	/** (1/7.2.2) */
#define CHARSPEC_TYPE_CS1		0x01	/** (1/7.2.3) */
#define CHARSPEC_TYPE_CS2		0x02	/** (1/7.2.4) */
#define CHARSPEC_TYPE_CS3		0x03	/** (1/7.2.5) */
#define CHARSPEC_TYPE_CS4		0x04	/** (1/7.2.6) */
#define CHARSPEC_TYPE_CS5		0x05	/** (1/7.2.7) */
#define CHARSPEC_TYPE_CS6		0x06	/** (1/7.2.8) */
#define CHARSPEC_TYPE_CS7		0x07	/** (1/7.2.9) */
#define CHARSPEC_TYPE_CS8		0x08	/** (1/7.2.10) */


/** FIXME move into <cdio/types.h>?  ***/
typedef uint8_t	 dstring;

typedef uint16_t le16_t;
typedef uint32_t le32_t;
typedef uint64_t le64_t;

/** Timestamp (ECMA 167r3 1/7.3) */
struct udf_time_s
{
  le16_t		type_tz;
  le16_t		year;
  uint8_t		month;
  uint8_t		day;
  uint8_t		hour;
  uint8_t		minute;
  uint8_t		second;
  uint8_t		centiseconds;
  uint8_t		hundreds_of_microseconds;
  uint8_t		microseconds;
} GNUC_PACKED;

typedef struct udf_time_s udf_time_t;

/** Type and Time Zone (ECMA 167r3 1/7.3.1) */
#define TIMESTAMP_TYPE_MASK		0xF000
#define TIMESTAMP_TYPE_CUT		0x0000
#define TIMESTAMP_TYPE_LOCAL		0x1000
#define TIMESTAMP_TYPE_AGREEMENT	0x2000
#define TIMESTAMP_TIMEZONE_MASK		0x0FFF

/** Entity identifier (ECMA 167r3 1/7.4) */
#define	UDF_REGID_ID_SIZE	23
struct regid_s
{
  uint8_t		flags;
  uint8_t		id[UDF_REGID_ID_SIZE];
  uint8_t		id_suffix[8];
} GNUC_PACKED;

typedef struct regid_s regid_t;

/** Flags (ECMA 167r3 1/7.4.1) */
#define ENTITYID_FLAGS_DIRTY		0x00
#define ENTITYID_FLAGS_PROTECTED	0x01

/** Volume Structure Descriptor (ECMA 167r3 2/9.1) */
#define VSD_STD_ID_LEN			5
struct vol_struct_desc_s
{
  uint8_t		struct_type;
  uint8_t		std_id[VSD_STD_ID_LEN];
  uint8_t		struct_version;
  uint8_t		struct_data[2041];
} GNUC_PACKED;

/** Standard Identifier (EMCA 167r2 2/9.1.2) */
#define VSD_STD_ID_NSR02		"NSR02"	/* (3/9.1) */

/** Standard Identifier (ECMA 167r3 2/9.1.2) */
#define VSD_STD_ID_BEA01		"BEA01"	/**< (2/9.2) */
#define VSD_STD_ID_BOOT2		"BOOT2"	/**< (2/9.4) */
#define VSD_STD_ID_CD001		"CD001"	/**< (ECMA-119) */
#define VSD_STD_ID_CDW02		"CDW02"	/**< (ECMA-168) */
#define VSD_STD_ID_NSR03		"NSR03"	/**< (3/9.1) */
#define VSD_STD_ID_TEA01		"TEA01"	/**< (2/9.3) */

/** Beginning Extended Area Descriptor (ECMA 167r3 2/9.2) */
struct beginning_extended_area_desc_s
{
  uint8_t		struct_type;
  uint8_t		std_id[VSD_STD_ID_LEN];
  uint8_t		struct_version;
  uint8_t		struct_data[2041];
} GNUC_PACKED;

/** Terminating Extended Area Descriptor (ECMA 167r3 2/9.3) */
struct terminating_extended_area_desc_s
{
  uint8_t		struct_type;
  uint8_t		std_id[VSD_STD_ID_LEN];
  uint8_t		struct_version;
  uint8_t		struct_data[2041];
} GNUC_PACKED;

/** Boot Descriptor (ECMA 167r3 2/9.4) */
struct boot_desc_s
{
  uint8_t		struct_type;
  uint8_t		std_ident[VSD_STD_ID_LEN];
  uint8_t		struct_version;
  uint8_t		reserved1;
  regid_t		arch_type;
  regid_t		boot_ident;
  le32_t		bool_ext_location;
  le32_t		bool_ext_length;
  le64_t		load_address;
  le64_t		start_address;
  udf_time_t	        desc_creation_time;
  le16_t		flags;
  uint8_t		reserved2[32];
  uint8_t		boot_use[1906];
} GNUC_PACKED;

/** Flags (ECMA 167r3 2/9.4.12) */
#define BOOT_FLAGS_ERASE		0x01

/** Extent Descriptor (ECMA 167r3 3/7.1) */
struct extent_ad_s
{
  le32_t		len;
  le32_t		loc;
} GNUC_PACKED;

typedef struct extent_ad_s extent_ad_t;

/** Descriptor Tag (ECMA 167r3 3/7.2) */
struct desc_tag_s
{
  le16_t		tag_id;
  le16_t		desc_version;
  uint8_t		cksum;
  uint8_t		reserved;
  le16_t		i_serial;
  le16_t		desc_CRC;
  le16_t		desc_CRC_len;
  le32_t		tag_loc;
} GNUC_PACKED;

typedef struct desc_tag_s desc_tag_t;


/** Tag Identifier (ECMA 167r3 3/7.2.1) */
#define TAG_IDENT_PVD			0x0001
#define TAG_IDENT_AVDP			0x0002
#define TAG_IDENT_VDP			0x0003
#define TAG_IDENT_IUVD			0x0004
#define TAG_IDENT_PD			0x0005
#define TAG_IDENT_LVD			0x0006
#define TAG_IDENT_USD			0x0007
#define TAG_IDENT_TD			0x0008
#define TAG_IDENT_LVID			0x0009

/** NSR Descriptor (ECMA 167r3 3/9.1) */
struct NSR_desc_s
{
  uint8_t	struct_type;
  uint8_t	std_id[VSD_STD_ID_LEN];
  uint8_t	struct_version;
  uint8_t	reserved;
  uint8_t	struct_data[2040];
} GNUC_PACKED;

/** Primary Volume Descriptor (ECMA 167r3 3/10.1) */
struct udf_pvd_s
{
  desc_tag_t	  tag;
  le32_t	  vol_desc_seq_num;
  le32_t	  primary_vol_desc_num;
  dstring	  vol_ident[32];
  le16_t	  vol_seq_num;
  le16_t	  max_vol_seqnum;
  le16_t	  interchange_lvl;
  le16_t	  max_interchange_lvl;
  le32_t	  charset_list;
  le32_t	  max_charset_list;
  dstring	  volSet_id[128];
  udf_charspec_t  desc_charset;
  udf_charspec_t  explanatory_charset;
  extent_ad_t	  vol_abstract;
  extent_ad_t	  vol_copyright;
  regid_t	  app_ident;
  udf_time_t      recording_time;
  regid_t	  imp_ident;
  uint8_t	  imp_use[64];
  le32_t	  predecessor_vol_desc_seq_location;
  le16_t	  flags;
  uint8_t	  reserved[22];
} GNUC_PACKED;

typedef struct udf_pvd_s udf_pvd_t;

/** Flags (ECMA 167r3 3/10.1.21) */
#define PVD_FLAGS_VSID_COMMON		0x0001

/** Anchor Volume Descriptor Pointer (ECMA 167r3 3/10.2) */
struct anchor_vol_desc_ptr_s
{
  desc_tag_t	tag;
  extent_ad_t	main_vol_desc_seq_ext;
  extent_ad_t	reserve_vol_desc_seq_ext;
  uint8_t	reserved[480];
} GNUC_PACKED;

typedef struct anchor_vol_desc_ptr_s anchor_vol_desc_ptr_t;

/** Volume Descriptor Pointer (ECMA 167r3 3/10.3) */
struct vol_desc_ptr_s
{
  desc_tag_t	tag;
  le32_t	vol_desc_seq_num;
  extent_ad_t	next_vol_desc_set_ext;
  uint8_t	reserved[484];
} GNUC_PACKED;

/** Implementation Use Volume Descriptor (ECMA 167r3 3/10.4) */
struct imp_use_vol_desc_s
{
  desc_tag_t  tag;
  le32_t      vol_desc_seq_num;
  regid_t     imp_id;
  uint8_t     imp_use[460];
} GNUC_PACKED;

/** Partition Descriptor (ECMA 167r3 3/10.5) */
struct partition_desc_s
{
  desc_tag_t    tag;
  le32_t	vol_desc_seq_num;
  le16_t	flags;
  le16_t	partition_num;
  regid_t	contents;
  uint8_t	contents_use[128];
  le32_t	access_type;
  le32_t	start_loc;
  le32_t	part_len;
  regid_t	imp_id;
  uint8_t	imp_use[128];
  uint8_t	reserved[156];
} GNUC_PACKED;

/** Partition Flags (ECMA 167r3 3/10.5.3) */
#define PD_PARTITION_FLAGS_ALLOC	0x0001

/** Partition Contents (ECMA 167r2 3/10.5.3) */
#define PD_PARTITION_CONTENTS_NSR02	"+NSR02"

/** Partition Contents (ECMA 167r3 3/10.5.5) */
#define PD_PARTITION_CONTENTS_FDC01	"+FDC01"
#define PD_PARTITION_CONTENTS_CD001	"+CD001"
#define PD_PARTITION_CONTENTS_CDW02	"+CDW02"
#define PD_PARTITION_CONTENTS_NSR03	"+NSR03"

/** Access Type (ECMA 167r3 3/10.5.7) */
#define PD_ACCESS_TYPE_NONE		0x00000000
#define PD_ACCESS_TYPE_READ_ONLY	0x00000001
#define PD_ACCESS_TYPE_WRITE_ONCE	0x00000002
#define PD_ACCESS_TYPE_REWRITABLE	0x00000003
#define PD_ACCESS_TYPE_OVERWRITABLE	0x00000004

/** Logical Volume Descriptor (ECMA 167r3 3/10.6) */
struct logical_vol_desc_s
{
  desc_tag_t     tag;
  le32_t	 seq_num;
  udf_charspec_t desc_charset;
  dstring	 logvol_id[128];
  le32_t	 logical_blocksize;
  regid_t	 domain_id;
  uint8_t	 logvol_contents_use[16];
  le32_t	 maptable_len;
  le32_t	 i_partition_maps;
  regid_t	 imp_id;
  uint8_t	 imp_use[128];
  extent_ad_t	 integrity_seq_ext;
  uint8_t	 partition_maps[0];
} GNUC_PACKED;

/** Generic Partition Map (ECMA 167r3 3/10.7.1) */
struct generic_partition_map
{
  uint8_t	partition_map_type;
  uint8_t	partition_map_length;
  uint8_t	partition_mapping[0];
} GNUC_PACKED;

/** Partition Map Type (ECMA 167r3 3/10.7.1.1) */
#define GP_PARTITION_MAP_TYPE_UNDEF	0x00
#define GP_PARTIITON_MAP_TYPE_1		0x01
#define GP_PARTITION_MAP_TYPE_2		0x02

/** Type 1 Partition Map (ECMA 167r3 3/10.7.2) */
struct generic_partition_map1
{
  uint8_t	partition_map_type;
  uint8_t	partition_map_length;
  le16_t	vol_seq_num;
  le16_t	i_partition;
} GNUC_PACKED;

/** Type 2 Partition Map (ECMA 167r3 3/10.7.3) */
struct generic_partition_map2
{
  uint8_t	partition_map_type;
  uint8_t	partition_map_length; 
  uint8_t	partition_id[62];
} GNUC_PACKED;

/** Unallocated Space Descriptor (ECMA 167r3 3/10.8) */
struct unalloc_space_desc_s
{
  desc_tag_t    tag;
  le32_t	vol_desc_seq_num;
  le32_t	i_alloc_descs;
  extent_ad_t	allocDescs[0];
} GNUC_PACKED;

/** Terminating Descriptor (ECMA 167r3 3/10.9) */
struct terminating_desc_s
{
  desc_tag_t    tag;
  uint8_t	reserved[496];
} GNUC_PACKED;

/** Logical Volume Integrity Descriptor (ECMA 167r3 3/10.10) */
struct logvol_integrity_desc_s
{
  desc_tag_t   tag;
  udf_time_t   recording_time;
  le32_t       integrity_type;
  extent_ad_t  next_integrity_ext;
  uint8_t      logvol_contents_use[32];
  le32_t       i_partitions;
  le32_t       imp_use_len;
  le32_t       freespace_table[0];
  le32_t       size_table[0];
  uint8_t      imp_use[0];
} GNUC_PACKED;

/** Integrity Type (ECMA 167r3 3/10.10.3) */
#define LVID_INTEGRITY_TYPE_OPEN	0x00000000
#define LVID_INTEGRITY_TYPE_CLOSE	0x00000001

/** Recorded Address (ECMA 167r3 4/7.1) */
struct lb_addr_s
{
  le32_t	logical_blockNum;
  le16_t	partitionReferenceNum;
} GNUC_PACKED;

typedef struct lb_addr_s lb_addr_t;

/** Short Allocation Descriptor (ECMA 167r3 4/14.14.1) */
struct short_ad_s
{
  le32_t	len;
  le32_t	pos;
} GNUC_PACKED;

typedef struct short_ad_s short_ad_t;

/** Long Allocation Descriptor (ECMA 167r3 4/14.14.2) */
struct long_ad_s
{
  le32_t	len;
  lb_addr_t	loc;
  uint8_t	imp_use[6];
} GNUC_PACKED;

typedef struct long_ad_s long_ad_t;

/** Extended Allocation Descriptor (ECMA 167r3 4/14.14.3) */
struct ext_ad_s
{
  le32_t	len;
  le32_t	recorded_len;
  le32_t	information_len;
  lb_addr_t	ext_loc;
} GNUC_PACKED ext_ad;

typedef struct ext_ad_s ext_ad_t;

/** Descriptor Tag (ECMA 167r3 4/7.2 - See 3/7.2) */

/** Tag Identifier (ECMA 167r3 4/7.2.1) */
#define TAG_IDENT_FSD			0x0100
#define TAG_IDENT_FID			0x0101
#define TAG_IDENT_AED			0x0102
#define TAG_IDENT_IE			0x0103
#define TAG_IDENT_TE			0x0104
#define TAG_IDENT_FE			0x0105
#define TAG_IDENT_EAHD			0x0106
#define TAG_IDENT_USE			0x0107
#define TAG_IDENT_SBD			0x0108
#define TAG_IDENT_PIE			0x0109
#define TAG_IDENT_EFE			0x010A

/** File Set Descriptor (ECMA 167r3 4/14.1) */
struct fileset_desc_s
{
  desc_tag_t      tag;
  udf_time_t      recording_time;
  le16_t	  interchange_lvl;
  le16_t	  maxInterchange_lvl;
  le32_t	  charset_list;
  le32_t	  max_charset_list;
  le32_t	  fileset_num;
  le32_t	  fileset_desc_num;
  udf_charspec_t  logical_vol_id_charset;
  dstring	  logical_vol_id[128];
  udf_charspec_t  fileset_charset;
  dstring	  fileSet_id[32];
  dstring	  copyright_file_id[32];
  dstring	  abstract_file_id[32];
  long_ad_t	  root_directory_ICB;
  regid_t	  domain_id;
  long_ad_t	  next_ext;
  long_ad_t	  stream_directory_ICB;
  uint8_t	  reserved[32];
} GNUC_PACKED;

typedef struct fileset_desc_s fileset_desc_t;

/** Partition Header Descriptor (ECMA 167r3 4/14.3) */
struct partition_header_desc_s
{
  short_ad_t	unalloc_space_table;
  short_ad_t	unalloc_space_bitmap;
  short_ad_t	partition_integrity_table;
  short_ad_t	freed_space_table;
  short_ad_t	freed_space_bitmap;
  uint8_t	reserved[88];
} GNUC_PACKED;

typedef struct partition_header_desc_s partition_header_desc_t;

/** File Identifier Descriptor (ECMA 167r3 4/14.4) */
struct fileIdentDesc
{
  desc_tag_t    tag;
  le16_t	file_version_num;
  uint8_t	file_characteristics;
  uint8_t	length_file_id;
  long_ad_t	icb;
  le16_t	i_imp_use;
  uint8_t	imp_use[0];
  uint8_t	file_id[0];
  uint8_t	padding[0];
} GNUC_PACKED;

#define	UDF_FID_SIZE	38
#define	UDF_FILE_CHAR_VIS	(1 << 0) /* Visible */
#define	UDF_FILE_CHAR_DIR	(1 << 1) /* Directory */
#define	UDF_FILE_CHAR_DEL	(1 << 2) /* Deleted */
#define	UDF_FILE_CHAR_PAR	(1 << 3) /* Parent Directory */
#define	UDF_FILE_CHAR_META	(1 << 4) /* Stream metadata */

/** File Characteristics (ECMA 167r3 4/14.4.3) */
#define FID_FILE_CHAR_HIDDEN		0x01
#define FID_FILE_CHAR_DIRECTORY		0x02
#define FID_FILE_CHAR_DELETED		0x04
#define FID_FILE_CHAR_PARENT		0x08
#define FID_FILE_CHAR_METADATA		0x10

/** Allocation Ext Descriptor (ECMA 167r3 4/14.5) */
struct allocExtDesc
{
  desc_tag_t   tag;
  le32_t       previous_alloc_ext_loc;
  le32_t       i_alloc_descs;
} GNUC_PACKED;

/** ICB Tag (ECMA 167r3 4/14.6) */
struct icbtag_s
{
  le32_t	prev_num_dirs;
  le16_t	strat_type;
  le16_t	strat_param;
  le16_t	max_num_entries;
  uint8_t	reserved;
  uint8_t	file_type;
  lb_addr_t	parent_ICB;
  le16_t	flags;
} GNUC_PACKED;

typedef struct icbtag_s icbtag_t;

#define	UDF_ICB_TAG_FLAGS_SETUID	0x40
#define	UDF_ICB_TAG_FLAGS_SETGID	0x80
#define	UDF_ICB_TAG_FLAGS_STICKY	0x100

/** Strategy Type (ECMA 167r3 4/14.6.2) */
#define ICBTAG_STRATEGY_TYPE_UNDEF	0x0000
#define ICBTAG_STRATEGY_TYPE_1		0x0001
#define ICBTAG_STRATEGY_TYPE_2		0x0002
#define ICBTAG_STRATEGY_TYPE_3		0x0003
#define ICBTAG_STRATEGY_TYPE_4		0x0004

/** File Type (ECMA 167r3 4/14.6.6) */
#define ICBTAG_FILE_TYPE_UNDEF		0x00
#define ICBTAG_FILE_TYPE_USE		0x01
#define ICBTAG_FILE_TYPE_PIE		0x02
#define ICBTAG_FILE_TYPE_IE		0x03
#define ICBTAG_FILE_TYPE_DIRECTORY	0x04
#define ICBTAG_FILE_TYPE_REGULAR	0x05
#define ICBTAG_FILE_TYPE_BLOCK		0x06
#define ICBTAG_FILE_TYPE_CHAR		0x07
#define ICBTAG_FILE_TYPE_EA		0x08
#define ICBTAG_FILE_TYPE_FIFO		0x09
#define ICBTAG_FILE_TYPE_SOCKET		0x0A
#define ICBTAG_FILE_TYPE_TE		0x0B
#define ICBTAG_FILE_TYPE_SYMLINK	0x0C
#define ICBTAG_FILE_TYPE_STREAMDIR	0x0D

/** Flags (ECMA 167r3 4/14.6.8) */
#define ICBTAG_FLAG_AD_MASK		0x0007
#define ICBTAG_FLAG_AD_SHORT		0x0000
#define ICBTAG_FLAG_AD_LONG		0x0001
#define ICBTAG_FLAG_AD_EXTENDED		0x0002
#define ICBTAG_FLAG_AD_IN_ICB		0x0003
#define ICBTAG_FLAG_SORTED		0x0008
#define ICBTAG_FLAG_NONRELOCATABLE	0x0010
#define ICBTAG_FLAG_ARCHIVE		0x0020
#define ICBTAG_FLAG_SETUID		0x0040
#define ICBTAG_FLAG_SETGID		0x0080
#define ICBTAG_FLAG_STICKY		0x0100
#define ICBTAG_FLAG_CONTIGUOUS		0x0200
#define ICBTAG_FLAG_SYSTEM		0x0400
#define ICBTAG_FLAG_TRANSFORMED		0x0800
#define ICBTAG_FLAG_MULTIVERSIONS	0x1000
#define ICBTAG_FLAG_STREAM		0x2000

/** Indirect Entry (ECMA 167r3 4/14.7) */
struct indirect_entry_s
{
  desc_tag_t      tag;
  icbtag_t	  icb_tag;
  long_ad_t	  indirect_ICB;
} GNUC_PACKED;

/** Terminal Entry (ECMA 167r3 4/14.8) */
struct terminal_entry_s
{
  desc_tag_t      tag;
  icbtag_t	  icb_tag;
} GNUC_PACKED;

/** File Entry (ECMA 167r3 4/14.9) */
struct file_entry_s
{
  desc_tag_t      tag;
  icbtag_t	  icb_tag;
  le32_t	  uid;
  le32_t	  gid;
  le32_t	  permissions;
  le16_t	  link_count;
  uint8_t	  rec_format;
  uint8_t	  rec_disp_attr;
  le32_t	  rec_len;
  le64_t	  info_len;
  le64_t	  logblks_recorded;
  udf_time_t      access_time;
  udf_time_t      modification_time;
  udf_time_t      attr_time;
  le32_t	  checkpoint;
  long_ad_t	  ext_attr_ICB;
  regid_t	  imp_id;
  le64_t	  unique_iD;
  le32_t	  length_extended_attr;
  le32_t	  length_alloc_descs;
  uint8_t	  ext_attr[0];
  uint8_t	  alloc_descs[0];
} GNUC_PACKED;

#define	UDF_FENTRY_SIZE	176
#define	UDF_FENTRY_PERM_USER_MASK	0x07
#define	UDF_FENTRY_PERM_GRP_MASK	0xE0
#define	UDF_FENTRY_PERM_OWNER_MASK	0x1C00

/** Permissions (ECMA 167r3 4/14.9.5) */
#define FE_PERM_O_EXEC			0x00000001U
#define FE_PERM_O_WRITE			0x00000002U
#define FE_PERM_O_READ			0x00000004U
#define FE_PERM_O_CHATTR		0x00000008U
#define FE_PERM_O_DELETE		0x00000010U
#define FE_PERM_G_EXEC			0x00000020U
#define FE_PERM_G_WRITE			0x00000040U
#define FE_PERM_G_READ			0x00000080U
#define FE_PERM_G_CHATTR		0x00000100U
#define FE_PERM_G_DELETE		0x00000200U
#define FE_PERM_U_EXEC			0x00000400U
#define FE_PERM_U_WRITE			0x00000800U
#define FE_PERM_U_READ			0x00001000U
#define FE_PERM_U_CHATTR		0x00002000U
#define FE_PERM_U_DELETE		0x00004000U

/** Record Format (ECMA 167r3 4/14.9.7) */
#define FE_RECORD_FMT_UNDEF		0x00
#define FE_RECORD_FMT_FIXED_PAD		0x01
#define FE_RECORD_FMT_FIXED		0x02
#define FE_RECORD_FMT_VARIABLE8		0x03
#define FE_RECORD_FMT_VARIABLE16	0x04
#define FE_RECORD_FMT_VARIABLE16_MSB	0x05
#define FE_RECORD_FMT_VARIABLE32	0x06
#define FE_RECORD_FMT_PRINT		0x07
#define FE_RECORD_FMT_LF		0x08
#define FE_RECORD_FMT_CR		0x09
#define FE_RECORD_FMT_CRLF		0x0A
#define FE_RECORD_FMT_LFCR		0x0B

/** Record Display Attributes (ECMA 167r3 4/14.9.8) */
#define FE_RECORD_DISPLAY_ATTR_UNDEF	0x00
#define FE_RECORD_DISPLAY_ATTR_1	0x01
#define FE_RECORD_DISPLAY_ATTR_2	0x02
#define FE_RECORD_DISPLAY_ATTR_3	0x03

/** Extended Attribute Header Descriptor (ECMA 167r3 4/14.10.1) */
struct extended_attr_header_desc_s
{
  desc_tag_t      tag;
  le32_t	  imp_attr_location;
  le32_t	  app_attr_location;
} GNUC_PACKED;

/** Generic Format (ECMA 167r3 4/14.10.2) */
struct generic_format_s
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  uint8_t	attrData[0];
} GNUC_PACKED;

/** Character Set Information (ECMA 167r3 4/14.10.3) */
struct charSet_info_s
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le32_t	escapeSeqLength;
  uint8_t	charSetType;
  uint8_t	escapeSeq[0];
} GNUC_PACKED;

/* Alternate Permissions (ECMA 167r3 4/14.10.4) */
struct alt_perms_s
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le16_t	owner_id;
  le16_t	group_id;
  le16_t	permission;
} GNUC_PACKED;

/** File Times Extended Attribute (ECMA 167r3 4/14.10.5) */
struct filetimes_ext_attr_s
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le32_t	dataLength;
  le32_t	fileTimeExistence;
  uint8_t	fileTimes;
} GNUC_PACKED;

/** FileTimeExistence (ECMA 167r3 4/14.10.5.6) */
#define FTE_CREATION			0x00000001
#define FTE_DELETION			0x00000004
#define FTE_EFFECTIVE			0x00000008
#define FTE_BACKUP			0x00000002

/** Information Times Extended Attribute (ECMA 167r3 4/14.10.6) */
struct infoTimesExtAttr
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le32_t	dataLength;
  le32_t	infoTimeExistence;
  uint8_t	infoTimes[0];
} GNUC_PACKED;

/** Device Specification (ECMA 167r3 4/14.10.7) */
struct deviceSpec
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le32_t	imp_useLength;
  le32_t	majorDevice_id;
  le32_t	minorDevice_id;
  uint8_t	imp_use[0];
} GNUC_PACKED;

/** Implementation Use Extended Attr (ECMA 167r3 4/14.10.8) */
struct impUseExtAttr
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le32_t	imp_useLength;
  regid_t	imp_id;
  uint8_t	imp_use[0];
} GNUC_PACKED;

/** Application Use Extended Attribute (ECMA 167r3 4/14.10.9) */
struct appUseExtAttr
{
  le32_t	attr_type;
  uint8_t	attr_subtype;
  uint8_t	reserved[3];
  le32_t	attrLength;
  le32_t	appUseLength;
  regid_t	app_id;
  uint8_t	appUse[0];
} GNUC_PACKED;

#define EXTATTR_CHAR_SET		1
#define EXTATTR_ALT_PERMS		3
#define EXTATTR_FILE_TIMES		5
#define EXTATTR_INFO_TIMES		6
#define EXTATTR_DEV_SPEC		12
#define EXTATTR_IMP_USE			2048
#define EXTATTR_APP_USE			65536


/** Unallocated Space Entry (ECMA 167r3 4/14.11) */
struct unallocSpaceEntry
{
  desc_tag_t    tag;
  icbtag_t	icb_tag;
  le32_t	lengthAllocDescs;
  uint8_t	allocDescs[0];
} GNUC_PACKED;

/** Space Bitmap Descriptor (ECMA 167r3 4/14.12) */
struct spaceBitmapDesc
{
  desc_tag_t      tag;
  le32_t	  i_bits;
  le32_t	  i_bytes;
  uint8_t	  bitmap[0];
} GNUC_PACKED;

/** Partition Integrity Entry (ECMA 167r3 4/14.13) */
struct partitionIntegrityEntry
{
  desc_tag_t      tag;
  icbtag_t	  icb_tag;
  udf_time_t      recording_time;
  uint8_t	  integrityType;
  uint8_t	  reserved[175];
  regid_t	  imp_id;
  uint8_t	  imp_use[256];
} GNUC_PACKED;

/** Short Allocation Descriptor (ECMA 167r3 4/14.14.1) */

/** Extent Length (ECMA 167r3 4/14.14.1.1) */
#define EXT_RECORDED_ALLOCATED		0x00000000
#define EXT_NOT_RECORDED_ALLOCATED	0x40000000
#define EXT_NOT_RECORDED_NOT_ALLOCATED	0x80000000
#define EXT_NEXT_EXTENT_ALLOCDECS	0xC0000000

/** Long Allocation Descriptor (ECMA 167r3 4/14.14.2) */

/** Extended Allocation Descriptor (ECMA 167r3 4/14.14.3) */

/** Logical Volume Header Descriptor (ECMA 167r3 4/14.15) */
struct logical_vol_header_desc_s 
{
  le64_t	uniqueID;
  uint8_t	reserved[24];
} GNUC_PACKED;

typedef struct logical_vol_header_desc_s logical_vol_header_desc_t;

/** Path Component (ECMA 167r3 4/14.16.1) */
struct pathComponent
{
  uint8_t	component_type;
  uint8_t	lengthComponent_id;
  le16_t	componentFileVersionNum;
  dstring	component_id[0];
} GNUC_PACKED;

/** File Entry (ECMA 167r3 4/14.17) */
struct extended_file_entry
{
  desc_tag_t tag;
  icbtag_t   icb_tag;
  le32_t     uid;
  le32_t     gid;
  le32_t     permissions;
  le16_t     link_count;
  uint8_t    rec_format;
  uint8_t    rec_display_attr;
  le32_t     record_len;
  le64_t     info_len;
  le64_t     object_size;
  le64_t     logblks_recorded;
  udf_time_t access_time;
  udf_time_t modification_time;
  udf_time_t create_time;
  udf_time_t attr_time;
  le32_t     checkpoint;
  le32_t     reserved;
  long_ad_t  ext_attr_ICB;
  long_ad_t  stream_directory_ICB;
  regid_t    imp_id;
  le64_t     unique_ID;
  le32_t     length_extended_attr;
  le32_t     length_alloc_descs;
  uint8_t    ext_attr[0];
  uint8_t    alloc_descs[0];
} GNUC_PACKED;

PRAGMA_END_PACKED

#endif /* _ECMA_167_H */
