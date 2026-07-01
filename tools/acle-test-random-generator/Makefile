CFLAGS = -Wall -g -I../

OBJS = acle-test-random-generator.o

all: acle-test-random-generator

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-test-random-generator: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-test-random-generator
