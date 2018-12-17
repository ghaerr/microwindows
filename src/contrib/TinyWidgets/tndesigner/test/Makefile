#Makefile for project test
	
#Generated Sun Dec  9 09:55:50 2018
#by TinyWidgets Designer
	
CC=gcc
CFLAGS=-Wall -O2
LIBS=-ltnW -lnano-X
			
OBJS=main.o interface.o support.o callback.o			
test: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
			
clean:
	@rm -f *.o core *~ test
			
%.o:%.c
		$(CC) $(CFLAGS) -c $<
