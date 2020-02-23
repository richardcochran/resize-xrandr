# Copyright (C) 2020 Richard Cochran <richardcochran@gmail.com>
# SPDX-License-Identifier: GPL-2.0+

DEBUG	=
CC	= $(CROSS_COMPILE)gcc
CFLAGS	= -Wall $(VER) $(DEBUG) $(EXTRA_CFLAGS)
LDLIBS	= -lXrandr -lX11 $(EXTRA_LDFLAGS)
PRG	= resize-xrandr
OBJ	= resize-xrandr.o
SRC	= $(OBJ:.o=.c)
DEPEND	= $(OBJ:.o=.d)
srcdir	:= $(dir $(lastword $(MAKEFILE_LIST)))
VPATH	= $(srcdir)

all: $(PRG)

resize-xrandr: $(OBJ)

clean:
	rm -f $(OBJ) $(DEPEND)

distclean: clean
	rm -f $(PRG)

# Implicit rule to generate a C source file's dependencies.
%.d: %.c
	@echo DEPEND $<; \
	rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

ifneq ($(MAKECMDGOALS), clean)
ifneq ($(MAKECMDGOALS), distclean)
-include $(DEPEND)
endif
endif

.PHONY: all clean distclean
