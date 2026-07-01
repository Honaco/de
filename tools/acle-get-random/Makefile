CFLAGS = -Wall -g -I../

OBJS = acle-get-random.o

all: acle-get-random

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-get-random: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-get-random
