# dereferencier

CC=gcc

CFLAGS=-Wall -pedantic -ansi
#CFLAGS=-Wall -pedantic -std=c99
CFLAGS+=-D_SDEBUG
CPPFLAGS=
LDFLAGS=-ltdb

BIN=drfe
INCS=say.h
SRCS=main.c say.c
OBJS=$(SRCS:%.c=%.o)

#all: depend drfe

$(BIN): $(OBJS)
		$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c say.h

clean:
		rm -f *~ $(OBJS) $(BIN)

