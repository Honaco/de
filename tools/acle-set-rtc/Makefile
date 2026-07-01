CFLAGS = -Wall -g -I../

OBJS = acle-set-rtc.o

all: acle-set-rtc

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-set-rtc: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-set-rtc
