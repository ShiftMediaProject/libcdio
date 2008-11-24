#!/usr/bin/env python

"""
setup.py file for pycdio
"""

from setuptools import setup
from distutils.core import Extension

version = '0.14vc'

import os
README = os.path.join(os.path.dirname(__file__), 'README.txt')
long_description = open(README).read() + '\n\n'

pycdio_module = Extension('_pycdio',
                          sources=['pycdio_wrap.c', 'swig/pycdio.swg'],
                          )
pyiso9660_module = Extension('_pyiso9660',
                             sources=['iso9660_wrap.c', 'swig/pyiso9660.swg'],
                          )
setup (name = 'pycdio',
       author             = 'Rocky Bernstein',
       author_email       = 'rocky@gnu.org',
       description        = 'Python OO interface to libcdio (CD Input and Control library)',
       py_ext_modules     = [pycdio_module, pyiso9660_module],
       license            = 'GPL',
       long_description   = long_description,
       name               = 'pytracer', 
       py_modules         = ['pycdio'],
       test_suite         = 'nose.collector',
       url                = 'http://freshmeat.net/projects/libcdio/?branch_id=62870',
       version            = version,
       )
