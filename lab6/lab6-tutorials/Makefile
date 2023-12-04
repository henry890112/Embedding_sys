CC=gcc 
all: x86_build 
clean: x86_clean 

x86_build:
	$(CC) -o makesem makesem.c -lpthread
	$(CC) -o mutex mutex.c -lpthread
	$(CC) -o semaphore semaphore.c -lpthread
x86_clean:
	rm -rf makesem mutex semaphore