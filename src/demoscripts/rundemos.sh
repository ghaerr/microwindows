#!/bin/sh
# show bin directory and run one nano-X program
clear
case $# in
0) ls -C bin
   echo -n "Please enter program name: "
   read progname
   bin/nano-X & bin/$progname ;;
1) bin/nano-X & bin/$1 ;;
esac
