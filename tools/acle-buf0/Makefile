CFLAGS = -Wall -g -I../

OBJS = acle-buf0.o

all: acle-buf0

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-buf0: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-buf0
