# ECEN 5713 Assignment 1/2 Makefile
# Builds the writer application with support for cross-compilation using CROSS-COMPILE
# Author: Sonal Tamrakar
# Credit: Adapted from Sam Siewert POSIX API Makefile 
# https://math.uaa.alaska.edu/~ssiewert/a335_code/EXAMPLES/POSIX/
# Date: 09/08/2024

INCLUDE_FLAGS= -Wall -Werror 
LIB_DIRS = 

CC = gcc



CDEFS=
CFLAGS= -O0 -g $(INCLUDE_FLAGS) $(CDEFS)
LIBS=

PRODUCT=writer

HFILES= 
CFILES=writer.c

SRCS= ${HFILES} ${CFILES}
OBJS= ${CFILES:.c=.o}

all:	${PRODUCT}

clean:
	-rm -f *.o *.d
	-rm -f writer

writer: ${OBJS}
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

depend:

.c.o:
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $<