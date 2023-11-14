#include <errno.h> /* Errors */
#include <stdio.h> /* Input/Output */
#include <stdlib.h> /* General Utilities */
#include <sys/types.h> /* Primitive System Data Types */
#include <sys/wait.h> /* Wait for Process Termination */
#include <unistd.h> /* Symbolic Constants */ /*包含fork的函式庫 ,dup2(), exec家族*/
#include <fcntl.h>  // open()
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>


#define PORT 4444
#define BUFFER_SIZE 1024
int server_socket, client_socket;

void stop_child(int signum) {
    // waitpid 是為了等待並處理子進程的結束，以防止它們變成殭屍進程。
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void stop_parent(int signum) {
    signal(SIGINT, SIG_DFL);  // 將 SIGINT 處理程序設為默認行為
    close(server_socket);     // 關閉伺服器主套接字
    printf("Server closed\n");
    exit(signum);            // 結束程式
}


void handle_client(int client_socket) {

    if (fork() == 0) {
        // child process
        pid_t child_pid = getpid();
        printf("Train ID: %d\n", child_pid);

        if (dup2(client_socket, STDOUT_FILENO) == -1) {
            perror("Error in dup2");
            exit(EXIT_FAILURE);
        }

        close(client_socket);
        // int execl(const char *path, const char *arg, ... /*, (char *) NULL */);
        // path: 執行檔路徑, 若沒有給會從echo $PATH中找
        execlp("sl", "sl", "-l", NULL);
        perror("Error executing sl");
        // print a random number
        exit(EXIT_FAILURE);
    } else {
        // parent process
        // 為了釋放與特定客戶端通訊相關的資源
        close(client_socket);
    }
}


int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // 设置 SIGCHLD 信号的处理程序
    // 當parent還沒結束, child就先結束了, 會變成zombie process, 
    // 此時發出SIGCHLD會執行handler函數
    // 如果parent沒結束就不會執行handler函數
    signal(SIGCHLD, stop_child);
    signal(SIGINT, stop_parent);

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) == -1) {
        perror("Error listening");
        exit(EXIT_FAILURE);
    }

    // printf("Server listening on port %d...\n", PORT);

    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
            perror("Error accepting connection");
            continue;
        }

        // printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        handle_client(client_socket);
    }

    // // 執行不到下面close所以改用stop_parent
    // close(server_socket);
    // printf("Server closed\n");

    return 0;
}
