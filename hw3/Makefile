CC=gcc 
all: x86_build 
clean: x86_clean 

x86_build:
	$(CC) -o hw3 hw3.c -lpthread
	$(CC) -o my_test_client my_test_client.c -lpthread
x86_clean:
	rm -rf hw3 my_test_client
