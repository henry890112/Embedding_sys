#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// FFmpeg 頭文件
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>

// SDL 頭文件
#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#define SERVER_IP "127.0.0.1" // 更改为服务器的IP地址
#define PORT 8000
#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    socklen_t server_addr_len = sizeof(server_addr);

    // 创建套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 打开文件以存储接收的数据
    FILE *output_fp = fopen("received_movie.mp4", "wb");
    if (output_fp == NULL) {
        perror("Error creating file");
        close(sockfd);
        exit(1);
    }

    // 向服务器发送请求以开始接收数据
    strcpy(buf, "Start");
    if (sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error sending to server");
        fclose(output_fp);
        close(sockfd);
        exit(1);
    }
    printf("Start receiving...\n");

    int packet_index = 0;
    // 循环接收数据
    while (1) {
        int bytes_received = recvfrom(sockfd, buf, BUFSIZE, 0, NULL, NULL);
        if (bytes_received <= 0) break; // 数据接收完成或错误

        fwrite(buf, 1, bytes_received, output_fp); // 将接收的数据写入文件
        printf("Received %d bytes\n", bytes_received);
        printf("Packet %d received\n", packet_index++);

        // 即時顯示影像收到的output_fp
    }

    printf("Over\n");
    fclose(output_fp);
    close(sockfd);
    return 0;
}
