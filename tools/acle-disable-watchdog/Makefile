CFLAGS = -Wall -g -I../

OBJS = acle-disable-watchdog.o

all: acle-disable-watchdog

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-disable-watchdog: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-disable-watchdog
