
all:
	g++ -I.. -I../../../include testcpp.cpp -o testcpp graphics.cpp apprun.cpp appevent.cpp ../agglite.cpp  ../../../obj/fonts/X6x13.o ../../../obj/fonts/winFreeSansSerif11x13.o -L../../../lib -L/opt/X11/lib -lmwengine -lmwimages -lX11 -lfreetype -lz
	g++ -I.. -I../../../include aggdemo.cpp -o aggdemo ../agglite.cpp ../../../obj/fonts/X6x13.o ../../../obj/fonts/winFreeSansSerif11x13.o -L../../../lib -L/opt/X11/lib -lmwengine -lmwimages -lX11 -lfreetype -lz
	cc -I.. -I../../../include testc.c -o testc ../../../obj/fonts/X6x13.o ../../../obj/fonts/winFreeSansSerif11x13.o -L../../../lib -L/opt/X11/lib -lmwengine -lmwimages -lX11 -lfreetype -lz -lm

clean:
	rm testcpp testc aggdemo
