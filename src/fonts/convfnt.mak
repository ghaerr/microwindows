#
# TEST2 makefile for Windows (SCROLLCREDITS)
#
MODEL=M
CFLAGS=-DSTRICT -W3
LFLAGS32=/map:convfnt.map
OBJS=convfnt.obj
WINPROG=convfnt
xLIBS32=fwiniow.lib futilw.lib jpeg\fjpgw.lib

all: $(WINPROG).exe

include <stddefs.mak>
