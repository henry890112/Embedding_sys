#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

int account_balance = 0;
sem_t balance_semaphore;
#define MAX_CLIENTS 10

int server_socket;

void stop_parent(int signum) {
    signal(SIGINT, SIG_DFL);
    close(server_socket);
    printf("Server closed\n");
    exit(signum);
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[256];

    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0';

    char action[10];
    int amount, times;
    sscanf(buffer, "%s %d %d", action, &amount, &times);

    for (int i = 0; i < times; ++i) {
        sem_wait(&balance_semaphore);
        if (strcmp(action, "deposit") == 0) {
            account_balance += amount;
        } else if (strcmp(action, "withdraw") == 0) {
            account_balance -= amount;
        }
        sem_post(&balance_semaphore);

        printf("After %s: %d\n", action, account_balance);
    }

    close(client_socket);
    free(arg);
    pthread_exit(NULL);
}

int start_server(int port) {
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    signal(SIGINT, stop_parent);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Socket binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Socket listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    sem_init(&balance_semaphore, 0, 1);  // 初始化 semaphore
    // print the sem id
    printf("sem id: %ld\n", balance_semaphore.__align);

    printf("Server listening on port %d\n", port);
    int count = 0;
    while (1) {
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        // printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_t client_thread;
        int *client_socket_ptr = (int *)malloc(sizeof(int));
        *client_socket_ptr = client_socket;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)client_socket_ptr) != 0) {
            perror("pthread_create");
            close(client_socket);
            free(client_socket_ptr);
        }

        pthread_detach(client_thread);
        // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!thread created %d\n", ++count);
    }

    sem_destroy(&balance_semaphore);  // 銷毀 semaphore
    close(server_socket);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    start_server(port);

    return 0;
}
