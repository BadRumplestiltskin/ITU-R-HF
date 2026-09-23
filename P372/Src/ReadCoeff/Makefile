# ReadCoeff is a standalone utility for inspecting the CCIR coefficient files.
# It was in the tree but referenced by no build, so it compiled only when
# someone did it by hand and its warnings went unnoticed.

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

source_dir = ../ReadCoeff/

OPTIMIZE ?= -O2
CFLAGS = -std=gnu11 -Wall -Wextra $(OPTIMIZE) -I$(source_dir) -I../../../Include
LDFLAGS = -lm

RM = rm -f
TARGETS = ReadCoeffExample FAKP

.PHONY: all
all: $(TARGETS)

ReadCoeffExample: $(source_dir)ReadCoeffExample.c $(source_dir)ReadCoeff.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

FAKP: $(source_dir)FAKP.c $(source_dir)ReadCoeff.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(TARGETS)
