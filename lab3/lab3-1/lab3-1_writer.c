#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <student ID>\n", argv[0]);
        return 1;
    }
    //Henry
    char *student_id = argv[1]; // 获取命令行参数作为字符串

    int fd;

    // 打开自定义驱动程序文件
    fd = open("/dev/etx_device", O_RDWR);

    if (fd < 0) {
        perror("Failed to open the driver");
        return -1;
    }

    // 将学号发送给驱动程序
    if (write(fd, student_id, strlen(student_id)) < 0) {
        perror("Failed to write student ID to driver");
        close(fd);
        return -1;
    }

    // 关闭驱动程序文件
    close(fd);

    return 0;
}
