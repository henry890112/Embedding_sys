#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8000
#define BUFSIZE 1024

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buf[BUFSIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    // 建立套接字
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    // 绑定套接字
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        close(sockfd);
        exit(1);
    }

    // 打开视频文件
    FILE *fp = fopen("movie.mp4", "rb");
    if (fp == NULL) {
        perror("Error opening file");
        close(sockfd);
        exit(1);
    }
    // check the whole bytes of the file
    fseek(fp, 0, SEEK_END);
    int file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("file size: %d\n", file_size);

    // 首先接收來自客戶端的請求
    if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len) < 0) {
        perror("Error receiving from client");
        close(sockfd);
        exit(1);
    }

    // 確定接收到客戶端的地址後，開始發送文件數據
    int n, packet_index = 0;
    while ((n = fread(buf, 1, BUFSIZE, fp)) > 0) {
        if (sendto(sockfd, buf, n, 0, (struct sockaddr *)&client_addr, client_addr_len) < 0) {
            perror("Error sending to client");
            break;
        }
        printf("Packet %d sent\n", packet_index++);
        usleep(10000); // 避免網絡擁堵
    }

    fclose(fp);
    close(sockfd);
    return 0;
}
