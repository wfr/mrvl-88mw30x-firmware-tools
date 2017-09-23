CC=gcc
LIBS=-lelf
OPTS=-g -Wall $(LIBS)

MRVL_SRCS=src/crc32.c src/marvel-88mw30x-firmware.c
MRVL_HDRS=src/crc32.h src/marvel-88mw30x-firmware.h

all: bin/fwinfo bin/axf2firmware bin/firmware2elf

bin/fwinfo: src/fwinfo.c $(MRVL_SRCS) $(MRVL_HDRS)
	$(CC) $(OPTS) -o $@ src/fwinfo.c $(MRVL_SRCS)

bin/axf2firmware: src/axf2firmware.c $(MRVL_SRCS) $(MRVL_HDRS)
	$(CC) $(OPTS) -o $@ src/axf2firmware.c $(MRVL_SRCS)

bin/firmware2elf: src/firmware2elf.c $(MRVL_SRCS) $(MRVL_HDRS)
	$(CC) $(OPTS) -o $@ src/firmware2elf.c $(MRVL_SRCS)
