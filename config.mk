# libgrapheme version
VERSION = 1

# Customize below to fit your system

# paths
PREFIX = /usr/local
INCPREFIX = $(PREFIX)/include
LIBPREFIX = $(PREFIX)/lib
MANPREFIX = $(PREFIX)/share/man

# flags
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS   = -std=c99 -Os -fPIC -Wall -Wextra -Wpedantic
LDFLAGS  = -Wl,--soname=libgrapheme.so

BUILD_CPPFLAGS = $(CPPFLAGS)
BUILD_CFLAGS   = $(CFLAGS)
BUILD_LDFLAGS  = -s

# tools
CC       = cc
BUILD_CC = $(CC)
AR       = ar
RANLIB   = ranlib
LDCONFIG = ldconfig # unset to not call ldconfig(1) after install/uninstall
