#
# This file is part of 'pdp', a PDP-11 simulator.
#
# For information contact:
#
#   Computer Science House
#   Attn: Eric Edwards
#   Box 861
#   25 Andrews Memorial Drive
#   Rochester, NY 14623
#
# Email:  mag@potter.csh.rit.edu
# FTP:    ftp.csh.rit.edu:/pub/csh/mag/pdp.tar.Z
# 
# Copyright 1994, Eric A. Edwards
#
# Permission to use, copy, modify, and distribute this
# software and its documentation for any purpose and without
# fee is hereby granted, provided that the above copyright
# notice appear in all copies.  Eric A. Edwards makes no
# representations about the suitability of this software
# for any purpose.  It is provided "as is" without expressed
# or implied warranty.
#
# Makefile
#
#

#
# Change as needed
#

CC = /usr/ucb/cc
CFLAGS = -O

#
# Targets
#

TARGET = pdp
UTILS = mkrl

#
# Source and Object Files
#

SRCS =	access.c boot.c branch.c double.c ea.c itab.c \
	kl.c kw.c main.c lp.c pdp.c rl.c rtc.c service.c \
	single.c tm.c ui.c wd.c weird.c
OBJS =	access.o boot.o branch.o double.o ea.o itab.o \
	kl.o kw.o main.o lp.o pdp.o rl.o rtc.o service.o \
	single.o tm.o ui.o wd.o weird.o
INCS =	defines.h
USRCS = mkrl.c

#
# Build Rules
#

everything:	$(TARGET) $(UTILS)
	touch everything

$(TARGET):	$(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

mkrl:
	$(CC) $(CFLAGS) -o mkrl mkrl.c

#
# Cool Utilities
#

clean:
	rm -f $(TARGET) $(OBJS) $(UTILS) everything

lint:
	lint $(SRCS)

count:
	wc -l $(SRCS) $(USRCS)
