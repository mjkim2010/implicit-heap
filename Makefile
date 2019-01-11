# A simple makefile for building a program composed of C source files.

bump.o: CFLAGS += -Og
implicit.o: CFLAGS += -O2
explicit.o: CFLAGS += -O2

ALLOCATORS = bump implicit explicit
PROGRAMS = $(ALLOCATORS:%=test_%)

all:: $(PROGRAMS)

CC = gcc
CFLAGS = -g3 -std=gnu99 -Wall $$warnflags
export warnflags = -Wfloat-equal -Wtype-limits -Wpointer-arith -Wlogical-op -Wshadow -Winit-self -fno-diagnostics-show-option
LDFLAGS =
LDLIBS =

$(PROGRAMS): test_%:%.o segment.c test_harness.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean::
	rm -f $(PROGRAMS) *.o callgrind.out.*

.PHONY: clean all

.INTERMEDIATE: $(ALLOCATORS:%=%.o)
