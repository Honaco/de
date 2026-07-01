CFLAGS = -Wall -g -I../

OBJS = acle-get-state.o

all: acle-get-state

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-get-state: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-get-state
