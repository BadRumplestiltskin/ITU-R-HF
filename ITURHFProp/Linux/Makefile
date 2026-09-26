UNAME_S := $(shell uname -s)

# Set CC based on the operating system
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

# Use the following for testing
#CFLAGS = -fPIC -Wall -Wextra -O0 -g
# Use the following for production
OPTIMIZE ?= -O2
CFLAGS = -fPIC -Wall -Wextra -MMD -MP $(OPTIMIZE) -I../Src/ITURHFProp -I../../Include

# Linked against the libraries it calls. The rpath finds them in the lib
# directory beside the bin directory "make install" puts the program in; in the
# build tree, tests/*.sh name the build directories on the library path
# instead. -z now resolves every call at start-up, so a missing or mismatched
# library stops the program before it does anything, not at the first call.
P533_DIR = ../../P533/Linux
P372_DIR = ../../P372/Linux
LIBS = $(P533_DIR)/libp533.so $(P372_DIR)/libp372.so
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -Wl,-rpath,@executable_path/../lib -L$(P533_DIR) -L$(P372_DIR) -lp533 -lp372 -lm
else
    LDFLAGS = -Wl,-rpath,'$$ORIGIN/../lib' -Wl,-z,now -L$(P533_DIR) -L$(P372_DIR) -lp533 -lp372 -lm
endif

RM = rm -f
source_dir = ../Src/ITURHFProp/

prefix ?= /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin

TARGET = ITURHFProp

SRCS = $(source_dir)DumpPathData.c\
	$(source_dir)ITURHFProp.c\
	$(source_dir)ReadInputConfiguration.c\
	$(source_dir)Report.c\
	$(source_dir)ValidateITURHFP.c

OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET}

$(TARGET): $(OBJS) $(LIBS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

# -MMD -MP (in CFLAGS) writes a dependency file beside each object as it is
# compiled; -MP adds an empty rule per header, so a deleted or renamed header
# does not stop make with "No rule to make target".
-include $(OBJS:.o=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET} ${OBJS} $(SRCS:.c=.d)

install: all
	install -d $(DESTDIR)$(bindir)/
	install -m 755 $(TARGET) $(DESTDIR)$(bindir)/ITURHFProp
