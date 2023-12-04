#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <errno.h>


int account_balance = 0;
int sem_id; // 不再需要全局變量
#define SEM_KEY 1234
#define MAX_CLIENTS 10
int server_socket;

void stop_parent(int signum) {
    signal(SIGINT, SIG_DFL);
    close(server_socket);
    semctl(sem_id, 0, IPC_RMID, 0); // 移除信號量
    printf("Server closed and removed semaphore.\n");
    exit(signum);
}

int P(int s) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if (semop(s, &sop, 1) < 0) {
        perror("P(): semop failed");
        exit(EXIT_FAILURE);
    } else {
        return 0;
    }
}

int V(int s) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if (semop(s, &sop, 1) < 0) {
        perror("V(): semop failed");
        exit(EXIT_FAILURE);
    } else {
        return 0;
    }
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
        P(sem_id);
        if (strcmp(action, "deposit") == 0) {
            account_balance += amount;
        } else if (strcmp(action, "withdraw") == 0) {
            account_balance -= amount;
        }
        V(sem_id);

        printf("After %s: %d\n", action, account_balance);
    }

    close(client_socket);
    free(arg);
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

    // 使用自己指定的key值創建信號量
    int key = SEM_KEY;
    if ((sem_id = semget(key, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    if (sem_id < 0) {
        if (errno == EEXIST) {
            // 信號量已經存在，只取得其 ID
            sem_id = semget(key, 1, 0666);
            printf("Semaphore already exists. Semaphore ID: %d\n", sem_id);
        }
    } else {
        // 初次創建信號量，將其初值設置為 1
        printf("Semaphore first created. Semaphore ID: %d\n", sem_id);
        if (semctl(sem_id, 0, SETVAL, 1) < 0)
        {
            fprintf(stderr, "Error setting initial value for the semaphore: %s\n", strerror(errno));
            exit(1);
        }
    }

    printf("Semaphore created successfully. Semaphore ID: %d\n", sem_id);



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
