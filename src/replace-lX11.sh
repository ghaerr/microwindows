#!/bin/sh
# Script to replace "-lX11" with "-lPX11" in Makefiles and remove "-lXext" from them.
# Enter Makefile (or different name) as parameter on command line. The new file is 
# called replace-lX11.out.
#
sed -e 's/-lX11/-lPX11/g' -e 's/-lXext //g' <$1 >replace-lX11.out
