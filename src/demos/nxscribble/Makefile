##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/nxscribble/
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

######################## Additional Flags section ############################

# Directories list for header files
INCLUDEDIRS +=
# Defines for preprocessor
DEFINES +=

EXTRAFLAGS += -Wno-missing-prototypes				# turn off missing prototype warnings in nxscribble compile
# Compilation flags for C files OTHER than include directories
CFLAGS +=
# Linking flags
LDFLAGS +=

############################# targets section ################################

ifeq ($(NANOX), Y)
ifeq ($(NANOXDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS := \
	$(MW_DIR_OBJ)/demos/nxscribble/nxscribble.o \
	$(MW_DIR_OBJ)/demos/nxscribble/scribwidget.o \
	$(MW_DIR_OBJ)/demos/nxscribble/hre_api.o \
	$(MW_DIR_OBJ)/demos/nxscribble/li_recognizer.o \
	$(MW_DIR_OBJ)/demos/nxscribble/bitvector.o \
	$(MW_DIR_OBJ)/demos/nxscribble/matrix.o \
	$(MW_DIR_OBJ)/demos/nxscribble/sc.o \
	$(MW_DIR_OBJ)/demos/nxscribble/util.o

ifeq ($(ARCH), ECOS)
DEFINES += -DREC_DEFAULT_USER_DIR=\"/bin\"
TARGET = $(MW_DIR_BIN)/nxscribble.o
else
TARGET = $(MW_DIR_BIN)/nxscribble
endif

all: default $(TARGET)
	$(CP) \
		$(MW_DIR_SRC)/demos/nxscribble/letters.cl \
		$(MW_DIR_SRC)/demos/nxscribble/digits.cl \
		$(MW_DIR_SRC)/demos/nxscribble/punc.cl \
		$(MW_DIR_BIN)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

$(MW_DIR_BIN)/nxscribble: $(OBJS) $(NANOXCLIENTLIBS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(NANOXCLIENTLIBS) $(LDFLAGS) $(LIBM)

ifeq ($(ARCH), ECOS)
$(MW_DIR_BIN)/nxscribble.o: $(OBJS) $(CONFIG)
	@echo "Linking $(patsubst $(MW_DIR_BIN)/%,%,$@) ..."
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -Wl,-r -Wl,--defsym -Wl,nxscribble_main=main -o XX.o
	$(NM) XX.o | grep -v _main | grep ' T' | awk -f $(MW_DIR_SRC)/ecos/retain.awk | xargs $(OBJCOPY) XX.o $@
	rm -f XX.o
endif
