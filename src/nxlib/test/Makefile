
INCLUDE=-I..
LIBS=-L../../microwin/src/lib ../libNX11.a -lnano-X

all:
	gcc -o williams $(INCLUDE) williams.c $(LIBS)
	gcc -o queryfont $(INCLUDE) queryfont.c $(LIBS)
	gcc -o loadfont $(INCLUDE) loadfont.c $(LIBS)

clean:
	rm williams
	rm queryfont
	rm loadfont
