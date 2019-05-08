AGGSRC=../../../agg-2.6/agg-src
FREETYPEINC = -I/usr/local/include/freetype2
PLATFORM = X11
PLATFORMLIB = -lX11
PLATFORMINC = -I/usr/X11/include -L/usr/X11/lib
#PLATFORM = nano-X
#PLATFORMLIB = -lnano-X
#PLATFORMINC = -I../include -L../lib

AGGINC=-I$(AGGSRC)/include
AGGLIBS = -L$(AGGSRC)/src -lagg
FREETYPEOBJ = $(AGGSRC)/font_freetype/agg_font_freetype.o
FREETYPELIB = $(FREETYPEOBJ) -lfreetype
AGGINC += $(FREETYPEINC) -I$(AGGSRC)/font_freetype
AGGLIBS += $(FREETYPELIB)
PLATFORMOBJ = $(AGGSRC)/src/platform/$(PLATFORM)/agg_platform_support.o
# AGG_BGRA32 = MWPF_TRUECOLORARGB (AGG names from memory byte order, MW names from in register hi-low)
PIXFMT=AGG_BGRA32

CXX = g++
CXXFLAGS = -D$(PIXFMT) -O3 $(AGGINC) $(PLATFORMINC)
LIBS = $(PLATFORMOBJ) $(AGGLIBS) -lm $(PLATFORMLIB)

all: shapes freetype_test

shapes: shapes.o
	$(CXX) $(CXXFLAGS) $^ -o shapes $(LIBS)

freetype_test: freetype_test.o
	$(CXX) $(CXXFLAGS) $^ -o freetype_test $(LIBS)
	
%.o:	%.cpp
	@echo \< $*.cpp \>
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $@

clean:
	rm -f shapes freetype_test *.o

