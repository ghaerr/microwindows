# image viewer demos for different image formats, bpp and portrait modes

# GIF image, test stretchblit
#bin/nano-X & bin/nxview -p -s bin/tux.gif &

# PPM image
bin/nano-X & bin/nxview bin/mwlogo.ppm & bin/nxclock & bin/nxmag &

# png 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 768 -N & bin/tux& bin/nxview -p mwin/bmp/32rgba.png

# alphademo.png is a center red dice, left blue, bottom yellow, right green
#bin/nano-X -x 1024 -y 768 -L &  bin/nxview -p mwin/bmp/alphademo.png

# png 24bpp RGB copy
#bin/nano-X -x 1024 -y 400 -N & bin/tux& bin/nxview -p mwin/bmp/24rgb.png
#bin/nano-X -x 1024 -y 400 -N & bin/nxview -p mwin/bmp/24rgb.png

# tif 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 400 -L & bin/tux& bin/nxview -p mwin/bmp/strike.tif
#bin/nano-X -x 1024 -y 400 -L & bin/tux& bin/nxview -p mwin/bmp/warriors.tif

# xpm 32bpp RGBA srcover
#bin/nano-X -x 1024 -y 768 -N & bin/tux& bin/nxview -p mwin/bmp/XFree86.xpm

# jpg 24bpp RGB copy
#bin/nano-X -x 1024 -y 768 -L & bin/tux& bin/nxview -p mwin/bmp/earth.jpg

# bmp 24 RGB copy, red in top left
#bin/nano-X -x 1024 -y 768 -L & bin/tux& bin/nxview -p mwin/bmp/g24.bmp

# bmp 32 RGBA 255 alpha copy, red in top left
#bin/nano-X -x 1024 -y 768 -L & bin/tux& bin/nxview -p mwin/bmp/g32def.bmp
