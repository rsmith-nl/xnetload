# $Id: Makefile,v 1.15 2002/07/23 09:04:09 rsmith Exp rsmith $
# This is the Makefile for xnetload

# If make complains about a missing file, run 'make depend' first

# Define the C compiler to be used, usually gcc.
CC = gcc

# Choose an appropriate CFLAGS and LFLAGS

# The next two lines are for building an executable suitable for debugging.
#CFLAGS = -pipe -g -O0 -Wall -I/usr/X11R6/include
#LFLAGS = -pipe -L/usr/X11R6/lib

# The next two lines are for building an optimized and stripped program.
CFLAGS = -pipe -O2 -Wall -DNDEBUG -I/usr/X11R6/include
LFLAGS = -s -pipe -L/usr/X11R6/lib

# These three lines are for building Athlon optimized programs on my system.
#CC=gcc-3.1
#CFLAGS = -s -O6 -ffast-math -fomit-frame-pointer -Wall -march=athlon -funroll-loops -fexpensive-optimizations -fschedule-insns2 -DNDEBUG -I/usr/X11R6/include
#LFLAGS = -s -pipe -L/usr/X11R6/lib

# Libraries to link against
LIBS = -lXaw -lXmu -lXt -lX11 -lm

# Location where to install the binary.
BINDIR = /usr/local/bin

# Location where to install the manual-page.
MANDIR = /usr/local/man/man1

##### Maintainer stuff goes here:

# Package name and version: BASENAME-VMAJOR.VMINOR.VPATCH.tar.gz
BASENAME = xnetload
VMAJOR   = 1
VMINOR   = 11
VPATCH   = 3

# Standard files that need to be included in the distribution
DISTFILES = README COPYING Makefile $(BASENAME).1

# Source files.
SRCS = data.c x11-ui.c

# Extra stuff to add into the distribution.
XTRA_DIST= XNetload Makefile.static

##### No editing necessary beyond this point
# Object files.
OBJS = $(SRCS:.c=.o)

# Predefined directory/file names
PKGDIR  = $(BASENAME)-$(VMAJOR).$(VMINOR).$(VPATCH)
TARFILE = $(BASENAME)-$(VMAJOR).$(VMINOR).$(VPATCH).tar.gz
BACKUP  = $(BASENAME)-backup-$(VMAJOR).$(VMINOR).$(VPATCH).tar.gz

# Version number
VERSION = -DVERSION=\"$(VMAJOR).$(VMINOR).$(VPATCH)\"
# Program name
PACKAGE = -DPACKAGE=\"$(BASENAME)\"
# Add to CFLAGS
CFLAGS += $(VERSION) $(PACKAGE)

.PHONY: clean install uninstall dist backup all

all: $(BASENAME)

LOG = ChangeLog

$(LOG): $(SRCS) $(DISTFILES) $(XTRA_DIST)
	rm -f $(LOG) 
	rcs2log -i 2 -l 70 >$(LOG)

DISTFILES += $(LOG)

# builds a binary.
$(BASENAME): $(OBJS)
	$(CC) $(LFLAGS) $(LDIRS) -o $(BASENAME) $(OBJS) $(LIBS)

# Remove all generated files.
clean:;
	rm -f $(OBJS) $(BASENAME) *~ core \
	$(TARFILE) $(BACKUP) $(LOG)

# Install the program and manual page. You should be root to do this.
install: $(BASENAME)
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to install the program!"; \
		exit 1; \
	fi
	install -m 755 $(BASENAME) $(BINDIR)
	if [ -e $(BASENAME).1 ]; then \
		install -m 644 $(BASENAME).1 $(MANDIR); \
		gzip -f $(MANDIR)/$(BASENAME).1; \
	fi

uninstall:;
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall the program!"; \
		exit 1; \
	fi
	rm -f $(BINDIR)/$(BASENAME)
	rm -f $(MANDIR)/$(BASENAME).1*

# Build a tar distribution file. Only needed for the maintainer.
dist: clean $(DISTFILES) $(XTRA_DIST)
	rm -rf $(PKGDIR)
	mkdir -p $(PKGDIR)
	cp $(DISTFILES) $(XTRA_DIST) *.c *.h $(PKGDIR)
	tar -czf $(TARFILE) $(PKGDIR)
	rm -rf $(PKGDIR)

# Make a backup, complete with the RCS files. Only for the maintainer.
backup: clean $(LOG)
	rm -rf /tmp/$(PKGDIR)
	mkdir -p /tmp/$(PKGDIR)
	cp -R -p * /tmp/$(PKGDIR)
	CURDIR=`pwd`
	cd /tmp ; tar -czf $(CURDIR)/$(BACKUP) $(PKGDIR)
	rm -rf /tmp/$(PKGDIR)

# if the file depend doesn't exist, run 'make depend' first.
depend:
	gcc -MM $(OBJS:.o=.c) >depend

# DO NOT DELETE THIS LINE 
include depend
