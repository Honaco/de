CFLAGS = -Wall -g -I../

OBJS = acle-set-sdc-mode.o

all: acle-set-sdc-mode

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-set-sdc-mode: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-set-sdc-mode
