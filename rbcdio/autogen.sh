#!/bin/sh
#$Id: autogen.sh,v 1.1 2006/11/13 05:12:41 rocky Exp $
aclocal -I .
automake --add-missing
autoconf
