CFLAGS = -Wall -g -I../

OBJS = acle-write-sectors.o

all: acle-write-sectors

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-write-sectors: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-write-sectors
