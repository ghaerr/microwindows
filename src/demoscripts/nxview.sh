cd ..
# image viewer demos for different image formats, bpp and portrait modes
case $# in

0) bin/nano-X -N & bin/nanowm & bin/nxview mwin/bmp/XFree86.xpm; sleep 10000;;

# e.g. ../bin/nxview.sh ../bin/tux.gif
1) bin/nano-X -N & bin/nanowm & bin/nxview $1 $2; sleep 10000;;

esac

# png 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 400 -L & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/32rgba.png; sleep 10000
# 			alphademo.png is a center red dice, left blue, bottom yellow, right green
#bin/nano-X -x 1024 -y 768 -L & bin/nanowm & bin/nxview -p mwin/bmp/alphademo.png; sleep 10000

# png 24bpp RGB copy
#bin/nano-X -x 1024 -y 400 -N & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/24rgb.png; sleep 10000
#bin/nano-X -x 1024 -y 400 -N & bin/nanowm &bin/nxview -p mwin/bmp/24rgb.png; sleep 10000

# tif 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 400 -L & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/strike.tif; sleep 10000
#bin/nano-X -x 1024 -y 400 -L & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/warriors.tif; sleep 10000

# xpm 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 768 -N & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/XFree86.xpm; sleep 10000

# jpg 24bpp RGB copy
#bin/nano-X -x 1024 -y 768 -L & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/earth.jpg; sleep 10000

# bmp 24 RGB copy, red in top left
#bin/nano-X -x 1024 -y 768 -L & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/g24.bmp; sleep 10000

# bmp 32 RGBA 255 alpha copy, red in top left
#bin/nano-X -x 1024 -y 768 -L & bin/nanowm &bin/tux& bin/nxview -p mwin/bmp/g32def.bmp; sleep 10000

cd demoscripts
