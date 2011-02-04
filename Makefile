# dereferencier

CC=gcc

prefix=/usr/local
DESTDIR=

CFLAGS=-Wall -pedantic -ansi
#CFLAGS=-Wall -pedantic -std=c99
#CFLAGS+=-D_SDEBUG -g
CPPFLAGS=
LDFLAGS=-ltdb

BIN=drfe
INCS=say.h store.h data.h
SRCS=main.c say.c store.c data.c version.c
OBJS=$(SRCS:%.c=%.o)

SCRIPTS=say ask
SCRIPT=$(SCRIPTS:%=script/%)

#all: depend drfe

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

main.o: main.c say.h

say.o: say.c store.h version.h

data.o: data.c data.h

version.o: version.h

install: $(BIN)
	mkdir -p $(DESTDIR)$(prefix)/bin
	for f in $(BIN) $(SCRIPT); do \
	  install $$f $(DESTDIR)$(prefix)/bin ; \
	done

version.h: .GITSHA
	echo "#define GITSHA \""$(shell cat $<)"\""  > $@
	cat $@.in >> $@

.GITSHA: version.h.in
	git describe --long --always > $@

clean:
	rm -f *~ $(OBJS) $(BIN) version.h

