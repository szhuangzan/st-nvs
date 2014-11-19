# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation. Portions created by Netscape are
# Copyright (C) 1998-1999 Netscape Communications Corporation. All
# Rights Reserved.
#
# Portions created by SGI are Copyright (C) 2000 Silicon Graphics, Inc.
# All Rights Reserved.
#
# Contributor(s): Silicon Graphics, Inc.
#
# Alternatively, the contents of this file may be used under the terms
# of the ____ license (the  "[____] License"), in which case the provisions
# of [____] License are applicable instead of those above. If you wish to
# allow use of your version of this file only under the terms of the [____]
# License and not to allow others to use your version of this file under the
# NPL, indicate your decision by deleting  the provisions above and replace
# them with the notice and other provisions required by the [____] License.
# If you do not delete the provisions above, a recipient may use your version
# of this file under either the NPL or the [____] License.

##########################
# Supported OSes:
#
#OS         = AIX
#OS         = FREEBSD
#OS         = HPUX
#OS         = HPUX_64
#OS         = IRIX
#OS         = IRIX_64
#OS         = LINUX
#OS         = OSF1
#OS         = SOLARIS

##########################

CC          = cc
AR          = ar
LD          = ld
RANLIB      = ranlib

SHELL       = /bin/sh
ECHO        = /bin/echo

BUILD       = DBG
TARGETDIR   = $(OS)_$(shell uname -r)_$(BUILD)

DEFINES     = -D$(OS)
CFLAGS      =
ARFLAGS     = -rv
DSO_SUFFIX  = so


##########################
# Platform section.
# Possible targets:

TARGETS     = aix-debug aix-optimized           \
              freebsd-debug freebsd-optimized   \
              hpux-debug hpux-optimized         \
              hpux-64-debug hpux-64-optimized   \
              irix-n32-debug irix-n32-optimized \
              irix-64-debug irix-64-optimized   \
              linux-debug linux-optimized       \
              osf1-debug osf1-optimized         \
              solaris-debug solaris-optimized

#
# Platform specifics
#

ifeq ($(OS), AIX)
AIX_VERSION = $(shell uname -v).$(shell uname -r)
TARGETDIR   = $(OS)_$(AIX_VERSION)_$(BUILD)
CC          = xlC
STATIC_ONLY = yes
ifeq ($(BUILD), OPT)
OTHER_FLAGS = -w
endif
ifneq ($(filter-out 4.1 4.2, $(AIX_VERSION)),)
DEFINES     += -DMD_HAVE_SOCKLEN_T
endif
endif

ifeq ($(OS), FREEBSD)
CFLAGS      = -fPIC
LDFLAGS     = -shared
OTHER_FLAGS = -Wall
endif

ifeq (HPUX, $(findstring HPUX, $(OS)))
ifeq ($(OS), HPUX_64)
DEFINES     = -DHPUX
CFLAGS      = -Ae +DD64 +Z
else
CFLAGS      = -Ae +DAportable +Z
endif
RANLIB      = true
LDFLAGS     = -b
DSO_SUFFIX  = sl
endif

ifeq (IRIX, $(findstring IRIX, $(OS)))
ifeq ($(OS), IRIX_64)
DEFINES     = -DIRIX
ABIFLAG     = -64
else
ABIFLAG     = -n32
endif
RANLIB      = true
CFLAGS      = $(ABIFLAG) -mips3
LDFLAGS     = $(ABIFLAG) -shared
OTHER_FLAGS = -fullwarn
endif

ifeq ($(OS), LINUX)
LDFLAGS     = -shared
OTHER_FLAGS = -Wall
endif

ifeq ($(OS), OSF1)
RANLIB      = true
LDFLAGS     = -shared -all -expect_unresolved "*"
endif

ifeq ($(OS), SOLARIS)
TARGETDIR   = $(OS)_$(shell uname -r | sed 's/^5/2/')_$(BUILD)
CC          = gcc
LD          = gcc
RANLIB      = true
LDFLAGS     = -G
OTHER_FLAGS = -Wall
endif

#
# End of platform section.
##########################


ifeq ($(BUILD), OPT)
OTHER_FLAGS += -O
else
OTHER_FLAGS += -g
DEFINES     += -DDEBUG
endif

# Other possible defines:
# Some platforms allow to define FD_SETSIZE, e.g.:
# DEFINES += -DFD_SETSIZE=4096
# To use malloc(3) instead of mmap(2) for stack allocation:
# DEFINES += -DMALLOC_STACK

CFLAGS      += $(DEFINES) $(OTHER_FLAGS)

OBJS        = $(TARGETDIR)/sched.o \
              $(TARGETDIR)/stk.o   \
              $(TARGETDIR)/sync.o  \
              $(TARGETDIR)/key.o   \
              $(TARGETDIR)/io.o
HEADER      = $(TARGETDIR)/st.h
SLIBRARY    = $(TARGETDIR)/libst.a
DLIBRARY    = $(TARGETDIR)/libst.$(DSO_SUFFIX)
EXAMPLES    = examples

ifeq ($(STATIC_ONLY), yes)
LIBRARIES   = $(SLIBRARY)
else
LIBRARIES   = $(SLIBRARY) $(DLIBRARY)
endif

ifeq ($(OS),)
ST_ALL      = unknown
else
ST_ALL      = $(TARGETDIR) $(LIBRARIES) $(HEADER) $(EXAMPLES)
endif

all: $(ST_ALL)

unknown:
	@echo
	@echo "Please specify one of the following targets:"
	@echo
	@for target in $(TARGETS); do echo $$target; done
	@echo

$(TARGETDIR):
	if [ ! -d $(TARGETDIR) ]; then mkdir $(TARGETDIR); fi

$(SLIBRARY): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
	$(RANLIB) $@

$(DLIBRARY): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(HEADER): public.h
	rm -f $@
	cp public.h $@

$(TARGETDIR)/%.o: %.c common.h md.h
	$(CC) $(CFLAGS) -c $< -o $@

examples::
	@cd $@; $(MAKE) CC="$(CC)" CFLAGS="$(CFLAGS)" OS="$(OS)" TARGETDIR="$(TARGETDIR)"

clean:
	rm -rf *_OPT *_DBG

##########################
# Target rules:

aix-debug:
	$(MAKE) OS="AIX" BUILD="DBG"
aix-optimized:
	$(MAKE) OS="AIX" BUILD="OPT"

freebsd-debug:
	$(MAKE) OS="FREEBSD" BUILD="DBG"
freebsd-optimized:
	$(MAKE) OS="FREEBSD" BUILD="OPT"

hpux-debug:
	$(MAKE) OS="HPUX" BUILD="DBG"
hpux-optimized:
	$(MAKE) OS="HPUX" BUILD="OPT"
hpux-64-debug:
	$(MAKE) OS="HPUX_64" BUILD="DBG"
hpux-64-optimized:
	$(MAKE) OS="HPUX_64" BUILD="OPT"

irix-n32-debug:
	$(MAKE) OS="IRIX" BUILD="DBG"
irix-n32-optimized:
	$(MAKE) OS="IRIX" BUILD="OPT"
irix-64-debug:
	$(MAKE) OS="IRIX_64" BUILD="DBG"
irix-64-optimized:
	$(MAKE) OS="IRIX_64" BUILD="OPT"

linux-debug:
	$(MAKE) OS="LINUX" BUILD="DBG"
linux-optimized:
	$(MAKE) OS="LINUX" BUILD="OPT"

osf1-debug:
	$(MAKE) OS="OSF1" BUILD="DBG"
osf1-optimized:
	$(MAKE) OS="OSF1" BUILD="OPT"

solaris-debug:
	$(MAKE) OS="SOLARIS" BUILD="DBG"
solaris-optimized:
	$(MAKE) OS="SOLARIS" BUILD="OPT"

##########################

