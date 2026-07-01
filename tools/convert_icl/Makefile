TARGETS = convert_icl convert_hd_serial

CFLAGS += -Wall -fPIC
CFLAGS += -I./include -I./include/ic-libs -I./include/ic-libs/fs
LDFLAGS += -lz #-ludev -llvm2app
LDFLAGS_HD += -lz

LIBS += lib/libfs.a lib/libfs_ocl.a lib/libmisc.a lib/libcrypto_gost.a lib/liblog.a

all: ${TARGETS}

convert_icl: convert_icl.o ${LIBS} Makefile
	${CC} ${LDFLAGS} convert_icl.o ${LIBS} -o $@

convert_hd_serial: convert_hd_serial.o ${LIBS} Makefile
	${CC} ${LDFLAGS_HD} convert_hd_serial.o ${LIBS} -o $@

%.o: %.c ${HEADERS} Makefile
	${CC} -c ${CFLAGS} $< -o $@

clean:
	rm -rf *.o ${TARGETS}

.PHONY: all clean
