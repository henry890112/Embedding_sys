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

// #define SERVER_IP "192.168.50.125" // 更改为服务器的IP地址
#define SERVER_IP "127.0.0.1" // 更改为服务器的IP地址
#define PORT 8000
#define BUFSIZE 38400
#define WIDTH 320
#define HEIGHT 240  
#define BUFFER_COUNT 1
#define MAX_IMAGE_SIZE 153600

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

void init_SDL(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    // 创建窗口
    *window = SDL_CreateWindow("Client Camera", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
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
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    socklen_t server_addr_len = sizeof(server_addr);

    // 创建套接字
    create_client_socket(&sockfd, &server_addr);

    
    // init SDL 去接收影像
    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    // 初始化 SDL
    init_SDL(&window, &renderer, &texture);

    SDL_Event event;

    // 向服务器发送请求以开始接收数据
    strcpy(buf, "Start");
    if (sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error sending to server");
        close(sockfd);
        exit(1);
    }
    printf("Start receiving...\n");

    int running = 1;
    // 假設 BUFSIZE 是每個 UDP 數據包的最大大小
    // 假設每幅完整影像的最大大小是 MAX_IMAGE_SIZE
    unsigned char image_buffer[MAX_IMAGE_SIZE];
    char buffer[BUFSIZE];
    int current_size = 0;
    int checker = 0;
    while (running) {
        int bytes_received = recvfrom(sockfd, buffer, BUFSIZE, 0, NULL, NULL);
        if (bytes_received < 0) {
            // 數據接收完成或錯誤，處理這些情況
            break;
        }
        printf("Received %d bytes\n", bytes_received);  
        if(bytes_received == 0)  // check block
        {
            printf("checker block\n");
            if (current_size == MAX_IMAGE_SIZE) {
                // 我們已經收到了一幅完整的影像
                SDL_UpdateTexture(texture, NULL, image_buffer, WIDTH * 2);
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, NULL, NULL);
                SDL_RenderPresent(renderer);
  
            }
            // 重置計數器，為接收下一幅影像準備
            current_size = 0;
            // 清空緩衝區
            memset(image_buffer, 0, MAX_IMAGE_SIZE); 
        }else
        {  // solve沒接到0byte的checker block
            if(current_size == MAX_IMAGE_SIZE)
            {
                    // 重置計數器，為接收下一幅影像準備
                current_size = 0;
                // 清空緩衝區
                memset(image_buffer, 0, MAX_IMAGE_SIZE);
            }
            else
            {
            // 將buffer資訊存在image_buffer + current_size
            // printf("Save in the buffer\n");
                memcpy(image_buffer + current_size, buffer, bytes_received);
                current_size += bytes_received;
            }
        }
        // 檢查退出事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }
        printf("Current Image Size: %d\n", current_size);
        printf("--------------------\n");
    }

    printf("Over\n");
    close(sockfd);
    return 0;
}