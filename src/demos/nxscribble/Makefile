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

ifeq ($(NANOX), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS = nxscribble.o scribwidget.o\
	hre_api.o li_recognizer.o bitvector.o matrix.o sc.o util.o

ifeq ($(ARCH), ECOS)
DEFINES += -DREC_DEFAULT_USER_DIR=\"/bin\"
TARGET = $(MW_DIR_BIN)/nxscribble.o
else
TARGET = $(MW_DIR_BIN)/nxscribble
endif

all: default $(TARGET)
	$(CP) letters.cl digits.cl punc.cl $(MW_DIR_BIN)

endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

ifeq ($(SHAREDLIBS), Y)
$(MW_DIR_BIN)/nxscribble: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(CCNANOXCLIENTLIBS) -lm
else
ifeq ($(ARCH), ECOS)
$(MW_DIR_BIN)/nxscribble.o: $(OBJS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -Wl,-r -Wl,--defsym -Wl,nxscribble_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o
else
$(MW_DIR_BIN)/nxscribble: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS) -lm
endif
endif
