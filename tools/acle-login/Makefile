CFLAGS = -Wall -g -I../

OBJS = acle-login.o any-xid.o

all: acle-login

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-login: ${OBJS} Makefile any-xid.h
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-login
