# Makefile for xnetload
#
# $Id: Makefile,v 1.2 1999/10/06 20:56:58 rsmith Exp rsmith $
#

# Location to install the binary.
BINDIR = /usr/local/bin

# Location to install the manual-page.
MANDIR = /usr/local/man/man1

# Location of the X11 header files. Try changing this if compilation fails due
# to missing include files.
HDIRS = -I/usr/X11R6/include
# Location of the X11 libraries. Try changing this if linking fails due to
# missing libraries. 
LDIRS = -L/usr/X11R6/lib

# Compilation flags.
# For gcc 2.95
#CFLAGS = -pipe -Wall -O3 -mcpu=pentium -ffast-math -fomit-frame-pointer
# For gcc 2.7.x
CFLAGS = -O2 -Wall -pipe -fomit-frame-pointer
# For debugging
#CFLAGS = -g -Wall -DDEBUG 

# Linking flags
LFLAGS = -pipe -Wall -s
# Linking flags for debugging
#LFLAGS = -pipe -Wall

# The compiler to use.
CC = gcc

#### You should not have to make changes beyond this point. ####

LIBS = -lXaw -lXmu -lXt -lX11 -lm


# Package name and version: BASENAME-VMAJOR.VMINOR.tar.gz
BASENAME = xnetload
VMAJOR   = 1
VMINOR   = 7
VPATCH   = 0

# Directory in which this library is built
BUILDDIR = $(BASENAME)-$(VMAJOR).$(VMINOR)

# If one of these is missing, comment them out!
README  = $(BUILDDIR)/README
CHLOG  = $(BUILDDIR)/CHANGELOG
LICENSE = $(BUILDDIR)/LICENSE
MANPAGE = $(BUILDDIR)/$(BASENAME).1
APPDEF  = $(BUILDDIR)/XNetload
EXTRAS = $(README) $(CHLOG)  $(LICENSE)  $(MANPAGE)  $(APPDEF) 

# Predefined file names
TARFILE = $(BASENAME)-$(VMAJOR).$(VMINOR).$(VPATCH).tar.gz
BACKUP  = $(BASENAME)-backup-$(VMAJOR).$(VMINOR).$(VPATCH).tar.gz

# Version number
VERSION = -DVERSION=\"$(VMAJOR).$(VMINOR).$(VPATCH)\"
# Construct CFLAGS
CXFLAGS = $(CFLAGS) $(VERSION)

# Object files.
OBJS = data.o x11-ui.o

##### No editing necessary beyond this point
.PHONY: clean install uninstall dist backup

# builds a binary.
$(BASENAME): $(OBJS)
	$(CC) $(LFLAGS) $(LDIRS) -o $(BASENAME) $(LIBS) $(OBJS)

# Remove all generated files.
clean:;
	rm -f $(OBJS) $(BASENAME) *~ core \
	$(TARFILE) $(BACKUP) 

# Install the program and manual page. You should be root to do this.
install: $(BASENAME)
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to install the program!"; \
		exit 1; \
	fi
	cp $(BASENAME) $(BINDIR)
	cp $(BASENAME).1 $(MANDIR)

uninstall:;
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall the program!"; \
		exit 1; \
	fi
	rm -f $(BINDIR)/$(BASENAME)
	rm -f $(MANDIR)/$(BASENAME).1

# Build a tar distribution file. Only needed for the maintainer.
dist:;
	cd .. ; tar -czf $(BUILDDIR)/$(TARFILE) $(EXTRAS) \
	$(BUILDDIR)/Makefile $(BUILDDIR)/depend \
	$(BUILDDIR)/*.h $(BUILDDIR)/*.c 

# Make a backup, complete with the RCS files. Only for the maintainer.
backup: clean
	cd .. ; \
	tar -czf $(BUILDDIR)/$(BACKUP) $(BUILDDIR)/*

# if the file depend doesn't exist, run 'make depend' first.
depend: $(OBJS:.o=.c)
	gcc -MM $(OBJS:.o=.c) >depend

# implicit rule (is needed because of HDIRS and CXFLAGS!)
.c.o:
	$(CC) $(CXFLAGS) $(CPPFLAGS) $(HDIRS) -c -o $@ $<

# DO NOT DELETE THIS FOLLOWING LINE 
include depend
