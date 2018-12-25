#Makefile for project hello
	
#Generated Thu Mar 15 11:46:54 2001
#by TinyWidgets Designer
	
CC=gcc
CFLAGS=-Wall -O2 -I ../../../include -I ../../../../../include
LIBS=-ltnW -lnano-X -L ../../../lib
			
OBJS=main.o interface.o support.o callback.o			
hello: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
			
clean:
	@rm -f *.o core *~ hello
			
%.o:%.c
	$(CC) $(CFLAGS) -c $<
