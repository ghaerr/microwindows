#CC = mipsel-linux-gcc -msoft-float -s
CC = gcc
CFLAGS = -Wall -I../../../include -I../../../drivers -L../../../lib
LIBS = -lmwin
OBJECTS = tpcal.o transform.o

all: tpcal

tpcal: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) $(LIBS) -o tpcal

clean:
	rm -f *.o tpcal
