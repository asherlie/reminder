CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -O3

all: test

test: test.c localnotify.c
ashnet_dir.o: ashnet_dir.c ashnet_dir.h

.PHONY:
clean:
	rm -f test
