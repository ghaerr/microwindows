#!/bin/sh
# show bin directory and run one nano-X or microwin program
clear
case $# in
0) ls -C bin
   echo -n "Please enter program name: "
   read progname
   case "$progname" in
     "mw"*) 
       bin/$progname ;;
     *) 
       bin/nano-X &bin/$progname ;;
   esac
   ;;
   
1) bin/nano-X &bin/$1 ;;
esac
