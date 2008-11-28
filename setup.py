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

# Find link args for libcdio and libiso9660 using pkg-config, create
# the right Extension for this to fit into ext_modules later.
modules = []
for lib_name in ('libcdio', 'libiso9660'):
    short_libname = lib_name[3:] # Strip off "lib" from name
    p = Popen(['pkg-config', '--libs', lib_name], stdout=PIPE)
    if p.returncode is None:
        link_args = [p.communicate()[0].strip()]
        lib      =  None # Above includes libcdio
    else:
        print ("Didn't the normal return code running pkg-config," + 
               "on %s. got:\n\t%s" % [lib_name, p.returncode])
        print "Will try to add %s anyway." % short_libname
        link_args = None
        lib       = short_libname # Strip off "lib" frame name
    py_shortname='py' + short_libname
    modules.append(Extension('_' + py_shortname,
                             libraries = lib,
                             extra_link_args = link_args,
                             sources=['swig/%s.i' % py_shortname]))

setup (name = 'pycdio',
       author             = 'Rocky Bernstein',
       author_email       = 'rocky@gnu.org',
       description        = 'Python OO interface to libcdio (CD Input and Control library)',
       ext_modules        = modules,
       license            = 'GPL',
       long_description   = long_description,
       name               = 'pycdio', 
       py_modules         = ['pycdio'],
       test_suite         = 'nose.collector',
       url                = 'http://freshmeat.net/projects/libcdio/?branch_id=62870',
       version            = version,
       )
