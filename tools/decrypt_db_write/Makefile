CFLAGS = -Wall -g -D_FILE_OFFSET_BITS=64

all: write-enc-db

write-enc-db: write-enc-db.o aes.o sha256.o password.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $*.c

install: write-enc-db
	install -o root -g root -m 755 write-enc-db /usr/bin

uninstall:
	rm -f /usr/bin/write-enc-db

clean:
	rm -f write-enc-db write-enc-db.o
