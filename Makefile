ALL = gdbi gdbsig
CFLAGS = --std=gnu11 -g -Og -Wall -Werror

.PHONY: clean all

all: ${ALL}

gdbi: protocol.o
gdbi: LDFLAGS += -lreadline

gdbsig: protocol.o
gdbsig.o: gdb/signals.def gdb/signals.h

clean:
	rm -vf ${ALL} *.o
