CC ?= gcc
CFLAGS ?= -O2 -Wall

all: zxheadless cpcheadless

zxheadless: zxheadless.c z80.c z80.h
	$(CC) $(CFLAGS) -o $@ zxheadless.c z80.c

cpcheadless: cpcheadless.c z80.c z80.h
	$(CC) $(CFLAGS) -o $@ cpcheadless.c z80.c

clean:
	rm -f zxheadless cpcheadless
