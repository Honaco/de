CFLAGS = -Wall -g -I../

OBJS = acle-sd.o

all: acle-sd

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-sd: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-sd
