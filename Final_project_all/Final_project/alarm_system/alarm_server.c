#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define PORT 7000

sig_atomic_t front_count = 0, back_count = 0;

int main() {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // 建立 socket 檔案描述符
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // 作業系統允許排隊等待接受的連線數
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // print 目前pid
        printf("PID: %d\n", getpid());
        // 接受新的连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // 读取数据
        valread = read(new_socket, buffer, 1024);
        printf("Data received: %s\n", buffer);

        // 根据数据更新计数器
        if (strcmp(buffer, "01") == 0) {
            back_count++;
        } else if (strcmp(buffer, "10") == 0) {
            front_count++;
        } else if (strcmp(buffer, "11") == 0) {
            front_count++;
            back_count++;
        }

        // 输出当前计数器状态
        printf("Front count: %d, Back count: %d\n", front_count, back_count);

        // 清空缓冲区
        memset(buffer, 0, sizeof(buffer));

        // 关闭新的 socket
        close(new_socket);
    }

    return 0;
}