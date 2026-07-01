CFLAGS = -Wall -g -I../

OBJS = acle-set-rng.o

all: acle-set-rng

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-set-rng: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-set-rng
