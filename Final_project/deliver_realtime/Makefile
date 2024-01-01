CC=gcc
CFLAGS=-I/usr/include

all:
	$(CC) -o realtime_server realtime_server.c -lpthread -lSDL2 -lv4l2
	$(CC) -o realtime_client realtime_client.c -lpthread -lSDL2 -lv4l2

clean:
	rm -rf realtime_server realtime_client 
