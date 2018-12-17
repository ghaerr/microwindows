#Makefile for project demos
	
#Generated Mon Dec  3 20:16:34 2018
#by TinyWidgets Designer
	
CC=gcc
CFLAGS=-Wall -O2
LIBS=-ltnW -lnano-X
			
OBJS=main.o interface.o support.o callback.o			
demos: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)
			
clean:
	@rm -f *.o core *~ demos
			
%.o:%.c
		$(CC) $(CFLAGS) -c $<
