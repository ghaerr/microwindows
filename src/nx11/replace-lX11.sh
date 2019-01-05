#!/bin/sh
# Script to replace "-lX11" with "-lNX11 -lnano-X" in Makefiles,
# and remove "-lXext" and  "-lXfixes" from them.
#
# Enter Makefile (or different name) as parameter on command line. 
# if the script fails, the original file is untouched

# replace -lX11 with -lNX11 -lnano-X
sed -e 's/-lX11/-lNX11 -lnano-X/g'	< $1 > tmp.out && mv tmp.out $1
# remove -lXext and -lXfixes
sed -e 's/-lXext//g'				< $1 > tmp.out && mv tmp.out $1
sed -e 's/-lXfixes//g'				< $1 > tmp.out && mv tmp.out $1

rm tmp.out
