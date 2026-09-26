UNAME_S := $(shell uname -s)

# Set CC based on the operating system
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

source_dir = ../Src/P533/
bin_dir = ../Bin/
# Use the following for testing
#CFLAGS = -fPIC -Wall -Wextra -O0 -g -I$(source_dir)
# Use the following for production
OPTIMIZE ?= -O2
# P533_BUILD makes P533_API (P533.h) mark the exported functions, and
# -fvisibility=hidden keeps every other symbol inside the library.
CFLAGS = -std=c99 -fPIC -fvisibility=hidden -DP533_BUILD -Wall -Wextra -MMD -MP $(OPTIMIZE) -I$(source_dir) -I../../Include

RM = rm -f
TARGET_LIB = libp533.so

# libp533 calls into libp372, so it is linked against it. See P372/Linux/Makefile
# for the library's recorded name and -z defs. The rpath finds libp372 in the
# directory libp533 itself was loaded from, which is where "make install" puts
# both.
P372_DIR = ../../P372/Linux
P372_LIB = $(P372_DIR)/libp372.so
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -shared -Wl,-install_name,@rpath/$(TARGET_LIB) -Wl,-rpath,@loader_path \
              -L$(P372_DIR) -lp372 -lm
else
    LDFLAGS = -shared -Wl,-soname,$(TARGET_LIB) -Wl,-z,defs -Wl,-rpath,'$$ORIGIN' \
              -L$(P372_DIR) -lp372 -lm
endif

prefix ?= /usr/local
exec_prefix = $(prefix)
libdir = $(exec_prefix)/lib

SRCS = $(source_dir)Between7000kmand9000km.c \
       $(source_dir)ELayerScreeningFrequency.c \
       $(source_dir)Magfit.c \
	   $(source_dir)MedianSkywaveFieldStrengthShort.c \
	   $(source_dir)ReadIonParameters.c \
	   $(source_dir)CalculateCPParameters.c \
	   $(source_dir)Geometry.c \
	   $(source_dir)MUFBasic.c \
	   $(source_dir)P533.c \
	   $(source_dir)MUFOperational.c \
	   $(source_dir)ReadP1239.c \
	   $(source_dir)CircuitReliability.c \
	   $(source_dir)InitializePath.c \
	   $(source_dir)MedianAvailableReceiverPower.c \
	   $(source_dir)ReadType13.c \
	   $(source_dir)InputDump.c \
	   $(source_dir)MedianSkywaveFieldStrengthLong.c \
	   $(source_dir)MUFVariability.c \
	   $(source_dir)PathMemory.c \
	   $(source_dir)ValidatePath.c

OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS) $(P372_LIB)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

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
