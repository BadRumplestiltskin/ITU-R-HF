UNAME_S := $(shell uname -s)

# Set CC based on the operating system
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

source_dir = ../Src/P372/
bin_dir = ../Bin/
# Use the following for testing
#CFLAGS = -fPIC -Wall -Wextra -O0 -g -I$(source_dir)
# Use the following for production
OPTIMIZE ?= -O2
# P372_BUILD makes P372_API (Noise.h) mark the exported functions, and
# -fvisibility=hidden keeps every other symbol inside the library.
CFLAGS = -fPIC -fvisibility=hidden -DP372_BUILD -Wall -Wextra -MMD -MP $(OPTIMIZE) -I$(source_dir) -I../../Include

RM = rm -f
TARGET_LIB = libp372.so

# The library records its own name, so whatever links against it looks it up
# by that name at run time: through the runner's rpath (@rpath on macOS) or the
# system search path. -z defs fails the link if the library leaves any symbol
# unresolved.
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -shared -Wl,-install_name,@rpath/$(TARGET_LIB) -lm
else
    LDFLAGS = -shared -Wl,-soname,$(TARGET_LIB) -Wl,-z,defs -lm
endif

prefix ?= /usr/local
exec_prefix = $(prefix)
libdir = $(exec_prefix)/lib

SRCS = $(source_dir)InitializeNoise.c \
       $(source_dir)MakeNoise.c \
       $(source_dir)Noise.c \
       $(source_dir)NoiseMemory.c

OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# -MMD -MP (in CFLAGS) writes a dependency file beside each object as it is
# compiled; -MP adds an empty rule per header, so a deleted or renamed header
# does not stop make with "No rule to make target".
-include $(OBJS:.o=.d)

.PHONY: clean

clean:
	${RM} ${OBJS} $(SRCS:.c=.d)
	${RM} ${TARGET_LIB}

install: all
	install -d $(DESTDIR)$(libdir)/
	install -m 755 $(TARGET_LIB) $(DESTDIR)$(libdir)/$(TARGET_LIB)
