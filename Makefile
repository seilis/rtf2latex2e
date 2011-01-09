VERSION = 1-8-0

CC?=gcc
TAR?=gnutar
RM?=rm -f
MKDIR?=mkdir -p
RMDIR?=rm -rf

#reasonable default set of compiler flags
CFLAGS=-g -Wall -Wno-write-strings

PLATFORM?=-DUNIX   # Mac OS X, Linux, BSD
#PLATFORM?=-DRTF2LATEX2E_DOS # Windows/DOS

#Base directory - adapt as needed
# Unix:
PREFIX?=/opt/local
#Uncomment next lines for DOS/Windows
#PREFIX_DRIVE=C:
#PREFIX?=$(PREFIX_DRIVE)/PROGRA~1/rtf2latex

BINARY_NAME=rtf2latex2e

# Location of binary, man, info, and support files - adapt as needed
BIN_INSTALL    =$(PREFIX)/bin
SUPPORT_INSTALL=$(PREFIX)/share/rtf2latex2e

# MacOS X flags to support PICT -> PDF conversion
CFLAGS  :=$(CFLAGS) -m32 -DPICT2PDF
LDFLAGS :=$(LDFLAGS) -m32 -framework ApplicationServices

# Uncomment to get debugging information about OLE translation
#CFLAGS:=$(CFLAGS) -DCOLE_VERBOSE

# Nothing to change below this line

CFLAGS:=$(CFLAGS) $(PLATFORM)

SRCS         = src/cole.c                 src/cole_decode.c          src/cole_support.c      \
               src/eqn.c                  src/main.c                 src/mygetopt.c          \
               src/reader.c               src/writer.c               
               
HDRS         = src/cole.h                 src/cole_support.h         src/eqn.h               \
               src/eqn_support.h          src/mygetopt.h             src/rtf2latex2e.h       \
               src/rtf.h

RTFPREP_SRCS = src/rtf-controls           src/rtfprep.c              src/standard-names      \
               src/tokenscan.c            src/tokenscan.h            src/rtf-ctrldef.h       \
               src/rtf-namedef.h          src/stdcharnames.h         src/rtf-ctrl

RTFPREP_OBJS = src/rtfprep.o              src/tokenscan.o

PREFS        = pref/TeX-map               pref/TeX-map.latin1        pref/cp437.map          \
               pref/TeX-map.applemac      pref/ansi-sym              pref/cp850.map          \
               pref/TeX-map.cp1250        pref/applemac.map          pref/mac-sym            \
               pref/TeX-map.cp1252        pref/cp1250.map            pref/pc-sym             \
               pref/TeX-map.default       pref/cp1252.map            pref/pca-sym            \
               pref/TeX-map.german        pref/cp1254.map            pref/r2l-head           \
               pref/r2l-map               pref/r2l-pref              pref/rtf-ctrl

DOCS         = doc/GPL_license            doc/Release-notes.txt      README                  \
               doc/rtf2latex2eSWP.tex     doc/rtfReader.tex          doc/rtf2latex2eDoc.tex
               
PDFS         = doc/rtf2latex2eSWP.pdf     doc/rtfReader.pdf          doc/rtf2latex2eDoc.pdf  

TEST         = test/Makefile              test/arch.rtf              test/arch-mac.rtf       \
               test/equation.rtf          test/fig-jpeg.rtf          test/multiline.rtf      \
               test/mapping.rtf           test/rtf-misc.rtf          test/rtf.rtf            \
               test/table.rtf             test/test.rtf              test/moreEqns.rtf       \
               test/twoEqn.rtf            \

RTFD         = test/sample.rtfd/TXT.rtf      test/sample.rtfd/amiga.gif \
               test/sample.rtfd/build.tiff   test/sample.rtfd/button_smile.jpeg \
               test/sample.rtfd/paste.eps

EQNS         = test/testeqn01.eqn         test/testeqn02.eqn         test/testeqn03.eqn      \
               test/testeqn04.eqn         test/testeqn05.eqn         test/testeqn06.eqn      \
               test/testeqn07.eqn         test/testeqn08.eqn         test/testeqn09.eqn      \
               test/testeqn10.eqn
               
OBJS         = src/cole.o                 src/cole_decode.o          src/cole_support.o      \
               src/eqn.o                  src/main.o                 src/mygetopt.o          \
               src/reader.o               src/tokenscan.o            src/writer.o

all : checkfiles rtf2latex2e

src/rtfprep: src/tokenscan.o src/rtfprep.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(RTFPREP_OBJS) -o src/rtfprep

rtf2latex2e: $(OBJS) $(HDRS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(BINARY_NAME)
	cp $(BINARY_NAME) rtf2latex

src/main.o: Makefile src/main.c
	$(CC) $(CFLAGS) -DLIBDIR=\"$(SUPPORT_INSTALL)\" -c src/main.c -o src/main.o

doc : doc/rtf2latex2eSWP.tex doc/rtfReader.tex doc/rtf2latex2eDoc.tex
	cd doc && $(MAKE)

test: rtf2latex2e
	cd test && $(MAKE)

checkfiles: $(SRCS) $(RTFPREP_SRCS) $(HDRS) $(PREFS) $(TEST) $(DOCS) Makefile

depend: $(SRCS)
	$(CC) -MM $(SRCS) >makefile.depend
	@echo "***** Append makefile.depend to Makefile manually ******"

dist: checkfiles doc $(SRCS) $(RTFPREP_SRC) $(HDRS) $(README) $(PREFS) $(TEST) $(DOCS) Makefile
	make doc
	$(MKDIR)           rtf2latex2e-$(VERSION)
	$(MKDIR)           rtf2latex2e-$(VERSION)/pref
	$(MKDIR)           rtf2latex2e-$(VERSION)/doc
	$(MKDIR)           rtf2latex2e-$(VERSION)/test
	$(MKDIR)           rtf2latex2e-$(VERSION)/test/sample.rtfd
	$(MKDIR)           rtf2latex2e-$(VERSION)/src
	ln README          rtf2latex2e-$(VERSION)
	ln Makefile        rtf2latex2e-$(VERSION)
	ln $(SRCS)         rtf2latex2e-$(VERSION)/src
	ln $(HDRS)         rtf2latex2e-$(VERSION)/src
	ln $(RTFPREP_SRCS) rtf2latex2e-$(VERSION)/src
	ln $(PREFS)        rtf2latex2e-$(VERSION)/pref
	ln $(DOCS)         rtf2latex2e-$(VERSION)/doc
	ln $(PDFS)         rtf2latex2e-$(VERSION)/doc
	ln $(TEST)         rtf2latex2e-$(VERSION)/test
	ln $(RTFD)         rtf2latex2e-$(VERSION)/test/sample.rtfd
	ln $(EQNS)         rtf2latex2e-$(VERSION)/test
#	tar cvf - rtf2latex2e-$(VERSION) | gzip > rtf2latex2e-$(VERSION).tar.gz
	zip -r rtf2latex2e-$(VERSION) rtf2latex2e-$(VERSION)
	rm -rf rtf2latex2e-$(VERSION)
	
install: rtf2latex2e doc
	$(MKDIR)                   $(BIN_INSTALL)
	$(MKDIR)                   $(SUPPORT_INSTALL)
	cp $(BINARY_NAME)          $(BIN_INSTALL)
	cp $(PREFS)                $(SUPPORT_INSTALL)
	cp doc/rtf2latex2eDoc.pdf  $(SUPPORT_INSTALL)
	@echo "******************************************************************"
	@echo "*** rtf2latex2e successfully installed as \"$(BINARY_NAME)\""
	@echo "*** in directory \"$(BIN_INSTALL)\""
	@echo "***"
	@echo "*** rtf2latex2e was compiled to search for its configuration files in"
	@echo "***           \"$(SUPPORT_INSTALL)\" "
	@echo "***"
	@echo "*** If the configuration files are moved then either"
	@echo "***   1) set the environment variable RTFPATH to this new location, or"
	@echo "***   2) use the command line option -P /path/to/prefs, or"
	@echo "***   3) edit the Makefile and recompile"
	@echo "******************************************************************"

clean: 
	rm -f $(OBJS) $(RTFPREP_OBJS) $(BINARY_NAME) rtf2latex
	cd test   && make clean
	cd doc    && make clean
	
realclean: checkfiles clean
	rm -f makefile.depend
	rm -f src/rtfprep
	cd test   && make realclean
	cd doc    && make realclean

parser: checkfiles clean
	rm -f src/rtfprep
	rm -f src/rtf-ctrldef.h  src/rtf-namedef.h  src/stdcharnames.h src/rtf-ctrl
	make src/rtfprep	
	cd src && ./rtfprep

appleclean:
	sudo xattr -r -d com.apple.FinderInfo ../trunk
	sudo xattr -r -d com.apple.TextEncoding ../trunk
	
splint: 
	splint -weak $(SRCS) $(HDRS)
	
.PHONY: all checkfiles clean depend dist doc install realclean test

# created using "make depend"
src/cole.o:          src/cole.c src/cole.h src/cole_support.h
src/cole_decode.o:   src/cole_decode.c src/cole.h src/cole_support.h
src/cole_support.o:  src/cole_support.c src/cole_support.h
src/eqn.o:           src/eqn.c src/rtf.h src/rtf-ctrldef.h src/rtf-namedef.h \
                     src/rtf2latex2e.h src/cole_support.h src/eqn.h src/eqn_support.h
src/main.o:          src/main.c src/rtf.h src/rtf-ctrldef.h src/rtf-namedef.h src/mygetopt.h src/rtf2latex2e.h
src/mygetopt.o:      src/mygetopt.c src/mygetopt.h
src/reader.o:        src/reader.c src/tokenscan.h src/rtf.h src/rtf-ctrldef.h \
                     src/rtf-namedef.h src/rtf2latex2e.h src/stdcharnames.h
src/writer.o:        src/writer.c src/rtf.h src/rtf-ctrldef.h src/rtf-namedef.h \
                     src/tokenscan.h src/cole.h src/cole_support.h src/rtf2latex2e.h src/eqn.h
