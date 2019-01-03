#!/bin/sh
# Script to replace "-lX11" with "-lNX11 -lnano-X" in Makefiles and remove "-lXext" from them.
#
# Enter Makefile (or different name) as parameter on command line. 
# if the script fails, the original file is untouched

# replace -lX11 with -lNX11 -lnano-X
sed -e 's/-lX11/-lNX11 -lnano-X/g' < $1 > tmp.out
# remove -lXext
sed -e 's/-lXext//g' < tmp.out > tmp2.out && mv tmp2.out $1

rm tmp.out
