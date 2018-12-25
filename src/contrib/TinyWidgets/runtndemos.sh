#!/bin/sh
clear
case $# in
0) find tndesigner/demos -type f -executable ;
   echo -n "Please enter demo program name: "
   read progname
   ../../bin/nano-X &tndesigner/demos/$progname/$progname ;;
1) ../../bin/nano-X &tndesigner/demos/$1/$1 ;;
esac
