CC=gcc
obj-m:= mydev.o
all:
	make -C /lib/modules/5.15.0-86-generic/build M=$(PWD) modules
	$(CC) -o writer writer.c
	$(CC) -o reader reader.c
clean:
	make -C /lib/modules/5.15.0-86-generic/build M=$(PWD) clean
	rm -rf writer reader