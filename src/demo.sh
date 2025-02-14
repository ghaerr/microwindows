# Script to show various demo startup options
#   Edit below to run demo(s) of your choice
# 

# standard startup - starts demos launcher and nano-X server
bin/nxstart

# chess w/bugs
#bin/nxchess & bin/nxroach &

# Rotated fonts and antialiasing demo
#T1LIB_CONFIG=./fonts/type1/t1lib.config bin/nano-X -L & bin/demo-aafont
#bin/nano-X -A & bin/demo-aafont &

# Antigrain Geometry subpixel rendering demo
# -A parameter demonstrates auto portrait reorientation by moving mouse cursor to borders
#bin/nano-X -A -x 900 -y 700 & bin/demo-agg & bin/nxmag &

# Blitter demo
#bin/nano-X -A -x 810 -y 640 & bin/demo-blit & bin/nxmag &

# Compositing demo
#bin/nano-X -A -x 900 -y 700 & bin/demo-composite & bin/nxmag &

# grab the 'a' key as exclusive hotkey
#bin/demo-grabkey 97 n &

# Graphics drawing demo
#bin/demo-region & bin/demo-dash & bin/demo-tilestipple & bin/demo-arc & bin/demo-polygon & bin/nxtux & bin/nxmag &

# monochrome bitmap test
#bin/nano-X -A -x 900 -y 700 & bin/demo-monobitmap & bin/nxmag &

# nuklear UI demo
#bin/demo-nuklear-calculator & bin/demo-nuklear-demo & bin/demo-nuklear-overview & bin/demo-nuklear-node_editor

# Nano-X terminal in portrait mode with eyes
# -L runs in left portrait mode, -N sets no rotation
#bin/nano-X -L & bin/nxterm & bin/nxeyes & bin/tux &

# requires MW_FEATURES_RESIZEFRAME and LINK_APP_INTO_SERVER
#bin/mwterm &
#bin/mwdemo2 &
#bin/mwmine &
#bin/mwcontrols &
#bin/openfile &
#bin/demo-composite &
#bin/demo-nuklear-overview &
#bin/demo-nuklear-node_editor &
#bin/demo-ttfont &

# Scaled fonts demo
#bin/nano-X -N & bin/demo-ttfont $1 & bin/nxmag &

# Nano-X desktop and application launcher
#bin/nano-X -x 1024 -y 768 & bin/launcher bin/launcher.cnf &

# terminal emulator with onscreen keyboard and handwriting recognition
#bin/nano-X -N -x 1024 -y 768 & bin/nxterm & bin/nxscribble & bin/nxkbd & bin/nxterm & bin/nxroach -roaches 10 &

# image viewer demos for different image formats, bpp and portrait modes
#
# GIF image, test stretchblit
#bin/nano-X & bin/nxview -p -s bin/tux.gif & bin/nxmag &

# PPM image
#bin/nano-X & bin/nxview bin/mwlogo.ppm & bin/nxclock & bin/nxmag &

# png 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 768 -N & bin/tux& bin/nxview -p images/test/32rgba.png

# image viewer demo showing alpha blending
# alphademo.png is a center red dice, left blue, bottom yellow, right green
#bin/nano-X -x 1024 -y 768 -N & bin/nxview -p images/test/alphademo.png

# png 24bpp RGB copy
#bin/nano-X -x 1024 -y 400 -N & bin/tux& bin/nxview -p images/test/24rgb.png
#bin/nano-X -x 1024 -y 400 -N & bin/nxview -p images/test/24rgb.png

# tif 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 400 -L & bin/tux& bin/nxview -p images/test/strike.tif
#bin/nano-X -x 1024 -y 400 -L & bin/tux& bin/nxview -p images/test/warriors.tif

# xpm 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 768 -N & bin/tux& bin/nxview -p images/test/XFree86.xpm

# jpg 24bpp RGB copy
#bin/nano-X -x 1024 -y 768 -L & bin/tux& bin/nxview -p images/test/earth.jpg

# bmp 24 RGB copy, red in top left
#bin/nano-X -x 1024 -y 768 -L & bin/tux& bin/nxview -p images/test/g24.bmp

# bmp 32 RGBA 255 alpha copy, red in top left
#bin/nano-X -x 1024 -y 768 -L & bin/tux& bin/nxview -p images/test/g32def.bmp

# PCF loadable compressed font demo, X11 fonts
#bin/show-font /usr/lib/X11/fonts/misc/7x14.pcf.gz
#bin/show-font /usr/lib/X11/fonts/100dpi/helvB12.pcf.gz
#bin/show-font /usr/lib/X11/fonts/misc/vga.pcf.gz
#bin/show-font /usr/lib/X11/fonts/misc/6x13.pcf.gz
#bin/show-font /usr/lib/X11/fonts/misc/9x15.pcf.gz
#bin/show-font fonts/pcf/jiskan24.pcf.gz
#bin/show-font /usr/share/fonts/X11/misc/cursor.pcf.gz & bin/show-font fonts/pcf/vga.pcf.gz & bin/show-font fonts/pcf/helvB12_lin.pcf.gz & bin/show-font fonts/pcf/jiskan24.pcf.gz &

#bin/show-font fonts/pcf/helvB12_lin.pcf.gz & bin/show-font fonts/pcf/gb24st.pcf.gz & bin/show-font fonts/pcf/jiskan24.pcf.gz
#bin/show-font fonts/pcf/helvB12_lin.pcf.gz &
#bin/show-font fonts/pcf/gb24st.pcf.gz &
#bin/show-font fonts/pcf/jiskan24.pcf.gz &

# load and display PPM
#bin/nano-X & bin/show-ppm bin/mwlogo.ppm
