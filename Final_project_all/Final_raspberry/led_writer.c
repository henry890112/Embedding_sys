#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <semaphore.h>
#include <fcntl.h> 

#define PORT 7000

int front_count = 0, back_count = 0, front_total_count = 0, back_total_count = 0;

int main() {
    sem_t sem;
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

    if (sem_init(&sem, 0, 1) != 0) {
        perror("sem_init failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // 接受新的连接
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // 读取数据
        valread = read(new_socket, buffer, 1024);
        printf("Data received: %s\n", buffer);

        sem_wait(&sem);
        // 根据数据更新计数器
        if (strcmp(buffer, "01") == 0) {
            back_count++;
        } else if (strcmp(buffer, "10") == 0) {
            front_count++;
        } 

        if ((strcmp(buffer, "01") == 0) || (strcmp(buffer, "000") == 0))
        {
            back_total_count++;
        }
        if ((strcmp(buffer, "10") == 0) || (strcmp(buffer, "111") == 0))
        {
            front_total_count++;
        }
        sem_post(&sem);



        printf("Front count: %d, Back count: %d\n", front_count, back_count);

        int fd = open("/dev/etx_device", O_WRONLY);
        if (fd < 0) {
            perror("Failed to open the device");
            return 1;
        }

        if (front_count == 2) {
            printf("write\n");
            write(fd, "10", 2);  // 如果 front_count 是 3
        } else if (back_count == 2) {
            write(fd, "01", 2);  // 如果 back_count 是 3
        } 

        if(back_total_count==2){
            back_count = 0;
            back_total_count = 0;
        }
        if(front_total_count==2){
            front_count = 0;
            front_total_count = 0;
        }


        close(fd);

        memset(buffer, 0, sizeof(buffer));

        // 关闭新的 socket
        close(new_socket);
    }
    sem_destroy(&sem);
    return 0;
}
