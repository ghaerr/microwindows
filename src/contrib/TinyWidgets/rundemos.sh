#!/bin/sh
clear
case $# in
0) find demos -type f -executable -exec basename {} \;
   echo -n "Please enter demo program name: "
   read progname
   ../../bin/nano-X & sleep 1 & demos/$progname ;;
1) ../../bin/nano-X & sleep 1 & ../../bin/nanowm & demos/$1 ;;
esac
