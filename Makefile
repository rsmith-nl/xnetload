# $Id$
# This is the Makefile for xnetload

# If make complains about a missing file, run 'make depend' first

# Define the C compiler to be used, usually gcc.
CC = gcc
#CC = gcc-3.1

# The next two lines are for building an executable suitable for debugging.
#CFLAGS = -pipe -g -O0 -Wall -I/usr/X11R6/include
#LFLAGS = -pipe -Wall -L/usr/X11R6/lib

# The next two lines are for building an optimized program.
CFLAGS = -pipe -O2 -Wall -DNDEBUG -I/usr/X11R6/include
LFLAGS = -s -pipe -Wall -L/usr/X11R6/lib

# These two lines are for building Athlon optimized programs with gcc-3.1.
#CFLAGS = -s -O3 -fomit-frame-pointer -Wall -march=athlon -funroll-loops -fexpensive-optimizations -fschedule-insns2 -DNDEBUG
#LFLAGS = -s -pipe -Wall

# Libraries to link against
LIBS =  -lXaw -lXmu -lXt -lX11 -lm

# Location where to install the binary.
BINDIR = /usr/local/bin

# Location where to install the manual-page.
MANDIR = /usr/local/man/man1

##### Maintainer stuff goes here:

# Package name and version: BASENAME-VMAJOR.VMINOR.VPATCH.tar.gz
BASENAME = xnetload
VMAJOR   = 1
VMINOR   = 11
VPATCH   = 2

# Files that need to be included in the distribution
DISTFILES = README COPYING Makefile $(BASENAME).1

# Source files.
SRCS = data.c x11-ui.c

# Extra stuff to add into the distribution.
XTRA_DIST= XNetload

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

LOG = ChangeLog
DISTFILES += $(LOG)

.PHONY: clean install uninstall dist backup all $(LOG)

all: $(BASENAME)

# builds a binary.
$(BASENAME): $(OBJS)
	$(CC) $(LFLAGS) $(LDIRS) -o $(BASENAME) $(OBJS) $(LIBS)

# compresses the manual page
$(BASENAME).1.gz: $(BASENAME).1
	cp $(BASENAME).1 $(BASENAME).1.org
	gzip -f $(BASENAME).1
	mv $(BASENAME).1.org $(BASENAME).1

# Remove all generated files.
clean:;
	rm -f $(OBJS) $(BASENAME) *~ core \
	$(TARFILE) $(BACKUP) $(BASENAME).1.gz

log: $(LOG)

$(LOG):;
	rm -f $(LOG) 
	rcs2log -i 2 -l 70 >$(LOG)

# Install the program and manual page. You should be root to do this.
install: $(BASENAME) $(BASENAME).1.gz
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to install the program!"; \
		exit 1; \
	fi
	cp $(BASENAME) $(BINDIR)
	cp $(BASENAME).1.gz $(MANDIR)

uninstall:;
	@if [ `id -u` != 0 ]; then \
		echo "You must be root to uninstall the program!"; \
		exit 1; \
	fi
	rm -f $(BINDIR)/$(BASENAME)
	rm -f $(MANDIR)/$(BASENAME).1*

# Build a tar distribution file. Only needed for the maintainer.
dist: $(DISTFILES) $(XTRA_DIST)
	rm -rf $(PKGDIR)
	mkdir -p $(PKGDIR)
	cp $(DISTFILES) $(XTRA_DIST) $(SRCS) *.h $(PKGDIR)
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

# implicit rule (is needed because of HDIRS!)
.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) $(HDIRS) -c -o $@ $<

# DO NOT DELETE THIS LINE 
include depend
