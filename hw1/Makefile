obj-m += led_driver.o
obj-m += seven_seg_driver.o

KDIR = /home/henry/linux
CC = /usr/bin/aarch64-linux-gnu-gcc
# CC = gcc

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(shell pwd) modules
	$(CC) -o menu menu.c

clean:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KDIR) M=$(shell pwd) clean
	rm menu

menu:
	$(CC) -o menu menu.c

clean_menu:
	rm menu