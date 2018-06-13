#!/bin/bash
gcc -I../include/ -g3 -Wall -Wchar-subscripts -Wmissing-prototypes -Wmissing-declarations -Wunused -Wpointer-arith -Wwrite-strings -Wnested-externs -Wno-sign-compare -o read-disc-struct.o -c read-disc-struct.c

/bin/sh ../libtool --silent --tag=CC   --mode=link gcc  -g3 -o read-disc-struct  read-disc-struct.o ../lib/driver/libcdio.la  -lm
