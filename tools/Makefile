CC ?= gcc
name = mram_api

#动态库制作:默认源文件放在src文件夹
dll: lib$(name).so
	@echo make shared lib success
	cp -f lib$(name).so ../tests
lib$(name).so: *.c
	$(CC) -fPIC -shared $^ -o $@ $(CFLAGS)

clean:
	rm -rf lib$(name).so
