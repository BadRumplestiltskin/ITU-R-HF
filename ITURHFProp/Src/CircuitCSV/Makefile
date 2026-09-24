UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

source_dir = ../CircuitCSV/
shared_dir = ../ITURHFProp/

OPTIMIZE ?= -O2
# -MMD -MP writes a dependency file beside each object, included below, so a
# change to a header in Include/ rebuilds CircuitCSV.
CFLAGS = -std=gnu11 -Wall -Wextra -MMD -MP $(OPTIMIZE) -I$(source_dir) -I$(shared_dir) -I../../../Include
LDFLAGS = -lm -ldl

RM = rm -f

prefix ?= /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

TARGET = CircuitCSV
SRCS = $(source_dir)CircuitCSV.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGET)

install: all
	install -d $(DESTDIR)$(bindir)/
	install -m 755 $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)
