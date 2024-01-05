CC=gcc
CFLAGS=-I/usr/include

all:
	$(CC) -o alarm_server alarm_server.c $(CFLAGS)
	$(CC) -o front_alarm_client front_alarm_client.c $(CFLAGS)
	$(CC) -o back_alarm_client back_alarm_client.c $(CFLAGS)

clean:
	rm -rf alarm_server front_alarm_client back_alarm_client