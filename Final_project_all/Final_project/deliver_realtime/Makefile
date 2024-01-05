CC=gcc
CFLAGS=-I/usr/include

all:
	$(CC) -o back_realtime_client_udp back_realtime_client_udp.c -lpthread -lSDL2 -lv4l2
	$(CC) -o back_realtime_server_udp back_realtime_server_udp.c -lpthread -lSDL2 -lv4l2
	$(CC) -o front_realtime_client_udp front_realtime_client_udp.c -lpthread -lSDL2 -lv4l2
	$(CC) -o front_realtime_server_udp front_realtime_server_udp.c -lpthread -lSDL2 -lv4l2

clean:
	rm -rf back_realtime_client_udp back_realtime_server_udp front_realtime_client_udp front_realtime_server_udp
