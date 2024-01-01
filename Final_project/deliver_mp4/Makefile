CC=gcc
CFLAGS=-I/usr/include

all:
	$(CC) -o mp4_server mp4_server.c -lpthread
	$(CC) -o mp4_client mp4_client.c -lpthread

clean:
	rm -rf mp4_server mp4_client
