/* -*- c -*-
  $Id: pyiso9660.swg,v 1.14 2008/05/01 16:55:05 karl Exp $

  Copyright (C) 2006, 2008 Rocky Bernstein <rocky@gnu.org>

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
%define DOCSTRING 
"This is a wrapper for The CD Input and Control library's ISO-9660 library
See also the ISO-9660 specification. The freely available European
equivalant standard is called ECMA-119."
%enddef
%module(docstring=DOCSTRING) pyiso9660

%{
/* Includes the header in the wrapper code */
#include <time.h>
#include <cdio/iso9660.h>
#include <cdio/version.h>
%}

#include <time.h>
#include <cdio/iso9660.h>
#include <cdio/version.h>

/* Various libcdio constants and typedefs */
%include "types.swg"
%include "compat.swg"

%include "typemaps.i"
%include "cstring.i"

%feature("autodoc", 1);

typedef unsigned int iso_extension_mask_t;
typedef unsigned int uint8_t;
typedef unsigned int uint16_t;

%constant long int ISO_BLOCKSIZE        = CDIO_CD_FRAMESIZE;
%constant long int PVD_SECTOR 	        = ISO_PVD_SECTOR ;
%constant long int EVD_SECTOR 	        = ISO_EVD_SECTOR ;
%constant long int LEN_ISONAME 	        = LEN_ISONAME;
%constant long int MAX_SYSTEM_ID 	= ISO_MAX_SYSTEM_ID ;
%constant long int MAX_ISONAME 	        = MAX_ISONAME;
%constant long int MAX_PREPARER_ID 	= ISO_MAX_PREPARER_ID ;
%constant long int MAX_ISOPATHNAME 	= MAX_ISOPATHNAME;

%constant long int FILE                 = ISO_FILE;
%constant long int EXISTENCE            = ISO_EXISTENCE;
%constant long int DIRECTORY            = ISO_DIRECTORY;
%constant long int ASSOCIATED           = ISO_ASSOCIATED;
%constant long int RECORD               = ISO_RECORD;

#include <cdio/version.h>
#if LIBCDIO_VERSION_NUM <= 76
%constant long int PROTECTION           = ISO_PROTECTION;
#else 
%constant long int PROTECTION           = 16;
#endif

%constant long int DRESERVED1           = ISO_DRESERVED1;
%constant long int DRESERVED2           = ISO_DRESERVED2;
%constant long int MULTIEXTENT          = ISO_MULTIEXTENT;

%constant long int VD_BOOT_RECORD       = ISO_VD_BOOT_RECORD;
%constant long int VD_PRIMARY           = ISO_VD_PRIMARY;
%constant long int VD_SUPPLEMENTARY     = ISO_VD_SUPPLEMENTARY;
%constant long int VD_PARITION          = ISO_VD_PARITION;
%constant long int VD_END               = ISO_VD_END;

%constant long int MAX_PUBLISHER_ID     = ISO_MAX_PUBLISHER_ID;
%constant long int MAX_APPLICATION_ID   = ISO_MAX_APPLICATION_ID;
%constant long int MAX_VOLUME_ID        = ISO_MAX_VOLUME_ID;
%constant long int MAX_VOLUMESET_ID     = ISO_MAX_VOLUMESET_ID;
%constant long int STANDARD_ID          = ISO_STANDARD_ID;

%constant long int NOCHECK              = ISO9660_NOCHECK;
%constant long int SEVEN_BIT            = ISO9660_7BIT;
%constant long int ACHARS               = ISO9660_ACHARS;
%constant long int DCHARS               = ISO9660_DCHARS;

%constant long int EXTENSION_JOLIET_LEVEL1 = ISO_EXTENSION_JOLIET_LEVEL1;
%constant long int EXTENSION_JOLIET_LEVEL2 = ISO_EXTENSION_JOLIET_LEVEL2;
%constant long int EXTENSION_JOLIET_LEVEL3 = ISO_EXTENSION_JOLIET_LEVEL3;
%constant long int EXTENSION_ROCK_RIDGE    = ISO_EXTENSION_ROCK_RIDGE;
%constant long int EXTENSION_HIGH_SIERRA   = ISO_EXTENSION_HIGH_SIERRA;

%constant long int EXTENSION_ALL           = ISO_EXTENSION_ALL;
%constant long int EXTENSION_NONE          = ISO_EXTENSION_NONE;
%constant long int EXTENSION_JOLIET        = ISO_EXTENSION_JOLIET;

/* Set up to allow functions to return stat lists of type "stat
   *". We'll use a typedef so we can make sure to isolate this. 
*/
%inline %{
typedef CdioList_t IsoStatList_t;
typedef iso9660_stat_t IsoStat_t;
%}
typedef CdioList_t IsoStatList_t;
typedef iso9660_stat_t IsoStat_t;

%typemap(newfree) IsoStatList_t "_cdio_list_free($1, 1);";

%typemap(out) IsoStatList_t *{   
    CdioList_t *p_entlist   = result;
    CdioListNode_t *p_entnode;

    if (result) {
      resultobj = PyList_New(0);
      _CDIO_LIST_FOREACH (p_entnode, p_entlist) {
	PyObject *py_list = PyList_New(5);
	iso9660_stat_t *p_statbuf = 
	  (iso9660_stat_t *) _cdio_list_node_data (p_entnode);
	PyObject *o;
	o = PyString_FromString(p_statbuf->filename);
	PyList_SetItem(py_list, 0, o);
	o = SWIG_From_long((long)(p_statbuf->lsn)); 
	PyList_SetItem(py_list, 1, o);
	o = SWIG_From_long((long)(p_statbuf->size)); 
	PyList_SetItem(py_list, 2, o);
	o = SWIG_From_long((long)(p_statbuf->secsize)); 
	PyList_SetItem(py_list, 3, o);
	o = SWIG_From_long((long)(p_statbuf->type)); 
	PyList_SetItem(py_list, 4, o);
	PyList_Append(resultobj, py_list);
      }
    }
}
%typemap(out) IsoStat_t *{   
    iso9660_stat_t *p_statbuf = result;

    if (result) {
      PyObject *o;
      resultobj = PyList_New(0);
      o = PyString_FromString(p_statbuf->filename);
      PyList_Append(resultobj, o);
      o = SWIG_From_long((long)(p_statbuf->lsn)); 
      PyList_Append(resultobj, o);
      o = SWIG_From_long((long)(p_statbuf->size)); 
      PyList_Append(resultobj, o);
      o = SWIG_From_long((long)(p_statbuf->secsize)); 
      PyList_Append(resultobj, o);
      o = SWIG_From_long((long)(p_statbuf->type)); 
      PyList_Append(resultobj, o);
      free (p_statbuf);
    }
}

%typemap(out) struct tm *{   
    struct tm *p_tm = result;

    if (result) {
      PyObject *o;
      resultobj = PyList_New(9);
      o = SWIG_From_int(p_tm->tm_year+1900);
      PyList_SetItem(resultobj, 0, o);
      o = SWIG_From_int(p_tm->tm_mon+1);
      PyList_SetItem(resultobj, 1, o);
      o = SWIG_From_int(p_tm->tm_mday);
      PyList_SetItem(resultobj, 2, o);
      o = SWIG_From_int(p_tm->tm_hour);
      PyList_SetItem(resultobj, 3, o);
      o = SWIG_From_int(p_tm->tm_min);
      PyList_SetItem(resultobj, 4, o);
      o = SWIG_From_int(p_tm->tm_sec);
      PyList_SetItem(resultobj, 5, o);
      o = SWIG_From_int((p_tm->tm_wday-1)%7);
      PyList_SetItem(resultobj, 6, o);
      o = SWIG_From_int(p_tm->tm_yday+1);
      PyList_SetItem(resultobj, 7, o);
      o = SWIG_From_int(p_tm->tm_isdst);
      PyList_SetItem(resultobj, 8, o);
      free (p_tm);
    }
}

#ifdef FINISHED
%typemap(out) IsoStatList_t *{   
    // $1 is of type IsoStatList_t
    CdioList_t *p_entlist   = result;
    CdioListNode_t *p_entnode;
    unsigned int num = 0;

    if (!result) goto out;

 PPCODE:
    /* Figure out how many items in the array */
    _CDIO_LIST_FOREACH (p_entnode, p_entlist) {
      num +=5;
    }

    /* For each element in the array of strings, create a new
     * mortalscalar, and stuff it into the above array. */
    _CDIO_LIST_FOREACH (p_entnode, p_entlist) {
      iso9660_stat_t *p_statbuf = 
	(iso9660_stat_t *) _cdio_list_node_data (p_entnode);
      /* Have perl compute the length of the string using strlen() */
      XPUSHs(sv_2mortal(newSVpv(p_statbuf->filename, 0)));
      XPUSHs(sv_2mortal(newSViv(p_statbuf->lsn)));
      XPUSHs(sv_2mortal(newSViv(p_statbuf->size)));
      XPUSHs(sv_2mortal(newSViv(p_statbuf->secsize)));
      XPUSHs(sv_2mortal(newSViv(p_statbuf->type)));
    }
    _cdio_list_free (p_entlist, true);
    argvi += num + 2;
 out: ;
}
#endif

%feature("autodoc",
"open_iso(path)
 Open an ISO 9660 image for reading. Maybe in the future we will have
 mode. None is returned on error.
");
%rename iso9660_open open_iso;
iso9660_t *iso9660_open (const char *psz_path /*flags, mode */);

%feature("autodoc",
"Open an ISO 9660 image for reading allowing various ISO 9660
 extensions.  Maybe in the future we will have a mode. None is
 returned on error.");
%rename iso9660_open_ext open_ext;
iso9660_t *iso9660_open_ext (const char *psz_path, 
			     iso_extension_mask_t iso_extension_mask);

%feature("autodoc",
"Open an ISO 9660 image for reading with some tolerence for positioning
of the ISO9660 image. We scan for ISO_STANDARD_ID and use that to set
the eventual offset to adjust by (as long as that is <= i_fuzz).

Maybe in the future we will have a mode. None is returned on error.

see iso9660_open");
%rename iso9660_open_fuzzy open_fuzzy;
iso9660_t *iso9660_open_fuzzy (const char *psz_path /*flags, mode */,
			       uint16_t i_fuzz);

%feature("autodoc",
"Open an ISO 9660 image for reading with some tolerence for positioning
of the ISO9660 image. We scan for ISO_STANDARD_ID and use that to set
the eventual offset to adjust by (as long as that is <= i_fuzz).

Maybe in the future we will have a mode. None is returned on error.

see open_iso
");
%rename iso9660_open_fuzzy open_fuzzy_ext;
iso9660_t *iso9660_open_fuzzy_ext (const char *psz_path,
				   iso_extension_mask_t iso_extension_mask,
				   uint16_t i_fuzz
				   /*flags, mode */);
%feature("autodoc",
"Read the Super block of an ISO 9660 image but determine framesize
and datastart and a possible additional offset. Generally here we are
not reading an ISO 9660 image but a CD-Image which contains an ISO 9660
filesystem.
");
%rename iso9660_ifs_fuzzy_read_superblock ifs_fuzzy_read_superblock;
bool iso9660_ifs_fuzzy_read_superblock (iso9660_t *p_iso, 
					iso_extension_mask_t iso_extension_mask,
					uint16_t i_fuzz);
%feature("autodoc",
"Close previously opened ISO 9660 image.
True is unconditionally returned. If there was an error false would
be returned.");
%rename iso9660_close close;
bool iso9660_close (iso9660_t * p_iso);

%feature("autodoc",
"Seek to a position and then read n bytes. (buffer, size) are
 returned.");
%cstring_output_withsize(char *p_buf, ssize_t *pi_size);
ssize_t seek_read (const iso9660_t *p_iso, 
		   lsn_t start, char *p_buf, ssize_t *pi_size);
%inline %{
ssize_t 
seek_read (const iso9660_t *p_iso, lsn_t start, char *p_buf, 
	   ssize_t *pi_size) 
{
  *pi_size = iso9660_iso_seek_read(p_iso, p_buf, start, 
				   (*pi_size) / ISO_BLOCKSIZE);
  return *pi_size;
 }
%}


%feature("autodoc",
"Read the Primary Volume Descriptor for a CD.
None  is returned if there was an error.");
iso9660_pvd_t *fs_read_pvd ( const CdIo_t *p_cdio );
%inline %{
iso9660_pvd_t *fs_read_pvd ( const CdIo_t *p_cdio ) {
  static iso9660_pvd_t pvd;
  bool b_ok = iso9660_fs_read_pvd ( p_cdio, &pvd );
  if (!b_ok) return NULL;
  return &pvd;
 }
%}
 
%feature("autodoc",
"Read the Primary Volume Descriptor for an ISO 9660 image.
None is returned if there was an error.");
iso9660_pvd_t *ifs_read_pvd ( const iso9660_t *p_iso );
%inline %{
iso9660_pvd_t *ifs_read_pvd ( const iso9660_t *p_iso ) {
  static iso9660_pvd_t pvd;
  bool b_ok = iso9660_ifs_read_pvd ( p_iso, &pvd );
  if (!b_ok) return NULL;
  return &pvd;
 }
%}

%feature("autodoc",
"Read the Super block of an ISO 9660 image. This is the 
Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume 
Descriptor if (Joliet) extensions are acceptable.");
%rename iso9660_fs_read_superblock fs_read_superblock;
bool iso9660_fs_read_superblock (CdIo_t *p_cdio, 
				 iso_extension_mask_t iso_extension_mask);

%feature("autodoc",
"Read the Super block of an ISO 9660 image. This is the 
 Primary Volume Descriptor (PVD) and perhaps a Supplemental Volume 
 Descriptor if (Joliet) extensions are acceptable.");
%rename iso9660_ifs_read_superblock ifs_read_superblock;
bool iso9660_ifs_read_superblock (iso9660_t *p_iso,
				  iso_extension_mask_t iso_extension_mask);


/*====================================================
  Time conversion 
 ====================================================*/
%feature("autodoc",
"Set time in format used in ISO 9660 directory index record");
iso9660_dtime_t *
set_dtime ( int year, int mon,  int mday, int hour, int min, int sec);
%inline %{
iso9660_dtime_t *
set_dtime ( int year, int mon,  int mday, int hour, int min, int sec)
{
  struct tm tm = { sec,  min, hour, mday, mon-1, year-1900, 0, 0, -1 };
  static iso9660_dtime_t dtime;
  iso9660_set_dtime (&tm, &dtime);
  return &dtime;
}
%}

%feature("autodoc",
"Set 'long' time in format used in ISO 9660 primary volume descriptor");
iso9660_ltime_t *
set_ltime ( int year, int mon,  int mday, int hour, int min, int sec);

%inline %{
iso9660_ltime_t *
set_ltime ( int year, int mon,  int mday, int hour, int min, int sec)
 {
   struct tm tm = { sec, min, hour, mday, mon-1, year-1900, 0, 0, -1 };
  static iso9660_ltime_t ldate;
  iso9660_set_ltime (&tm, &ldate);
  return &ldate;
}
%}

%feature("autodoc",
"Get Unix time structure from format use in an ISO 9660 directory index 
record. Even though tm_wday and tm_yday fields are not explicitly in
idr_date, they are calculated from the other fields.

If tm is to reflect the localtime, set 'use_localtime' true, otherwise
tm will reported in GMT.");
struct tm *get_dtime (const iso9660_dtime_t *p_dtime, bool use_localtime);
%inline %{
struct tm *get_dtime (const iso9660_dtime_t *p_dtime, bool use_localtime) {
  struct tm *p_tm = (struct tm *) calloc(1, sizeof(struct tm));
  if (!iso9660_get_dtime (p_dtime, use_localtime, p_tm)) {
    free(p_tm);
    return NULL;
  }
  return p_tm;
 }
%}

%feature("autodoc",
"Get 'long' time in format used in ISO 9660 primary volume descriptor
 from a Unix time structure.");
struct tm *get_ltime (const iso9660_ltime_t *p_ltime);
%inline %{
struct tm *get_ltime (const iso9660_ltime_t *p_ltime)
{
  struct tm *p_tm = (struct tm *) calloc(1, sizeof(struct tm));
  if (!iso9660_get_ltime (p_ltime, p_tm)) {
    free(p_tm);
    return NULL;
  }
  return p_tm;
}
%}

/*========================================================
  Characters used in file and directory and manipulation
 =======================================================*/
%feature("autodoc",
" Return true if c is a DCHAR - a character that can appear in an an
 ISO-9600 level 1 directory name. These are the ASCII capital
 letters A-Z, the digits 0-9 and an underscore.");
%rename iso9660_isdchar is_dchar;
bool iso9660_isdchar (int c);

%feature("autodoc",
"Return true if c is an ACHAR - 
 These are the DCHAR's plus some ASCII symbols including the space 
 symbol.");
%rename iso9660_isachar is_achar;
bool iso9660_isachar (int c);

%feature("autodoc",
"Convert an ISO-9660 file name that stored in a directory entry into 
 what's usually listed as the file name in a listing.
 Lowercase name, and remove trailing ;1's or .;1's and
 turn the other ;'s into version numbers.

 @param psz_oldname the ISO-9660 filename to be translated.
 @param psz_newname returned string. The caller allocates this and
 it should be at least the size of psz_oldname.
 @return length of the translated string is returned.");
%newobject name_translate;
char * name_translate(const char *psz_oldname);
%inline %{
char *
name_translate(const char *psz_oldname) {
  char *psz_newname=calloc(sizeof(char), strlen(psz_oldname)+1);
  iso9660_name_translate(psz_oldname, psz_newname);
  return psz_newname;
}
%}

%feature("autodoc",
"Convert an ISO-9660 file name that stored in a directory entry into
 what's usually listed as the file name in a listing.  Lowercase
 name if no Joliet Extension interpretation. Remove trailing ;1's or
 .;1's and turn the other ;'s into version numbers.

 @param psz_oldname the ISO-9660 filename to be translated.
 @param psz_newname returned string. The caller allocates this and
 it should be at least the size of psz_oldname.
 @param i_joliet_level 0 if not using Joliet Extension. Otherwise the
 Joliet level.
 @return length of the translated string is returned. It will be no greater
 than the length of psz_oldname.");
%newobject name_translate_ext;
char * name_translate_ext(const char *psz_oldname, uint8_t i_joliet_level);
%inline %{
char * 
name_translate_ext(const char *psz_oldname, uint8_t i_joliet_level) {
  char *psz_newname=calloc(sizeof(char), strlen(psz_oldname)+1);
  iso9660_name_translate_ext(psz_oldname, psz_newname, i_joliet_level);
  return psz_newname;
}
%}

%feature("autodoc",
"Pad string src with spaces to size len and copy this to dst. If
 en is less than the length of src, dst will be truncated to the
 first len characters of src.

 src can also be scanned to see if it contains only ACHARs, DCHARs, 
 7-bit ASCII chars depending on the enumeration _check.

 In addition to getting changed, dst is the return value.
 Note: this string might not be NULL terminated.");
%newobject strncpy_pad; // free malloc'd return value
char *strncpy_pad(const char src[], size_t len, enum strncpy_pad_check _check);
%inline %{
char *
strncpy_pad(const char src[], size_t len, enum strncpy_pad_check _check) {
  char *dst = calloc(sizeof(char), len+1);
  return iso9660_strncpy_pad(dst, src, len, _check);
}
%}
 
/*=====================================================================
  Files and Directory Names 
======================================================================*/

%feature("autodoc",
"Check that psz_path is a valid ISO-9660 directory name.

 A valid directory name should not start out with a slash (/), 
 dot (.) or null byte, should be less than 37 characters long, 
 have no more than 8 characters in a directory component 
 which is separated by a /, and consist of only DCHARs. 

 True is returned if psz_path is valid.");
%rename iso9660_dirname_valid_p dirname_valid_p;
bool iso9660_dirname_valid_p (const char psz_path[]);

%feature("autodoc",
"Take psz_path and a version number and turn that into a ISO-9660
pathname.  (That's just the pathname followed by ';' and the version
number. For example, mydir/file.ext -> MYDIR/FILE.EXT;1 for version
1. The resulting ISO-9660 pathname is returned.");
%rename iso9660_pathname_isofy pathname_isofy;
%newobject iso9660_pathname_isofy; // free malloc'd return value
char *iso9660_pathname_isofy (const char psz_path[], uint16_t i_version=1);

%feature("autodoc",
"Check that psz_path is a valid ISO-9660 pathname.  

A valid pathname contains a valid directory name, if one appears and
the filename portion should be no more than 8 characters for the
file prefix and 3 characters in the extension (or portion after a
dot). There should be exactly one dot somewhere in the filename
portion and the filename should be composed of only DCHARs.
  
True is returned if psz_path is valid.");
%rename iso9660_pathname_valid_p pathname_valid_p;
bool iso9660_pathname_valid_p (const char psz_path[]);

/* ... */

%feature("autodoc",
"Given a directory pointer, find the filesystem entry that contains
lsn and return information about it.

Returns stat_t of entry if we found lsn, or None otherwise.");
#if 0
%rename iso9660_find_fs_lsn fs_find_lsn;
IsoStat_t *iso9660_find_fs_lsn(CdIo_t *p_cdio, lsn_t i_lsn);


%feature("autodoc",
"Given a directory pointer, find the filesystem entry that contains
lsn and return information about it.

Returns stat_t of entry if we found lsn, or None otherwise.");
%rename iso9660_find_ifs_lsn ifs_find_lsn;
IsoStat_t *iso9660_find_ifs_lsn(const iso9660_t *p_iso, lsn_t i_lsn);
#endif


%feature("autodoc",
"Return file status for psz_path. None is returned on error.");
%rename iso9660_fs_stat fs_stat;
IsoStat_t *iso9660_fs_stat (CdIo_t *p_cdio, const char psz_path[]);
  

%feature("autodoc",
"Return file status for path name psz_path. None is returned on error.
pathname version numbers in the ISO 9660 name are dropped, i.e. ;1
is removed and if level 1 ISO-9660 names are lowercased.

The b_mode2 parameter is not used.");
%rename iso9660_fs_stat_translate fs_stat_translate;
IsoStat_t *iso9660_fs_stat_translate (CdIo_t *p_cdio, 
				      const char psz_path[], 
				      bool b_mode2=false);

%feature("autodoc",
"Return file status for pathname. None is returned on error.");
%rename iso9660_ifs_stat ifs_stat;
IsoStat_t *iso9660_ifs_stat (iso9660_t *p_iso, const char psz_path[]);


%feature("autodoc",
"Return file status for path name psz_path. undef is returned on error.
pathname version numbers in the ISO 9660 name are dropped, i.e. ;1 is
removed and if level 1 ISO-9660 names are lowercased.");
%rename iso9660_ifs_stat_translate ifs_stat_translate;
IsoStat_t *iso9660_ifs_stat_translate (iso9660_t *p_iso, 
				       const char psz_path[]);

%feature("autodoc",
"Read psz_path (a directory) and return a list of iso9660_stat_t
pointers for the files inside that directory.");
IsoStatList_t *fs_readdir (CdIo_t *p_cdio, const char psz_path[]);

%inline %{
IsoStatList_t *fs_readdir (CdIo_t *p_cdio, const char psz_path[])
{
  CdioList_t *p_statlist = iso9660_fs_readdir (p_cdio, psz_path, false);
  return p_statlist;
}
%}

%feature("autodoc",
"Read psz_path (a directory) and return a list of iso9660_stat_t
pointers for the files inside that directory.");
IsoStatList_t *ifs_readdir (iso9660_t *p_iso, const char psz_path[]);

%inline %{
IsoStatList_t *ifs_readdir (iso9660_t *p_iso, const char psz_path[])
{
  CdioList_t *p_statlist = iso9660_ifs_readdir (p_iso, psz_path);
  return p_statlist;
}
%}

%feature("autodoc",
"Return the PVD's application ID.
None is returned if there is some problem in getting this.");
%rename iso9660_get_application_id get_application_id;
char * iso9660_get_application_id(iso9660_pvd_t *p_pvd);
  
%feature("autodoc",
"Get the application ID.  Return None if there
is some problem in getting this.");
%newobject get_application_id; // free malloc'd return value
char *ifs_get_application_id(iso9660_t *p_iso);
%inline %{
char *
ifs_get_application_id(iso9660_t *p_iso) {
  char *psz;
  bool ok = iso9660_ifs_get_application_id(p_iso, &psz);
  if (!ok) return NULL;
  return psz;
}
%}


%feature("autodoc",
"Return the Joliet level recognized for p_iso.");
%rename iso9660_ifs_get_joliet_level get_joliet_level;
uint8_t iso9660_ifs_get_joliet_level(iso9660_t *p_iso);

%rename iso9660_get_dir_len get_dir_len;
uint8_t iso9660_get_dir_len(const iso9660_dir_t *p_idr);

%feature("autodoc",
"Return the directory name stored in the iso9660_dir_t.");
%newobject iso9660_dir_to_name; // free malloc'd return value
%rename iso9660_get_to_name get_to_name;
char * iso9660_dir_to_name (const iso9660_dir_t *p_iso9660_dir);
  
#if LIBCDIO_VERSION_NUM > 76
%feature("autodoc",
"Returns a POSIX mode for a given p_iso_dirent.");
%rename iso9660_get_posix_filemode get_posix_filemode;
mode_t iso9660_get_posix_filemode(const iso9660_stat_t *p_iso_dirent);
#endif

%feature("autodoc",
"Return a string containing the preparer id with trailing
blanks removed.");
%rename iso9660_get_preparer_id get_preparer_id;
char *iso9660_get_preparer_id(const iso9660_pvd_t *p_pvd);

%feature("autodoc",
"Get the preparer ID.  Return None if there is some problem in
 getting this.");
%newobject ifs_get_preparer_id; // free malloc'd return value
char *ifs_get_preparer_id(iso9660_t *p_iso);
%inline %{
char *
ifs_get_preparer_id(iso9660_t *p_iso) {
  char *psz;
  bool ok = iso9660_ifs_get_preparer_id(p_iso, &psz);
  if (!ok) return NULL;
  return psz;
}
%}

%feature("autodoc",
"Return a string containing the PVD's publisher id with trailing
 blanks removed.");
%rename iso9660_get_publisher_id get_publisher_id;
char *iso9660_get_publisher_id(const iso9660_pvd_t *p_pvd);

%feature("autodoc",
"Get the publisher ID. Return None if there
is some problem in getting this.");
%newobject ifs_get_publisher_id; // free malloc'd return value
char *ifs_get_publisher_id(iso9660_t *p_iso);
%inline %{
char *
ifs_get_publisher_id(iso9660_t *p_iso) {
  char *psz;
  bool ok = iso9660_ifs_get_publisher_id(p_iso, &psz);
  if (!ok) return NULL;
  return psz;
}
%}

%rename iso9660_get_pvd_type get_pvd_type;
uint8_t iso9660_get_pvd_type(const iso9660_pvd_t *p_pvd);
  
%rename iso9660_get_pvd_id get_pvd_id;
const char * iso9660_get_pvd_id(const iso9660_pvd_t *p_pvd);

%rename iso9660_get_pvd_space_size get_pvd_space_size;
int iso9660_get_pvd_space_size(const iso9660_pvd_t *p_pvd);

%rename iso9660_get_pvd_block_size get_pvd_block_size;
int iso9660_get_pvd_block_size(const iso9660_pvd_t *p_pvd) ;

%feature("autodoc",
"Return the primary volume id version number (of pvd).
If there is an error 0 is returned.");
%rename iso9660_get_pvd_version get_pvd_version;
int iso9660_get_pvd_version(const iso9660_pvd_t *pvd) ;

%feature("autodoc",
"Return a string containing the PVD's system id with trailing
 blanks removed.");
%rename iso9660_get_system_id get_system_id;
char *iso9660_get_system_id(const iso9660_pvd_t *p_pvd);

%feature("autodoc",
"Get the system ID.  None is returned if there
is some problem in getting this.");
%newobject ifs_get_system_id; // free malloc'd return value
char *ifs_get_system_id(iso9660_t *p_iso);
%inline %{
char *
ifs_get_system_id(iso9660_t *p_iso) {
  char *psz;
  bool ok = iso9660_ifs_get_system_id(p_iso, &psz);
  if (!ok) return NULL;
  return psz;
}
%}

%feature("autodoc",
"Return the LSN of the root directory for pvd.  If there is an error
INVALID_LSN is returned.
");
%rename iso9660_get_root_lsn get_root_lsn;
lsn_t iso9660_get_root_lsn(const iso9660_pvd_t *p_pvd);

%feature("autodoc",
"Return the PVD's volume ID.");
%rename iso9660_get_volume_id get_volume_id;
char *iso9660_get_volume_id(const iso9660_pvd_t *p_pvd);

%feature("autodoc",
"Get the system ID. None is returned if there
is some problem in getting this.");
%newobject ifs_get_volume_id; // free malloc'd return value
char *ifs_get_volume_id(iso9660_t *p_iso);
%inline %{
char *
ifs_get_volume_id(iso9660_t *p_iso) {
  char *psz;
  bool ok = iso9660_ifs_get_volume_id(p_iso, &psz);
  if (!ok) return NULL;
  return psz;
}
%}

%feature("autodoc",
"  Return the PVD's volumeset ID.
  None is returned if there is some problem in getting this. 
");
%rename iso9660_get_volumeset_id get_volumeset_id;
char *iso9660_get_volumeset_id(const iso9660_pvd_t *p_pvd);

%feature("autodoc",
"Get the volumeset ID.  None is returned if there
is some problem in getting this.");
%newobject ifs_get_volumeset_id; // free malloc'd return value
char *ifs_get_volumeset_id(iso9660_t *p_iso);
%inline %{
char *
ifs_get_volumeset_id(iso9660_t *p_iso) {
  char *psz;
  bool ok = iso9660_ifs_get_volumeset_id(p_iso, &psz);
  if (!ok) return NULL;
  return psz;
}
%}

/* ================= pathtable  ================== */
  
%feature("autodoc",
"Zero's out pathable. Do this first.");
%rename iso9660_pathtable_init pathtable_init;
void iso9660_pathtable_init (void *pt);
  
%rename iso9660_pathtable_get_size pathtable_get_size;
unsigned int iso9660_pathtable_get_size (const void *pt);
  
%rename iso9660_pathtable_l_add_entry pathtable_l_add_entry;
uint16_t iso9660_pathtable_l_add_entry (void *pt, const char name[], 
					uint32_t extent, uint16_t parent);
  
%rename iso9660_pathtable_m_add_entry pathtable_m_add_entry;
uint16_t iso9660_pathtable_m_add_entry (void *pt, const char name[], 
					uint32_t extent, uint16_t parent);
  
/*======================================================================
   Volume Descriptors
========================================================================*/

#ifdef FINSHED
void iso9660_set_pvd (void *pd, const char volume_id[], 
		      const char application_id[], 
		      const char publisher_id[], const char preparer_id[],
		      uint32_t iso_size, const void *root_dir, 
		      uint32_t path_table_l_extent, 
		      uint32_t path_table_m_extent,
		      uint32_t path_table_size, const time_t *pvd_time);
#endif /*FINISHED*/

%rename iso9660_set_evd set_evd;
void iso9660_set_evd (void *pd);

%feature("autodoc",
"Return true if ISO 9660 image has extended attrributes (XA).");
%rename iso9660_ifs_is_xa is_xa;
bool iso9660_ifs_is_xa (const iso9660_t * p_iso);

// %pythoncode %{
//%}
