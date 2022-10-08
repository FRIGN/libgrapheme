# Customize below to fit your system

# paths
PREFIX = /usr/local
INCPREFIX = $(PREFIX)/include
LIBPREFIX = $(PREFIX)/lib
MANPREFIX = $(PREFIX)/share/man

# flags
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS   = -std=c99 -Os -Wall -Wextra -Wpedantic
LDFLAGS  = -s

BUILD_CPPFLAGS = $(CPPFLAGS)
BUILD_CFLAGS   = $(CFLAGS)
BUILD_LDFLAGS  = $(LDFLAGS)

SHFLAGS   = -fPIC -ffreestanding

SOFLAGS   = -shared -nostdlib -Wl,--soname=libgrapheme.so.$(VERSION_MAJOR).$(VERSION_MINOR)
SONAME    = libgrapheme.so.$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)
SOSYMLINK = true

# -- OpenBSD -- (also unset LDCONFIG)
# SOFLAGS   = -shared -nostdlib
# SONAME    = libgrapheme.so.$(VERSION_MAJOR).$(VERSION_MINOR)
# SOSYMLINK = false

# -- macOS -- (also unset LDCONFIG)
# SOFLAGS   = -dynamiclib -install_name "libgrapheme.$(VERSION_MAJOR).dylib" \
#             -current_version "$(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_PATCH)" \
#             -compatibility_version "$(VERSION_MAJOR).$(VERSION_MINOR).0"
# SONAME    = libgrapheme.$(VERSION_MAJOR).dylib
# SOSYMLINK = false

# tools
CC       = cc
BUILD_CC = $(CC)
AR       = ar
RANLIB   = ranlib
LDCONFIG = ldconfig # unset to not call ldconfig(1) after install/uninstall
SH       = sh
