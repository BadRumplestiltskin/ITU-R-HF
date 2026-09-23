UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC = clang
else
    CC = gcc
endif

source_dir = ../CircuitCSV/
shared_dir = ../ITURHFProp/

OPTIMIZE ?= -O2
CFLAGS = -std=gnu11 -Wall -Wextra $(OPTIMIZE) -I$(source_dir) -I$(shared_dir) -I../../../Include
LDFLAGS = -lm -ldl

RM = rm -f
TARGET = CircuitCSV
SRCS = $(source_dir)CircuitCSV.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(TARGET)
