#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <SDL2/SDL.h>

#define DEVICE_PATH "/dev/video0"  // 攝像頭設備路徑
#define WIDTH 640
#define HEIGHT 480
#define BUFFER_COUNT 1

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("打開攝像頭失敗");
        return -1;
    }

    struct v4l2_format format = {0};
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = WIDTH;
    format.fmt.pix.height = HEIGHT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    format.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("設置攝像頭格式失敗");
        close(fd);
        return -1;
    }

    struct v4l2_requestbuffers req = {0};
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("請求緩衝區失敗");
        close(fd);
        return -1;
    }

    struct v4l2_buffer buffer = {0};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buffer) == -1) {
        perror("查詢緩衝區失敗");
        close(fd);
        return -1;
    }

    void *buffer_start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buffer.m.offset);
    if (buffer_start == MAP_FAILED) {
        perror("內存映射失敗");
        close(fd);
        return -1;
    }
    //我添加了對 VIDIOC_QBUF 的調用。這個調用將緩衝區放入隊列中，
    //使其準備好接收來自攝像頭的數據。這是 V4L2 程序中的一個標準步驟，
    //但在您最初的代碼中可能遺漏了。
    if (ioctl(fd, VIDIOC_QBUF, &buffer) == -1) {
        perror("將緩衝區放入隊列失敗");
        munmap(buffer_start, buffer.length);
        close(fd);
        return -1;
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("啟動影像流失敗");
        munmap(buffer_start, buffer.length);
        close(fd);
        return -1;
    }

    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "無法初始化 SDL: %s\n", SDL_GetError());
        munmap(buffer_start, buffer.length);
        close(fd);
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Webcam", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);

    SDL_Event event;
    int running = 1;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        if (ioctl(fd, VIDIOC_DQBUF, &buffer) == -1) {
            perror("無法從攝像頭讀取數據");
            break;
        }

        SDL_UpdateTexture(texture, NULL, buffer_start, WIDTH * 2);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        if (ioctl(fd, VIDIOC_QBUF, &buffer) == -1) {
            perror("無法將數據返回給攝像頭");
            break;
        }
    }

    if (ioctl(fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("停止影像流失敗");
    }

    munmap(buffer_start, buffer.length);
    close(fd);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
