#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <string>\n", argv[0]);
        return 1;
    }

    char *input_string = argv[1];
    int string_length = strlen(input_string);
    printf("string_length = %d\n", string_length);

    int fd;

    // 打开自定义驱动程序文件
    fd = open("/dev/mydev", O_RDWR);

    if (fd < 0) {
        perror("Failed to open the driver");
        return -1;
    }

    // 一秒傳送下一個字母
    for (int i = 0; i < string_length; i++) {
        // 将当前字母发送给驱动程序
        if (write(fd, &input_string[i], 1) < 0) {
            perror("Failed to write letter to driver");
            close(fd);
            return -1;
        }

        printf("Sent letter: %c\n", input_string[i]);

        // 休眠1秒钟
        sleep(1);
    }

    // 关闭驱动程序文件
    close(fd);
    printf("sent letter complete.\n");

    return 0;
}
