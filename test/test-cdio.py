#!/usr/bin/env python
"""Unit test for cdio

Note: for compatibility with old unittest 1.46 we won't use assertTrue
or assertFalse."""
import unittest, sys, os

libdir = os.path.join(os.path.dirname(__file__), 
                      '..', 'build', 'lib.linux-i686-2.5')
if libdir[-1] != os.path.sep:
    libdir += os.path.sep
sys.path.insert(0, libdir)
libdir = os.path.join(os.path.dirname(__file__), '..')
if libdir[-1] != os.path.sep:
    libdir += os.path.sep
sys.path.insert(0, libdir)
import pycdio
import cdio

class CdioTests(unittest.TestCase):

    def test_ops(self):
        """Test running miscellaneous operations
        No assumption about the CD-ROM drives is made, so
        we're just going to run operations and see that they
        don't crash."""
        self.device = cdio.Device()
        if pycdio.VERSION_NUM >= 76:
            # FIXME: Broken on Darwin? 
            # self.device.open()
            self.device.have_ATAPI()
            # FIXME: Broken on Darwin? 
            # self.device.get_media_changed()
        self.assertEqual(True, True, "Test misc operations")

    def test_device_default(self):
        """Test getting default device"""
        result1=cdio.get_default_device_driver(pycdio.DRIVER_DEVICE)
        result2=cdio.get_default_device_driver()
        self.assertEqual(result1, result2,
                         "get_default_device with/out parameters")
        self.device = cdio.Device()
        result2=pycdio.get_device()
        if result1 is not None:
            self.assertEqual(result1[0], result2)
            # Now try getting device using driver that we got back
            try: 
              device=cdio.Device(driver_id=result1[1])
              result1 = device.get_device()
              self.assertEqual(result1, result2,
                             "get_default_device using driver name")
            except:
              pass

    def test_exceptions(self):
        """Test that various routines raise proper exceptions"""
        self.device = cdio.Device()
        # No CD or or CD image has been set yet. So these fail
        try: 
          lsn = self.device.get_disc_last_lsn()
        except IOError:
          self.assertEqual(True, True, "get_last_lsn() IO Error")
        except cdio.DriverError:
          self.assertEqual(True, True, "get_last_lsn() DriverError")
        else:
           self.assertTrue(False, "get_last_lsn() should raise error")
        self.assertRaises(IOError, self.device.get_disc_mode)
        try:
          track = self.device.get_num_tracks()
        except IOError:
          self.assertEqual(True, True, "get_num_tracks() IO Error")
        except cdio.DriverError:
          self.assertEqual(True, True, "get_num_tracks() DriverError")
        except cdio.TrackError:
          self.assertEqual(True, True, "get_num_tracks() TrackError")
        else:
           self.assertTrue(False, "get_last_lsn() should raise error")
        self.assertRaises(IOError, self.device.get_driver_name)
        self.assertRaises(cdio.DriverUninitError,
                          self.device.get_media_changed)
        self.assertRaises(IOError, self.device.open, "***Invalid device***")

    def test_have_image_drivers(self):
        """Test that we have image drivers"""
        result = cdio.have_driver('CDRDAO')
        self.assertEqual(True, result, "Have cdrdrao driver via string")
        result = cdio.have_driver(pycdio.DRIVER_CDRDAO)
        self.assertEqual(True, result, "Have cdrdrao driver via driver_id")
        result = cdio.have_driver('NRG')
        self.assertEqual(True, result, "Have NRG driver via string")
        result = cdio.have_driver(pycdio.DRIVER_NRG)
        self.assertEqual(True, result, "Have NRG driver via driver_id")
        result = cdio.have_driver('BIN/CUE')
        self.assertEqual(True, result, "Have BIN/CUE driver via string")
        result = cdio.have_driver(pycdio.DRIVER_BINCUE)
        self.assertEqual(True, result, "Have BIN/CUE driver via driver_id")

    def test_tocfile(self):
        """Test functioning of cdrdao image routines"""
        ## TOC reading needs to be done in the directory where the
        ## TOC/BIN files reside.
        olddir=os.getcwd()
        os.chdir('.')
        tocfile="cdda.toc"
        device = cdio.Device(tocfile, pycdio.DRIVER_CDRDAO)
        ok, vendor, model, revision  = device.get_hwinfo()
        self.assertEqual(True, ok, "get_hwinfo ok")
        self.assertEqual('libcdio', vendor, "get_hwinfo vendor")
        self.assertEqual('cdrdao', model, "get_hwinfo cdrdao")
        # Test known values of various access parameters:
        # access mode, driver name via string and via driver_id
        # and cue name
        result = device.get_arg("access-mode")
        self.assertEqual(result, 'image', 'get_arg("access_mode")',)
        result = device.get_driver_name()
        self.assertEqual(result, 'CDRDAO', 'get_driver_name')
        result = device.get_driver_id()
        self.assertEqual(result, pycdio.DRIVER_CDRDAO, 'get_driver_id')
        result = device.get_arg("source")
        self.assertEqual(result, tocfile, 'get_arg("source")')
        result = device.get_media_changed()
        self.assertEqual(False, result, "tocfile: get_media_changed")
        # Test getting is_tocfile
        result = cdio.is_tocfile(tocfile)
        self.assertEqual(True, result, "is_tocfile(tocfile)")
        result = cdio.is_nrg(tocfile)
        self.assertEqual(False, result, "is_nrgfile(tocfile)")
        result = cdio.is_device(tocfile)
        self.assertEqual(False, result, "is_device(tocfile)")
        self.assertRaises(cdio.DriverUnsupportedError,
                          device.set_blocksize, 2048)
        self.assertRaises(cdio.DriverUnsupportedError,
                          device.set_speed, 5)
        device.close()
        os.chdir(olddir)

    def test_read(self):
        """Test functioning of read routines"""
        cuefile="./../data/isofs-m1.cue"
        device = cdio.Device(source=cuefile)
        # Read the ISO Primary Volume descriptor
        blocks, data=device.read_sectors(16, pycdio.READ_MODE_M1F1)
        self.assertEqual(data[1:6], 'CD001')
        self.assertEqual(blocks, 1)
        blocks, data=device.read_data_blocks(26)
        self.assertEqual(data[6:32], 'GNU GENERAL PUBLIC LICENSE')

    def test_bincue(self):
        """Test functioning of BIN/CUE image routines"""
        cuefile="./cdda.cue"
        device = cdio.Device(source=cuefile)
        # Test known values of various access parameters:
        # access mode, driver name via string and via driver_id
        # and cue name
        result = device.get_arg("access-mode")
        self.assertEqual(result, 'image', 'get_arg("access_mode")',)
        result = device.get_driver_name()
        self.assertEqual(result, 'BIN/CUE', 'get_driver_name')
        result = device.get_driver_id()
        self.assertEqual(result, pycdio.DRIVER_BINCUE, 'get_driver_id')
        result = device.get_arg("cue")
        self.assertEqual(result, cuefile, 'get_arg("cue")')
        # Test getting is_binfile and is_cuefile
        binfile = cdio.is_cuefile(cuefile)
        self.assertEqual(True, binfile != None, "is_cuefile(cuefile)")
        cuefile2 = cdio.is_binfile(binfile)
        # Could check that cuefile2 == cuefile, but some OS's may 
        # change the case of files
        self.assertEqual(True, cuefile2 != None, "is_cuefile(binfile)")
        result = cdio.is_tocfile(cuefile)
        self.assertEqual(False, result, "is_tocfile(tocfile)")
        ok, vendor, model, revision  = device.get_hwinfo()
        self.assertEqual(True, ok, "get_hwinfo ok")
        self.assertEqual('libcdio', vendor, "get_hwinfo vendor")
        self.assertEqual('CDRWIN', model, "get_hwinfo model")
        result = cdio.is_device(cuefile)
        self.assertEqual(False, result, "is_device(tocfile)")
        result = device.get_media_changed()
        self.assertEqual(False, result, "binfile: get_media_changed")
        if pycdio.VERSION_NUM >= 77:
            # There's a bug in libcdio 0.76 that causes these to crash
            self.assertRaises(cdio.DriverUnsupportedError,
                              device.set_blocksize, 2048)
            self.assertRaises(cdio.DriverUnsupportedError,
                              device.set_speed, 5)
        device.close()

    def test_cdda(self):
        """Test functioning CD-DA"""
        device = cdio.Device()
        cuefile="./cdda.cue"
        device.open(cuefile)
        result = device.get_disc_mode()
        self.assertEqual(result, 'CD-DA', 'get_disc_mode')
        self.assertEqual(device.get_mcn(), '0000010271955', 'get_mcn')
        self.assertRaises(cdio.DriverUnsupportedError,
                          device.get_last_session)
        # self.assertRaises(IOError, device.get_joliet_level)
        result = device.get_num_tracks()
        self.assertEqual(result, 1, 'get_num_tracks')
        disc_last_lsn = device.get_disc_last_lsn()
        self.assertEqual(disc_last_lsn, 302, 'get_disc_last_lsn')
        t=device.get_last_track()
        self.assertEqual(t.track, 1, 'get_last_track')
        self.assertEqual(t.get_last_lsn(), 301, '(track) get_last_lsn')
        self.assertEqual(device.get_track_for_lsn(t.get_last_lsn()).track,
                         t.track)
        t=device.get_first_track()
        self.assertEqual(t.track, 1, 'get_first_track')
        self.assertEqual(t.get_format(), 'audio', 'get_track_format')
        device.close()
    
if __name__ == "__main__":
    unittest.main()
