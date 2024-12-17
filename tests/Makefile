CC ?= gcc
name = mram_api

all: mram_test

mram_test: mram_test.o
	$(CC) -o $@ $^ -L ./ -lmram_api -Wimplicit-function-declaration -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

%.o: %.c
	$(CC) -c -std=c99 -o $@ $< -Wimplicit-function-declaration -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

clean:
	rm -rf *.o *.bin mram_test
