##############################################################################
# Microwindows template Makefile
# Copyright (c) 2000 Martin Jolicoeur, Greg Haerr
##############################################################################

TOP = $(shell pwd)

# See if the user has a local config file. If so use that else
# use the one provided in the distribution.
ifeq ($(HOME)/microwin/config,$(wildcard $(HOME)/microwin/config))
CONFIG = $(HOME)/microwin/config
else
CONFIG = $(TOP)/config
endif

include $(CONFIG)

############################# targets section ################################

# If you want to create a library with the objects files, define the name here
LIBNAME =
LIBNAMESO =

# List of objects to compile
OBJS =	

# demos should be built after the libs !
dirs = drivers mwin engine fonts nanox

all: default
	-$(MAKE) -C demos
ifeq ($(ARCH), ECOS)
	$(MAKE) -C ecos
endif

realclean: clean
	$(MAKE) -C fonts realclean
	$(MAKE) -C mwin/bmp realclean

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
	doxygen Doxyfile-internal
doc-nanox:
	doxygen Doxyfile-nanox
pdfdoc-nanox: doc-nanox
	cd ../doc/nano-X/latex && $(MAKE)
doc-clean:
	rm -rf ../doc/nano-X ../doc/internal

######################### Makefile.rules section #############################

include $(TOP)/Makefile.rules

######################## Tools targets section ###############################
