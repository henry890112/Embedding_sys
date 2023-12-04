CC=gcc 
all: x86_build 
clean: x86_clean 

x86_build:
	$(CC) -o server server.c -lpthread
	$(CC) -o client client.c -lpthread
x86_clean:
	rm -rf server client