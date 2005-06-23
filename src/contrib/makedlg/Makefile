
CC = gcc


CFLAGS = -I.

OBJS = makedlg.o 



#
# Compilation target for C files
#
%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c $(CFLAGS) -o $@ $<



makedlg: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@

