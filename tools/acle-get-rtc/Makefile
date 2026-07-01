CFLAGS = -Wall -g -I../

OBJS = acle-get-rtc.o

all: acle-get-rtc

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-get-rtc: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-get-rtc
