# libgrapheme version
VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_PATCH = 0
UNICODE_VERSION = 15.0.0
MAN_DATE = 2022-09-07

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

SHFLAGS  = -fPIC -ffreestanding
SOFLAGS  = -shared -nostdlib -Wl,--soname=libgrapheme.so.$(VERSION_MAJOR).$(VERSION_MINOR)

# tools
CC       = cc
BUILD_CC = $(CC)
AR       = ar
RANLIB   = ranlib
LDCONFIG = ldconfig # unset to not call ldconfig(1) after install/uninstall
SH       = sh
