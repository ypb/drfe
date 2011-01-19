# dereferencier

CC=gcc

CFLAGS=-Wall -pedantic -ansi
#CFLAGS=-Wall -pedantic -std=c99
CFLAGS+=-D_SDEBUG -g
CPPFLAGS=
LDFLAGS=-ltdb

BIN=drfe
INCS=say.h store.h
SRCS=main.c say.c store.c
OBJS=$(SRCS:%.c=%.o)

#all: depend drfe

$(BIN): $(OBJS)
		$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c say.h

say.o: say.c store.h

clean:
		rm -f *~ $(OBJS) $(BIN)

