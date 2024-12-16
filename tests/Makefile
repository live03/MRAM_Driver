CC ?= gcc
name = mram_api

all: dll

mram_test: mram_test.o mram_api.o dma_utils.o
	$(CC) -o $@ $^ -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

%.o: %.c
	$(CC) -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

#动态库制作:默认源文件放在src文件夹
dll: lib$(name).so
	@echo make shared lib success
lib$(name).so: *.c
	$(CC) -fPIC -shared $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o *.bin mram_test
