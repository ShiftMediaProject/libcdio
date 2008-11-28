#!/usr/bin/env python

"""
distutils setup (setup.py) for pycdio
"""

from setuptools import setup
from distutils.core import Extension
from subprocess import *

version = '0.14vc'

import os
README = os.path.join(os.path.dirname(__file__), 'README.txt')
long_description = open(README).read() + '\n\n'

# Find link args for libcdio and libiso9660 using pkg-config and add
# collect them into a list for setup's ext_modules.
libraries=[]
link_args=[]
for i, lib_name in enumerate(('libcdio', 'libiso9660',)):
    p = Popen(['pkg-config', '--libs', lib_name], stdout=PIPE)
    if p.returncode is None:
        link_args.append([p.communicate()[0]])
        libraries.append(None) # Above includes libcdio
    else:
        print ("Didn't the normal return code running pkg-config," + 
               "got:\n\t%s" % p.returncode)
        short_libname = lib_name[3:] # Strip off "lib" from name
        print "Will try to add %s anyway." % short_libname
        link_args.append(None)
        libraries.append(short_libname) # Strip off "lib" frame name
else:
    libcdio_link_args = None
    libcdio_library = ['cdio']  

# FIXME: incorporate into above loop. E.g. using modules=[] var.
pycdio_module    = Extension('_pycdio', 
                             libraries=libraries[0],
                             extra_link_args=link_args[0],
                             sources=['swig/pycdio.i'])
pyiso9660_module = Extension('_pyiso9660', 
                             libraries=libraries[1],
                             extra_link_args=link_args[1],
                             sources=['swig/pyiso9660.i'])
setup (name = 'pycdio',
       author             = 'Rocky Bernstein',
       author_email       = 'rocky@gnu.org',
       description        = 'Python OO interface to libcdio (CD Input and Control library)',
       ext_modules        = [pycdio_module, pyiso9660_module],
       license            = 'GPL',
       long_description   = long_description,
       name               = 'pytracer', 
       py_modules         = ['pycdio'],
       test_suite         = 'nose.collector',
       url                = 'http://freshmeat.net/projects/libcdio/?branch_id=62870',
       version            = version,
       )
