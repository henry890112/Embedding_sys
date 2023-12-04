#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

int account_balance = 0;
pthread_mutex_t balance_mutex = PTHREAD_MUTEX_INITIALIZER;
#define MAX_CLIENTS 10

int server_socket;

void stop_parent(int signum) {
    signal(SIGINT, SIG_DFL);  // 將 SIGINT 處理程序設為默認行為
    close(server_socket);     // 關閉伺服器主套接字
    printf("Server closed\n");
    exit(signum);            // 結束程式
}

void *handle_client(void *arg) {
    int client_socket = *(int *)arg;
    char buffer[1024];

    // 接收客戶端的要求
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);

    // 解析客戶端的要求
    buffer[bytes_received] = '\0';
    char action[10];
    int amount, times;
    // sscanf接收的參數是指標，所以要加上&
    sscanf(buffer, "%s %d %d", action, &amount, &times);

    // 重複執行指定次數
    for (int i = 0; i < times; ++i) {
        pthread_mutex_lock(&balance_mutex);
        if (strcmp(action, "deposit") == 0) {
            account_balance += amount;
        } else if (strcmp(action, "withdraw") == 0) {
            account_balance -= amount;
        }
        pthread_mutex_unlock(&balance_mutex);

        // 輸出帳戶餘額
        printf("After %s: %d\n", action, account_balance);
    }

    // 關閉連線
    close(client_socket);
    free(arg); // 释放传递给线程的内存
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

    printf("Server listening on port %d\n", port);
    int count = 0;
    while (1)
    {

        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // 每次有新連線，啟動一個新的執行緒處理
        // client一直近來就會一直開新的thread
        // 为每个客户端创建一个线程
        pthread_t client_thread;
        int *client_socket_ptr = (int *)malloc(sizeof(int));
        *client_socket_ptr = client_socket;
        if (pthread_create(&client_thread, NULL, handle_client, (void *)client_socket_ptr) != 0) {
            perror("pthread_create");
            close(client_socket);
            free(client_socket_ptr);
        }
        // join 會等待thread結束才會繼續執行
        // pthread_join(client_thread, NULL);
        pthread_detach(client_thread);
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!thread created %d\n", ++count);
    }

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
