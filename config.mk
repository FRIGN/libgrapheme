# libgrapheme version
VERSION = 0

# Customize below to fit your system

# paths
PREFIX = /usr/local
INCPREFIX = $(PREFIX)/include
LIBPREFIX = $(PREFIX)/lib
MANPREFIX = $(PREFIX)/share/man

# flags
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS   = -std=c99 -Os -fPIC -Wall -Wextra -Wpedantic
LDFLAGS  = -s

# tools
CC = cc
AR = ar
RANLIB = ranlib
