# ECEN 5713 Assignment 5 Makefile
# Builds the aesdsocket application with support for cross-compilation using CROSS-COMPILE
# Author: Sonal Tamrakar
# Credit: Adapted from Sam Siewert POSIX API Makefile 
# https://math.uaa.alaska.edu/~ssiewert/a335_code/EXAMPLES/POSIX/
# Date: 10/06/2024


CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -g -Wall -Werror
LDFLAGS ?= -pthread -lrt
TARGET ?= aesdsocket
CFILES ?= aesdsocket.c
OBJS = $(CFILES:.c=.o)

all: $(TARGET)

default: all

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

aesdsocket: $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -f *.o *.d
	rm -f aesdsocket