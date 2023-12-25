CC=gcc 
all: x86_build 
clean: x86_clean 

x86_build:
	$(CC) -o sig_catch sig_catch.c
	$(CC) -o sig_count sig_count.c
x86_clean:
	rm -rf sig_catch sig_count
