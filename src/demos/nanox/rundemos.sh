#!/bin/sh
clear
case $# in
0) ls -C 
   echo -n "Please enter program name: "
   read progname
   ./nano-X & ./nanowm &./$progname ;;
1) ./nano-X & ./nanowm &./$1 ;;
esac
