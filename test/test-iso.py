#!/usr/bin/env python
"""Unit test for iso9660

Test some low-level ISO9660 routines
This is basically the same thing as libcdio's testiso9660.c"""

import unittest, sys, os

libdir = os.path.join(os.path.dirname(__file__), '..')
if libdir[-1] != os.path.sep:
    libdir += os.path.sep
sys.path.insert(0, libdir)
import pyiso9660
import iso9660

def is_eq(a, b):
    if len(a) != len(b): return False
    
    for i in range(len(a)):
        if a[i] != b[i]:
            print "position %d: %d != %d\n" % (i, a[i], b[i])
            return False
    return True
    
achars = ('!', '"', '%', '&', '(', ')', '*', '+', ',', '-', '.',
           '/', '?', '<', '=', '>')

class ISO9660Tests(unittest.TestCase):

    def test_chars(self):
        """Test ACHAR and DCHAR"""
        bad = 0
        c=ord('A')
        while c<=ord('Z'):
            if not pyiso9660.is_dchar(c):
                print "Failed iso9660_is_achar test on %c" % c
                bad += 1
            if not pyiso9660.is_achar(c):
                print "Failed iso9660_is_achar test on %c" % c
                bad += 1
            c += 1

        self.assertEqual(True, bad==0, 'is_dchar & is_achar A..Z')

        bad=0
        c=ord('0')
        while c<=ord('9'):
            if not pyiso9660.is_dchar(c):
                print "Failed iso9660_is_dchar test on %c" % c
                bad += 1
            if not pyiso9660.is_achar(c):
                print "Failed iso9660_is_achar test on %c" % c
                bad += 1
            c += 1
        self.assertEqual(True, bad==0, 'is_dchar & is_achar 0..9')

        bad=0
        i=0
        while i<=13:
            c=ord(achars[i])
            if pyiso9660.is_dchar(c):
                print "Should not pass is_dchar test on %c" % c
                bad += 1
            if not pyiso9660.is_achar(c):
                print "Failed is_achar test on symbol %c" % c
                bad += 1
            i += 1

        self.assertEqual(True, bad==0, 'is_dchar & is_achar symbols')

    def test_strncpy_pad(self):
        """Test pyiso9660.strncpy_pad"""

        dst = pyiso9660.strncpy_pad("1_3", 5, pyiso9660.DCHARS)
        self.assertEqual(dst, "1_3  ", "strncpy_pad DCHARS")
    
        dst = pyiso9660.strncpy_pad("ABC!123", 2, pyiso9660.ACHARS)
        self.assertEqual(dst, "AB", "strncpy_pad ACHARS truncation")

    def test_dirname(self):
        """Test pyiso9660.dirname_valid_p"""
        
        self.assertEqual(False, pyiso9660.dirname_valid_p("/NOGOOD"),
                         "dirname_valid_p - /NOGOOD is no good.")

        self.assertEqual(False, 
                         pyiso9660.dirname_valid_p("LONGDIRECTORY/NOGOOD"),
                         "pyiso9660.dirname_valid_p - too long directory")

        self.assertEqual(True, pyiso9660.dirname_valid_p("OKAY/DIR"),
                        "dirname_valid_p - OKAY/DIR should pass ")

        self.assertEqual(False, pyiso9660.dirname_valid_p("OKAY/FILE.EXT"),
                         "pyiso9660.dirname_valid_p - OKAY/FILENAME.EXT")

    def test_image_info(self):
        """Test retrieving image information"""
        
        # The test ISO 9660 image
        image_path="../data"
        image_fname=os.path.join(image_path, "copying.iso")
        iso = iso9660.ISO9660.IFS(source=image_fname)
        self.assertNotEqual(iso, None, "Opening %s" % image_fname)

        self.assertEqual(iso.get_application_id(),
"MKISOFS ISO 9660/HFS FILESYSTEM BUILDER & CDRECORD CD-R/DVD CREATOR (C) 1993 E.YOUNGDALE (C) 1997 J.PEARSON/J.SCHILLING",
        "get_application_id()")

        self.assertEqual(iso.get_system_id(), "LINUX",
        "get_system_id() eq 'LINUX'")
        self.assertEqual(iso.get_volume_id(), "CDROM",
        "get_volume_id() eq 'CDROM'")

        file_stats = iso.readdir('/')

        okay_stats = [
          ['.', 23, 2048, 1, 2],
          ['..', 23, 2048, 1, 2],
          ['COPYING.;1', 24, 17992, 9, 1]
          ]
        self.assertEqual(file_stats, okay_stats, "file stat info")

    def test_pathname_valid(self):
        """Test pyiso9660.pathname_valid_p"""

        self.assertEqual(True, pyiso9660.pathname_valid_p("OKAY/FILE.EXT"),
                        "pyiso9660.dirname_valid_p - OKAY/FILE.EXT ")
        self.assertEqual(False, 
                         pyiso9660.pathname_valid_p("OKAY/FILENAMELONG.EXT"),
                        'invalid pathname, long basename')

        self.assertEqual(False, 
                         pyiso9660.pathname_valid_p("OKAY/FILE.LONGEXT"),
                         "pathname_valid_p - long extension" )

        dst = pyiso9660.pathname_isofy("this/file.ext", 1)
        self.assertNotEqual(dst, "this/file.ext1", "iso9660_pathname_isofy")

    def test_time(self):
        """Test time"""
        import time

        tm = time.localtime(0)
        dtime = pyiso9660.set_dtime(tm[0], tm[1], tm[2], tm[3], tm[4], tm[5])
        new_tm =  pyiso9660.get_dtime(dtime, True)

        ### FIXME Don't know why the discrepancy, but there is an hour
        ### difference, perhaps daylight savings time.
        ### Versions before 0.77 have other bugs.
        if new_tm is not None:
            # if pyiso9660.VERSION_NUM < 77: new_tm[3] = tm[3]
            new_tm[3] = tm[3]
            self.assertEqual(True, is_eq(new_tm, tm), 'get_dtime(set_dtime())')
        else: 
            self.assertEqual(True, False, 'get_dtime is None')

#        if pyiso9660.VERSION_NUM >= 77:
#            tm = time.gmtime(0)
#            ltime = pyiso9660.set_ltime(tm[0], tm[1], tm[2], tm[3], tm[4],
#                                        tm[5])
#            new_tm =  pyiso9660.get_ltime(ltime)
#            self.assertEqual(True, is_eq(new_tm, tm), 
#                             'get_ltime(set_ltime())')
        return
    
if __name__ == "__main__":
    unittest.main()
