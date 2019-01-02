##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)/../..
endif
MW_DIR_RELATIVE := demos/nxkbd/
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

ifeq ($(MICROWIN), Y)
ifeq ($(MICROWINDEMO), Y)

# If you want to create a library with the objects files, define the name here
LIBNAME =

# List of objects to compile
OBJS :=	$(MW_DIR_OBJ)/demos/mwdvetest/mwdvetest.o

FONTSPATH = $(MW_DIR_BIN)/fonts
FONTS = $(FONTSPATH)/truetype/Accidental\ Presidency.ttf \
		$(FONTSPATH)/truetype/VAGRounded\ Lt.ttf \
		$(FONTSPATH)/truetype/Bitstream\ Cyberbit.ttf
		
CYBERBIT = $(FONTSPATH)/truetype/Cyberbit.ttf

DEMOS = $(MW_DIR_BIN)/fonts $(FONTS)
DEMOS += $(MW_DIR_BIN)/mwdvetest $(MW_DIR_BIN)/mwdvetest.res 

all: default $(DEMOS)

endif
endif

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################

$(MW_DIR_BIN)/mwdvetest: $(OBJS) $(MWINLIBS) $(CONFIG)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@ $(MWINLIBS) $(LDFLAGS)

$(MW_DIR_BIN)/mwdvetest.res:
	ln -s ../demos/mwdvetest/dveres.res $@

$(FONTSPATH):
	(cd $(MW_DIR_BIN) && ln -s ../fonts)
		
$(CYBERBIT):
#	wget ftp://ftp.netscape.com/pub/communicator/extras/fonts/windows/Cyberbit.ZIP && unzip Cyberbit.ZIP
#	wget ftp://ftp.netscape.com/pub/communicator/extras/fonts/windows/License.wri
#	mv Cyberbit.ttf License.wri "$(FONTSPATH)/truetype/"
#	rm Cyberbit.ZIP
	
$(FONTS): $(FONTSPATH) $(CYBERBIT)
#	test -L $(FONTSPATH)/truetype/Accidental\ Presidency.ttf || ln -s accid___.ttf $(FONTSPATH)/truetype/Accidental\ Presidency.ttf
#	test -L $(FONTSPATH)/truetype/VAGRounded\ Lt.ttf || ln -s VAGROLN.TTF  $(FONTSPATH)/truetype/VAGRounded\ Lt.ttf
#	test -L $(FONTSPATH)/truetype/Bitstream\ Cyberbit.ttf || ln -s Cyberbit.ttf  $(FONTSPATH)/truetype/Bitstream\ Cyberbit.ttf

