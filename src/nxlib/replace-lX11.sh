#!/bin/sh
# Script to replace "-lX11" with "-lNX11 -lnano-X" in Makefiles and remove "-lXext" from them.
# if you add -e as second parameter, -lXext will be replaced by -lNXext
#
# Enter Makefile (or different name) as parameter on command line. 
# if the script fails, the original file is untouched


# replace -lX11 with -lNX11
sed -e 's/-lX11/-lNX11 -lnano-X/g' <$1 >tmp.out

if [ "$2" = "-e" ] ; then
 # replace with -lNXext
 sed -e 's/-lXext/-lNXext/g' <tmp.out > tmp2.out && mv tmp2.out $1
else
 # remove -lXext
 sed -e 's/-lXext//g' <tmp.out > tmp2.out && mv tmp2.out $1
fi

rm tmp.out
