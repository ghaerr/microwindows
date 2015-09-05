#!/bin/sh
clear
case $# in
0) ls -C bin
   echo -n "Please enter program name: "
   read progname
   bin/nano-X & bin/nanowm &bin/$progname ;;
1) bin/nano-X & bin/nanowm &bin/$1 ;;
esac
