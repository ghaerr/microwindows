CC = gcc

all: speed xspeed

speed.o : speed.c
	$(CC) -DX11=1 -c $<

speed: speed.o
	$(CC) -DX11=1 $< -o $@ \
		-lnano-X -lmwengine -lmwfonts -lmwdrivers \
		-L/usr/X11R6/lib -lX11

xspeed.o : speed.c
	$(CC) -DTEST_FOR_X=1 -c $< -o $@

xspeed: xspeed.o
	$(CC) -DTEST_FOR_X=1 $< -o $@ \
		-L/usr/X11R6/lib -lX11
