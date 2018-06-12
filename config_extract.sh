#!/bin/sh
sed -r -e 's/^#define ([A-Z_]+/#define CDIO_$1/' config.h
