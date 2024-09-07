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