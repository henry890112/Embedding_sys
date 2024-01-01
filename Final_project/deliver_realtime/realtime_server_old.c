#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <SDL2/SDL.h>
#include <fcntl.h>

#define DEVICE_PATH "/dev/video2"  // 攝像頭設備路徑
#define WIDTH 640
#define HEIGHT 480
#define BUFFER_COUNT 1
#define PORT 8000
#define BUFSIZE 61440
#define MAX_CLIENTS 5

void create_server_socket(int *sockfd, struct sockaddr_in *server_addr) {
    // 建立套接字
    *sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // 初始化服务器地址结构
    memset(server_addr, 0, sizeof(*server_addr));
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr->sin_port = htons(PORT);

    // 绑定套接字
    if (bind(*sockfd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
        perror("Error binding socket");
        close(*sockfd);
        exit(1);
    }

    if (listen(*sockfd, MAX_CLIENTS) < 0) {
        perror("Error listening on socket");
        close(*sockfd);
        exit(1);
    }
}

void init_camera(int *fd, struct v4l2_format *format, struct v4l2_requestbuffers *req, struct v4l2_buffer *buffer, void **buffer_start) {

    // 设置摄像头格式
    format->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format->fmt.pix.width = WIDTH;
    format->fmt.pix.height = HEIGHT;
    format->fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    format->fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(*fd, VIDIOC_S_FMT, format) == -1) {
        perror("设置摄像头格式失败");
        close(*fd);
        exit(1);
    }

    // 请求缓冲区
    req->count = BUFFER_COUNT;
    req->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req->memory = V4L2_MEMORY_MMAP;

    if (ioctl(*fd, VIDIOC_REQBUFS, req) == -1) {
        perror("请求缓冲区失败");
        close(*fd);
        exit(1);
    }

    // 查询缓冲区
    buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer->memory = V4L2_MEMORY_MMAP;
    buffer->index = 0;

    if (ioctl(*fd, VIDIOC_QUERYBUF, buffer) == -1) {
        perror("查询缓冲区失败");
        close(*fd);
        exit(1);
    }

    // 内存映射
    *buffer_start = mmap(NULL, buffer->length, PROT_READ | PROT_WRITE, MAP_SHARED, *fd, buffer->m.offset);
    if (*buffer_start == MAP_FAILED) {
        perror("内存映射失败");
        close(*fd);
        exit(1);
    }

    // 将缓冲区放入队列
    if (ioctl(*fd, VIDIOC_QBUF, buffer) == -1) {
        perror("将缓冲区放入队列失败");
        munmap(*buffer_start, buffer->length);
        close(*fd);
        exit(1);
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(*fd, VIDIOC_STREAMON, &type) == -1) {
        perror("开启摄像头失败");
        munmap(*buffer_start, buffer->length);
        close(*fd);
        exit(1);
    }
}

void init_SDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // 创建窗口
    *window = SDL_CreateWindow("Server Camera", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
    if (!*window) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }

    // 创建渲染器
    *renderer = SDL_CreateRenderer(*window, -1, 0);
    if (!*renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        exit(1);
    }

    // 创建纹理
    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!*texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buf[BUFSIZE];
    socklen_t client_addr_len = sizeof(client_addr);

    // 建立socket
    create_server_socket(&sockfd, &server_addr);

    // 初始化摄像头
    int fd;
    struct v4l2_format format;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buffer;
    void *buffer_start;
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("打开摄像头失败");
        exit(1);
    }
    init_camera(&fd, &format, &req, &buffer, &buffer_start);
    printf("Waiting for client...\n");
    // 首先接收來自客戶端的請求, 請求之後才開始傳送影像
    if (recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len) < 0) {
        perror("Error receiving from client");
        close(sockfd);
        exit(1);
    }

    // 初始化 SDL
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    init_SDL(&window, &renderer, &texture);

    SDL_Event event;
    int running = 1;

    // 確定接收到客戶端的地址後，開始發送文件數據
    int n, packet_index = 0;
    char checker_buf[BUFSIZE];

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
        // 印出完整影像的大小
        printf("buffer length: %d\n", buffer.length);
        
        char *data_ptr = (char *)buffer_start;
        int remaining = buffer.length;

        while (remaining > 0) {
            // 此while會發送完所有的數據, 但clinet端不一定會收到所有的數據
            ssize_t sent_bytes = sendto(sockfd, data_ptr, BUFSIZE, 0, (struct sockaddr *)&client_addr, client_addr_len);
            if (sent_bytes == -1) {
                perror("無法發送數據");
                break;
            }
            data_ptr += sent_bytes;
            remaining -= sent_bytes;
            usleep(500);
        }

        // 當remaining == 0時, 代表一幅完整影像已經發送完畢, 傳送一個空的buffer給client端
        if (remaining == 0) {
            printf("Send empty buffer to client\n");
            if (sendto(sockfd, NULL, 0, 0, (struct sockaddr *)&client_addr, client_addr_len) < 0)
            {
                perror("Error sending to client");
                close(sockfd);
                exit(1);
            }
        }       

        SDL_UpdateTexture(texture, NULL, buffer_start, WIDTH * 2);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        if (ioctl(fd, VIDIOC_QBUF, &buffer) == -1) {
            perror("無法將數據返回給攝像頭");
            break;
        }
        // sleep(1);
    }

    fclose(F_DUPFD);
    close(sockfd);
    return 0;
}
