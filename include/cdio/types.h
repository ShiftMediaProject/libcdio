/*
    $Id: types.h,v 1.17 2004/05/26 06:29:15 rocky Exp $

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2002, 2003, 2004 Rocky Bernstein <rocky@panix.com>

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

/** \file types.h 
 *  \brief  Common type definitions used pervasively in libcdio.
 */


#ifndef __CDIO_TYPES_H__
#define __CDIO_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  /* provide some C99 definitions */

#if defined(HAVE_SYS_TYPES_H) 
#include <sys/types.h>
#endif 

#if defined(HAVE_STDINT_H)
# include <stdint.h>
#elif defined(HAVE_INTTYPES_H)
# include <inttypes.h>
#elif defined(AMIGA) || defined(__linux__)
  typedef u_int8_t uint8_t;
  typedef u_int16_t uint16_t;
  typedef u_int32_t uint32_t;
  typedef u_int64_t uint64_t;
#else
  /* warning ISO/IEC 9899:1999 <stdint.h> was missing and even <inttypes.h> */
  /* fixme */
#endif /* HAVE_STDINT_H */
  
  /* default HP/UX macros are broken */
#if defined(__hpux__)
# undef UINT16_C
# undef UINT32_C
# undef UINT64_C
# undef INT64_C
#endif

  /* if it's still not defined, take a good guess... should work for
     most 32bit and 64bit archs */
  
#ifndef UINT16_C
# define UINT16_C(c) c ## U
#endif
  
#ifndef UINT32_C
# if defined (SIZEOF_INT) && SIZEOF_INT == 4
#  define UINT32_C(c) c ## U
# elif defined (SIZEOF_LONG) && SIZEOF_LONG == 4
#  define UINT32_C(c) c ## UL
# else
#  define UINT32_C(c) c ## U
# endif
#endif
  
#ifndef UINT64_C
# if defined (SIZEOF_LONG) && SIZEOF_LONG == 8
#  define UINT64_C(c) c ## UL
# elif defined (SIZEOF_INT) && SIZEOF_INT == 8
#  define UINT64_C(c) c ## U
# else
#  define UINT64_C(c) c ## ULL
# endif
#endif
  
#ifndef INT64_C
# if defined (SIZEOF_LONG) && SIZEOF_LONG == 8
#  define INT64_C(c) c ## L
# elif defined (SIZEOF_INT) && SIZEOF_INT == 8
#  define INT64_C(c) c 
# else
#  define INT64_C(c) c ## LL
# endif
#endif
  
#if defined(HAVE_STDBOOL_H)
#include <stdbool.h>
#else
  /* ISO/IEC 9899:1999 <stdbool.h> missing -- enabling workaround */
  
# ifndef __cplusplus
  typedef enum
    {
      false = 0,
      true = 1
    } _Bool;
  
#  define false   false
#  define true    true
#  define bool _Bool
# endif
#endif
  
  /* some GCC optimizations -- gcc 2.5+ */
  
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define GNUC_PRINTF( format_idx, arg_idx )              \
  __attribute__((format (printf, format_idx, arg_idx)))
#define GNUC_SCANF( format_idx, arg_idx )               \
  __attribute__((format (scanf, format_idx, arg_idx)))
#define GNUC_FORMAT( arg_idx )                  \
  __attribute__((format_arg (arg_idx)))
#define GNUC_NORETURN                           \
  __attribute__((noreturn))
#define GNUC_CONST                              \
  __attribute__((const))
#define GNUC_UNUSED                             \
  __attribute__((unused))
#define GNUC_PACKED                             \
  __attribute__((packed))
#else   /* !__GNUC__ */
#define GNUC_PRINTF( format_idx, arg_idx )
#define GNUC_SCANF( format_idx, arg_idx )
#define GNUC_FORMAT( arg_idx )
#define GNUC_NORETURN
#define GNUC_CONST
#define GNUC_UNUSED
#define GNUC_PACKED
#endif  /* !__GNUC__ */
  
#if defined(__GNUC__)
  /* for GCC we try to use GNUC_PACKED */
# define PRAGMA_BEGIN_PACKED
# define PRAGMA_END_PACKED
#elif defined(HAVE_ISOC99_PRAGMA)
  /* should work with most EDG-frontend based compilers */
# define PRAGMA_BEGIN_PACKED _Pragma("pack(1)")
# define PRAGMA_END_PACKED   _Pragma("pack()")
#else /* neither gcc nor _Pragma() available... */
  /* ...so let's be naive and hope the regression testsuite is run... */
# define PRAGMA_BEGIN_PACKED
# define PRAGMA_END_PACKED
#endif
  
  /*
   * user directed static branch prediction gcc 2.96+
   */
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 95)
# define GNUC_LIKELY(x)   __builtin_expect((x),true)
# define GNUC_UNLIKELY(x) __builtin_expect((x),false)
#else 
# define GNUC_LIKELY(x)   (x) 
# define GNUC_UNLIKELY(x) (x)
#endif
  
#ifndef NULL
# define NULL ((void*) 0)
#endif
  
  /* our own offsetof()-like macro */
#define __cd_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
  
  /*!
    \brief MSF (minute/second/frame) structure 

    One CD-ROMs addressing scheme especially used in audio formats
    (Red Book) is an address by minute, sector and frame which
    BCD-encoded in three bytes. An alternative format is an lba_t.
    
    @see lba_t
  */
  PRAGMA_BEGIN_PACKED
  struct msf_rec {
    uint8_t m, s, f;
  } GNUC_PACKED;
  PRAGMA_END_PACKED
  
  typedef struct msf_rec msf_t;

#define msf_t_SIZEOF 3
  
  /* type used for bit-fields in structs (1 <= bits <= 8) */
#if defined(__GNUC__)
  /* this is strict ISO C99 which allows only 'unsigned int', 'signed
     int' and '_Bool' explicitly as bit-field type */
  typedef unsigned int bitfield_t;
#else
  /* other compilers might increase alignment requirements to match the
     'unsigned int' type -- fixme: find out how unalignment accesses can
     be pragma'ed on non-gcc compilers */
  typedef uint8_t bitfield_t;
#endif
  
  /*! The type of a Logical Block Address. We allow for an lba to be 
    negative to be consistent with an lba, although I'm not sure this
    this is possible.
      
   */
  typedef int32_t lba_t;
  
  /*! The type of a Logical Sector Number. Note that an lba lsn be negative
    and the MMC3 specs allow for a conversion of a negative lba

    @see msf_t
  */
  typedef int32_t lsn_t;
  
  /*! The type of an track number 0..99. */
  typedef uint8_t track_t;
  
  /*! 
    Constant for invalid track number
  */
#define CDIO_INVALID_TRACK   0xFF
  
  /*! 
    Constant for invalid LBA. It is 151 less than the most negative
    LBA -45150. This provide slack for the 150-frame offset in
    LBA to LSN 150 conversions
  */
#define CDIO_INVALID_LBA    -45301
  
  /*! 
    Constant for invalid LSN
  */
#define CDIO_INVALID_LSN    CDIO_INVALID_LBA

  /*! 
    Number of ASCII bytes in a media catalog number (MCN).
  */
#define CDIO_MCN_SIZE       13

typedef int cdio_fs_anal_t;

/*! The type of an drive capability bit mask. See below for values*/
  typedef uint32_t cdio_drive_cap_t;
  
/*!
  \brief Drive types returned by cdio_get_drive_cap()

  Most are copied from the GNU/Linux the uniform CD-ROM driver header
  linux/cdrom.h>  NOTE: Setting a bit here means the presence of
  a capability.
*/ 

#define CDIO_DRIVE_CAP_CLOSE_TRAY     0x00001 /**< caddy systems can't 
                                                   close... */
#define CDIO_DRIVE_CAP_OPEN_TRAY      0x00002 /**< but can eject.  */
#define CDIO_DRIVE_CAP_LOCK	      0x00004 /**< disable manual eject */
#define CDIO_DRIVE_CAP_SELECT_SPEED   0x00008 /**< programmable speed */
#define CDIO_DRIVE_CAP_SELECT_DISC    0x00010 /**< select disc from juke-box */
#define CDIO_DRIVE_CAP_MULTI_SESSION  0x00020 /**< read sessions>1 */
#define CDIO_DRIVE_CAP_MCN	      0x00040 /**< Medium Catalog Number */
#define CDIO_DRIVE_CAP_MEDIA_CHANGED  0x00080 /**< media changed */
#define CDIO_DRIVE_CAP_CD_AUDIO	      0x00100 /**< drive can play CD audio */
#define CDIO_DRIVE_CAP_RESET          0x00200 /**< hard reset device */
#define CDIO_DRIVE_CAP_IOCTLS         0x00400 /**< driver has non-standard 
                                                   ioctls */
#define CDIO_DRIVE_CAP_DRIVE_STATUS   0x00800 /**< driver implements drive 
                                               status */
#define CDIO_DRIVE_CAP_GENERIC_PACKET 0x01000 /**< driver implements generic 
                                                   packets */
#define CDIO_DRIVE_CAP_CD_R	      0x02000 /**< drive can write CD-R */
#define CDIO_DRIVE_CAP_CD_RW	      0x04000 /**< drive can read CD-RW */
#define CDIO_DRIVE_CAP_DVD	      0x08000 /**< drive can read DVD */
#define CDIO_DRIVE_CAP_DVD_R	      0x10000 /**< drive can write DVD-R */
#define CDIO_DRIVE_CAP_DVD_RAM	      0x20000 /**< drive can write DVD-RAM */

/**<  These are not taken by GNU/Linux cdrom.h (yet) */
#define CDIO_DRIVE_CAP_ERROR          0x00000 /**< Error */
#define CDIO_DRIVE_CAP_FILE	      0x40000 /**< drive is really a file, i.e
                                                   a CD file image */
#define CDIO_DRIVE_CAP_UNKNOWN        0x80000 /**< Dunno. It can be on if we
					        have only partial information 
                                                or are not completely certain
                                          */
/**< Masks derived from above... */
#define CDIO_DRIVE_CAP_CD_WRITER \
   (CDIO_DRIVE_CAP_CD_R|CDIO_DRIVE_CAP_CD_RW) 
/**< Has some sort of CD writer ability */

#define CDIO_DRIVE_CAP_CD \
   (CDIO_DRIVE_CAP_CD_AUDIO|CDIO_DRIVE_CAP_CD_WRITER)
/**< Has some sort of CD ability */

#define CDIO_DRIVE_CAP_DVD_WRITER \
   (CDIO_DRIVE_CAP_DVD_R|CDIO_DRIVE_CAP_DVD_RAM)
/**< Has some sort of DVD writer ability */

/*! 
  track flags
  Q Sub-channel Control Field (4.2.3.3)
*/
typedef enum {
  CDIO_TRACK_FLAG_NONE = 		0x00,	/**< no flags set */
  CDIO_TRACK_FLAG_PRE_EMPHASIS =	0x01,	/**< audio track recorded with
                                                     pre-emphasis */
  CDIO_TRACK_FLAG_COPY_PERMITTED =	0x02,	/**< digital copy permitted */
  CDIO_TRACK_FLAG_DATA =		0x04,	/**< data track */
  CDIO_TRACK_FLAG_FOUR_CHANNEL_AUDIO =	0x08,	/**< 4 audio channels */
  CDIO_TRACK_FLAG_SCMS =			0x10	/**< SCMS (5.29.2.7) */
} cdio_track_flag;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_TYPES_H__ */


/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
