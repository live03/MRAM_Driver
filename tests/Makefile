CC ?= gcc
name = mram_api

all: mram_test

mram_test: mram_test.o dma_utils.o mram_api.o
	$(CC) -o $@ $^ -g -Wimplicit-function-declaration -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

%.o: %.c
	$(CC) -c -std=c99 -o $@ $< -g -w -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE
	
#动态库制作:默认源文件放在src文件夹
dll: lib$(name).so
	@echo make shared lib success
	cp -f lib$(name).so ../tests
lib$(name).so: mram_api.c dma_utils.c 
	$(CC) -fPIC -shared $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o *.bin *.log mram_test libmram_api.so
