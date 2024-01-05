CC=gcc
CFLAGS=-I/usr/include

all:
	$(CC) -o back_server_tcp back_server_tcp.c -lSDL2 -lSDL2_image -pthread
	$(CC) -o front_server_tcp front_server_tcp.c -lSDL2 -lSDL2_image -pthread

clean:
	rm -rf back_server_tcp front_server_tcp 
