!include <ntwin32.mak>

FILENAME        = Makefile

CX		= $(cc)

#LD		= $(link) /debug:full
LD		= $(link)

MAKE		= nmake

#CXFLAGS		= $(cflags) $(cvars) -Zi -Od -W3
CXFLAGS		= $(cflags) $(cvars) -W3

LDFLAGS		= $(guiflags)

LIBS		= 

STDLIBS		= $(guilibs)

SRC1BASE	= convfnt

OBJS		= $(SRC1BASE).obj

PROGRAM		= $(SRC1BASE)32.exe

all: $(PROGRAM)

$(PROGRAM): $(OBJS) $(LIBS)
	$(LD) $(LDFLAGS) -out:$@ $(OBJS) $(LIBS) $(STDLIBS)
	@echo Done.

$(OBJS): $$(@B).c
	$(CX) $(CXFLAGS) $(@B).c

clean::
	@del $(OBJS)
	@del $(PROGRAM)
