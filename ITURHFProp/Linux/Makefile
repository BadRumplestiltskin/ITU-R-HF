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

# Set linker flags based on the operating system
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = -lm -ldl
else
    LDFLAGS = -lm -ldl
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

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

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
