CC ?= gcc

all: reg_rw dma_to_device dma_from_device performance test_chrdev dll

dma_to_device: dma_to_device.o
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

dma_from_device: dma_from_device.o
	$(CC) -lrt -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

performance: performance.o
	$(CC) -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

reg_rw: reg_rw.o
	$(CC) -o $@ $<

test_chrdev: test_chrdev.o
	$(CC) -o $@ $<

%.o: %.c
	$(CC) -c -std=c99 -o $@ $< -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -D_LARGE_FILE_SOURCE

name = mram_api

#动态库制作:默认源文件放在src文件夹
dll: lib$(name).so
	@echo make shared lib success
	cp -f lib$(name).so ../tests
lib$(name).so: mram_api.c dma_utils.c 
	$(CC) -fPIC -shared $^ -o $@ $(CFLAGS)

clean:
	rm -rf *.o *.bin reg_rw dma_to_device dma_from_device performance test_chrdev lib$(name).so
