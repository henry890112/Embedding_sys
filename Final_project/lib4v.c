#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

int main() {
    int fd; // 文件描述符
    struct v4l2_format format;

    // 打開設備，這裡的 "/dev/video0" 可能需要根據你的系統進行調整
    fd = v4l2_open("/dev/video0", O_RDWR);
    if (fd < 0) {
        perror("無法打開設備");
        return -1;
    }

    // 設置影像格式
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = 640;
    format.fmt.pix.height = 480;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (v4l2_ioctl(fd, VIDIOC_S_FMT, &format) < 0) {
        perror("設置影像格式失敗");
        v4l2_close(fd);
        return -1;
    }

    // 這裡你可以添加額外的程式碼來進行影像捕捉...

    // 關閉設備
    v4l2_close(fd);
    return 0;
}

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <libv4l2.h>

int main() {
    // 初始化 GLFW
    if (!glfwInit()) {
        fprintf(stderr, "GLFW 初始化失敗\n");
        return -1;
    }

    // 創建一個窗口
    GLFWwindow* window = glfwCreateWindow(640, 480, "Webcam", NULL, NULL);
    if (!window) {
        fprintf(stderr, "GLFW 窗口創建失敗\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // 初始化 GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "GLEW 初始化失敗\n");
        return -1;
    }

    // 設置 OpenGL 狀態
    // ...[這裡可能包括設置視口大小、投影方式等]

    // 使用 libv4l2 打開攝像頭並設置格式
    int fd = open("/dev/video0", O_RDWR);
    if (fd == -1) {
        perror("無法打開攝像頭設備");
        return -1;
    }

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &format) == -1) {
        perror("無法獲取攝像頭格式");
        close(fd);
        return -1;
    }

    // 在這裡可以進一步配置攝像頭格式

    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("無法設置攝像頭格式");
        close(fd);
        return -1;
    }

    // 主循環
    while (!glfwWindowShouldClose(window)) {
        // 處理攝像頭影像捕捉和處理
        // ...[這裡包括使用 libv4l2 捕捉影像，並將其轉換為適合 OpenGL 使用的格式]

        // 使用 OpenGL 顯示影像
        // ...[這裡包括創建紋理、繪製四邊形並將影像映射到四邊形上]

        // 交換緩衝區並處理事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 清理和退出
    close(fd);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

