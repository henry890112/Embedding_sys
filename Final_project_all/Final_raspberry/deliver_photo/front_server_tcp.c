#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define PORT 12345
#define BUFFER_SIZE 1024

char buffer[BUFFER_SIZE];
int send_pic = 0;
int server_socketfd;


void* handleClient(void* arg) {
    int client_sockfd = *(int*)arg;
    free(arg);
    printf("!!!!!!!!\n");
    while(send_pic != 1){
        usleep(1000); // 小延迟以防CPU过载
    }

    FILE *file = fopen("./deliver_photo/front_camera.jpg", "rb");
    printf("file pointer is : %p\n", file);
    if (file == NULL) {
        perror("ERROR opening file");
        close(client_sockfd);
        pthread_exit(NULL);
    }

    int n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {

        if (write(client_sockfd, buffer, n) < 0) {
            perror("ERROR writing to socket");
            break; 
        }
        usleep(5000);
    }
    
    // 发送完毕，关闭文件和套接字
    fclose(file);
    close(client_sockfd);

    printf("傳送照片成功\n");
    pthread_exit(NULL);
}

void safe_close_routine(int signum) {
    signal(SIGINT, SIG_DFL);
    close(server_socketfd);

    printf("Server closed !!!!!\n");
    // 結束程式
    exit(signum);            
}

// 信号处理函数
void usr1_sig_handler(int signo) {
    send_pic = 1;
}

int main() {
    printf("Front PID: %d\n", getpid());
    
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    int client_sockfd;

    // signal
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = usr1_sig_handler;
    
    signal(SIGINT, safe_close_routine);

    sigaction(SIGUSR1, &sa, NULL);

    // 建立TCP套接字
    if ((server_socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // 可以使用相同的port
    int yes = 1;
    if (setsockopt(server_socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("Error setting socket option");
        exit(EXIT_FAILURE);
    }

    // 設置TCP套接字選項
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // 綁定套接字到伺服器地址
    if (bind(server_socketfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        perror("ERROR on binding");

    // 監聽套接字
    listen(server_socketfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        // 接受客戶端連接
        
        client_sockfd = accept(server_socketfd, (struct sockaddr *)&cli_addr, (socklen_t*)&clilen);
        if (client_sockfd < -0) {
            perror("Error accepting connection");
            continue;
        }

        int* client_socketfd_ptr = malloc(sizeof(int));
        if (client_socketfd_ptr == NULL) {
            perror("malloc failed");
            close(client_sockfd);
            continue;
        }

        *client_socketfd_ptr = client_sockfd;
        
        // printf("Client connected\n");

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handleClient, (void *)client_socketfd_ptr) != 0) {
            perror("Error creating client_thread");
            close(client_sockfd);
            free(client_socketfd_ptr);
        }
        usleep(1000000 * 0.5);


        //////////////
        pthread_detach(client_thread);
        //////////////
        // // 等待新的執行緒開始執行
        // if (pthread_join(client_thread, NULL) != 0) {
        //     perror("Error joining client_thread");
        // }
        // /////////////
    }
    return 0;
}
