CFLAGS = -Wall -g -I../

OBJS = acle-logout.o

all: acle-logout

%.o: %.c
	${CC} -c ${CFLAGS} -o $@ $<

acle-logout: ${OBJS} Makefile
	${CC} -o $@ ${OBJS}

.PHONY: clean all
clean:
	rm -rf ${OBJS} acle-logout
