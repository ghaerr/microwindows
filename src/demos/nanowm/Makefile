##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC = ../..
endif
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Preprocessor flags OTHER than defines
CPPFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(NANOWM), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS = nanowm.o events.o wlist.o clients.o actions.o

ifeq ($(ARCH), ECOS)
TARGET = $(MW_DIR_BIN)/nanowm.o
else
TARGET = $(MW_DIR_BIN)/nanowm
endif

all: default $(TARGET)

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/nanowm: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS)
else
ifeq ($(ARCH), ECOS)
$(MW_DIR_BIN)/nanowm.o: $(OBJS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -Wl,-r -Wl,--defsym -Wl,nanowm_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o
else
$(MW_DIR_BIN)/nanowm: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS)
endif
endif
