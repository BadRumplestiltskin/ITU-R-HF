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
# Linked against the libraries it calls. The rpath finds them in the lib
# directory beside the bin directory "make install" puts the program in; in the
# build tree, tests/*.sh name the build directories on the library path
# instead. -z now resolves every call at start-up, so a missing or mismatched
# library stops the program before it does anything, not at the first call.
P533_DIR = ../../../P533/Linux
P372_DIR = ../../../P372/Linux
LIBS = $(P533_DIR)/libp533.so $(P372_DIR)/libp372.so
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -Wl,-rpath,@executable_path/../lib -L$(P533_DIR) -L$(P372_DIR) -lp533 -lp372 -lm
else
    LDFLAGS = -Wl,-rpath,'$$ORIGIN/../lib' -Wl,-z,now -L$(P533_DIR) -L$(P372_DIR) -lp533 -lp372 -lm
endif

RM = rm -f

prefix ?= /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

TARGET = CircuitCSV
SRCS = $(source_dir)CircuitCSV.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(OBJS:.o=.d) $(TARGET)

install: all
	install -d $(DESTDIR)$(bindir)/
	install -m 755 $(TARGET) $(DESTDIR)$(bindir)/$(TARGET)
