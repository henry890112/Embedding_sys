CC=gcc # or CC = /usr/bin/aarch64-linux-gnu-gcc
obj-m:= mydev.o
all: x86_build # or RPI_build
clean: x86_clean # or RPI_clean
x86_build:
	make -C /lib/modules/5.15.0-86-generic/build M=$(PWD) modules
	$(CC) -o writer writer.c
	$(CC) -o reader reader.c
x86_clean:
	make -C /lib/modules/5.15.0-86-generic/build M=$(PWD) clean
	rm -rf writer reader

RPI_build:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(shell pwd) modules
	$(CC) -o writer writer.c
	$(CC) -o reader reader.c
RPI_clean:
	make -ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(shell pwd) clean
	rm -rf writer reader