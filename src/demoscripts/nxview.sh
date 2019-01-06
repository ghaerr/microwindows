# image viewer demos for different image formats, bpp and portrait modes

# GIF image, test stretchblit
bin/nano-X & bin/nxview -p -s bin/tux.gif & bin/nxmag &

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
