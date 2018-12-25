#!/bin/sh
clear
case $# in
0) find demos -maxdepth 1 -type f -executable -exec basename {} \;
   echo -n "Please enter demo program name: "
   read progname
   ../../bin/nano-X & demos/$progname ;;
1) ../../bin/nano-X & demos/$1 ;;
esac
