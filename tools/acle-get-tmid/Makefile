CFLAGS = -Wall -g -I../

OBJS = acle-get-tmid.o

all: acle-get-tmid

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-get-tmid: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-get-tmid
