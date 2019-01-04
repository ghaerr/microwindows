# OSX version
# demonstration of linking with same symbol name "x1"
#
# main -> x1() -> nx1() -> "real" x1();
#

all: clean main

clean:
	-rm -f *.o *.so *.a main

CFLAGS=-fpic
LDFLAGS=-shared -undefined dynamic_lookup -L`pwd`
NM=otool -L

# main is linked with "wrapper" x1 (APP)
main: x1.so
	cc $(CFLAGS) -c main.c
	cc main.c -o main x1.so
	$(NM) main

# x1 wrapper calls nx1 (NXLIB)
x1.so: nx1.so
	cc $(CFLAGS) -c x1.c
	cc $(LDFLAGS) x1.o -o x1.so nx1.so
	$(NM) x1.so

# nx1 implementation calls real x1 (NXSTANDALONE = libnano-X.so w/LINK_APP_INTO_SERVER=Y)
nx1.so: realx1.so
	cc $(CFLAGS) -c nx1.c
	cc $(LDFLAGS) nx1.o -o nx1.so realx1.so
	$(NM) nx1.so

# the real x1 (X11)
realx1.so:
	cc $(CFLAGS) -c realx1.c
	cc $(LDFLAGS) realx1.o -o realx1.so
	$(NM) realx1.so
