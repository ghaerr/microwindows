# Note! You need to have gtk-doc, docbook, jade, tex, and various other
# things set up and playing nicely before you will be able to rebuild
# this documentation. If you just want to read the docs, use the prebuilt
# copies provided.

MODULE=nano-X
SRCDIR=../../src/
SOURCEDIR=$(SRCDIR)nanox/
INCLUDES=$(SRCDIR)/include/nano-X.h
MODFLG=--module=$(MODULE)

.PHONY : scan templates sgml html ps pdf dvi rtf clean

all: pdf html cleanup

scan: $(INCLUDES)
	gtkdoc-scan $(MODFLG) --source-dir=$(SOURCEDIR) $(INCLUDES)

templates: scan
	gtkdoc-mktmpl $(MODFLG)

sgml: templates
	gtkdoc-mkdb $(MODFLG) --source-dir=$(SOURCEDIR)

html: sgml $(MODULE)-docs.sgml
	if ! test -d html ; then mkdir html ; fi
	cd html && gtkdoc-mkhtml $(MODFLG) ../$(MODULE)-docs.sgml

ps: sgml $(MODULE)-docs.sgml
	db2ps $(MODULE)-docs.sgml

pdf: ps
	ps2pdf $(MODULE)-docs.ps

cleanup:
	rm -f sgml/* tmpl/* *.bak *.log $(MODULE)-unused.txt $(MODULE)-decl.txt $(MODULE)-decl-list.txt *.tex *.dvi *.aux
	if test -d tmpl ; then rmdir tmpl ; fi
	if test -d sgml ; then rmdir sgml ; fi

clean: cleanup
	rm -f html/* *.pdf *.ps
	if test -d html ; then rmdir html ; fi
