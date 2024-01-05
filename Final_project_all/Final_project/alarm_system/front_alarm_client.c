#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// // FFmpeg 頭文件
// #include <libavcodec/avcodec.h>
// #include <libavutil/imgutils.h>
// #include <libavutil/opt.h>
// #include <libavformat/avformat.h>

// // SDL 頭文件
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_thread.h>

#define SERVER_IP "192.168.50.11" // 更改为服务器的IP地址
// #define SERVER_IP "127.0.0.1" // 更改为服务器的IP地址
#define PORT 9000
#define TCP_PORT 7000
#define BUFSIZE 38400
#define TCP_BUFSIZE 1024
#define WIDTH 320
#define HEIGHT 240  
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 290
#define BUFFER_COUNT 1
#define MAX_IMAGE_SIZE 153600
#define INIT_WINDOW_POS_X 100
#define INIT_WINDOW_POS_Y 100



void create_client_socket(int *sockfd, struct sockaddr_in *server_addr) {
    // 建立套接字
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // 初始化服务器地址结构
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    server_addr->sin_addr.s_addr = inet_addr(SERVER_IP);
}



void sendAlarmToServer() {

    // use TCP to send alarm message to TCP server
    int tcp_sockfd;
    struct sockaddr_in tcp_server_addr;
    char tcp_buf[TCP_BUFSIZE];

    // 建立套接字
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // 初始化服务器地址结构
    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_port = htons(TCP_PORT);
    tcp_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 連接伺服器
    if (connect(tcp_sockfd, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) < 0) {
        perror("Error connecting to server");
        close(tcp_sockfd);
        exit(1);
    }

    // 向服务器发送请求以开始接收数据
    strcpy(tcp_buf, "10"); // front傳送10給server
    if (send(tcp_sockfd, tcp_buf, strlen(tcp_buf), 0) < 0) {
        perror("Error sending to server");
        close(tcp_sockfd);
        exit(1);
    }

    printf("Alarm message sent to server\n");
    close(tcp_sockfd);
}


int main(int argc, char *argv[]) {

    int alarm = 1;
    if (alarm == 1)
    {
        printf("alarm\n");
        // 新建立一個tcp來傳資訊給tcp server
        sendAlarmToServer();

    }
    else if (alarm == 0)
    {
        printf("no alarm\n");
        // 重新執行detection
    }

    printf("Over\n");
    // close(sockfd);
    return 0;
}