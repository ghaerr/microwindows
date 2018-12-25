#Makefile for project calculator
	
#Generated Thu Mar 15 11:09:45 2001
#by TinyWidgets Designer
	
CC=gcc
CFLAGS=-Wall -O2 -I ../../../include -I ../../../../../include
LIBS=-ltnW -lnano-X -L ../../../lib
			
OBJS=main.o interface.o support.o callback.o
calculator: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
			
clean:
	@rm -f *.o core *~ calculator
			
%.o:%.c
	$(CC) $(CFLAGS) -c $<
