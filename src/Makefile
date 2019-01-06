##############################################################################
# Microwindows top-level Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

ifndef MW_DIR_SRC
MW_DIR_SRC := $(CURDIR)
endif
MW_DIR_RELATIVE :=
include $(MW_DIR_SRC)/Path.rules
include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME =
LIBNAMESO =

# List of objects to compile
OBJS =	

# demos should be built after the libs !
dirs := \
	$(MW_DIR_SRC)/drivers \
	$(MW_DIR_SRC)/engine \
	$(MW_DIR_SRC)/fonts \
	$(MW_DIR_SRC)/images

ifeq ($(MICROWIN), Y)
dirs += $(MW_DIR_SRC)/mwin
endif

ifeq ($(NANOX), Y)
dirs += $(MW_DIR_SRC)/nanox
endif

ifeq ($(NX11), Y)
dirs += $(MW_DIR_SRC)/nx11
endif

all: default
ifeq ($(ARCH), RTEMS)
	$(MAKE) -C $(MW_DIR_SRC)/rtems
endif
ifeq ($(ARCH), ECOS)
	$(MAKE) -C $(MW_DIR_SRC)/ecos
endif
	-$(MAKE) -C $(MW_DIR_SRC)/demos
ifeq ($(TINYWIDGETS), Y)
	$(MAKE) -C $(MW_DIR_SRC)/contrib/TinyWidgets 
endif

.PHONY: realclean

realclean: clean
ifeq ($(MW_DIR_OBJ),$(MW_DIR_SRC))
	$(MAKE) -C $(MW_DIR_SRC)/fonts realclean
	$(MAKE) -C $(MW_DIR_SRC)/images realclean
endif

#
# Documentation targets:
#
# doc          - All HTML docs.
# pdfdoc       - All HTML and PDF docs - i.e. everything.
# doc-nanox    - Documentation for public API - HTML.
# pdfdoc-nanox - Documentation for public API - HTML and PDF.
# doc-internal - Documentation for everything, including internal
#                   functions - HTML.
#
# Note that there are no internal PDF docs - the internal APIs
# change, so printed docs are less useful.
#
# Also note that PDF requires a working LaTEX install.  HTML will
# work without LaTEX.
#
# The docs end up in microwin/doc/{nano-X,internal}/{html,latex}/
#
.PHONY: doc doc-internal doc-nanox pdfdoc pdfdoc-nanox

doc: doc-internal doc-nanox
	@#
pdfdoc: doc-internal pdfdoc-nanox
	@#
doc-internal:
	doxygen ../doc/Doxyfile-internal
doc-nanox:
	doxygen ../doc/Doxyfile-nanox
pdfdoc-nanox: doc-nanox
	cd ../doc/nano-X/latex && $(MAKE)
doc-clean:
	rm -rf ../doc/nano-X ../doc/internal

######################### Makefile.rules section #############################

include $(MW_DIR_SRC)/Makefile.rules

######################## Tools targets section ###############################
