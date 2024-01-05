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

// #define SERVER_IP "192.168.50.11" // 更改为服务器的IP地址
#define SERVER_IP "127.0.0.1" // 更改为服务器的IP地址
#define PORT 9000
#define TCP_PORT 8080
#define BUFSIZE 38400
#define TCP_BUFSIZE 1024
#define WIDTH 320
#define HEIGHT 240  
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 290
#define BUFFER_COUNT 1
#define MAX_IMAGE_SIZE 153600
#define INIT_WINDOW_POS_X 100
#define INIT_WINDOW_POS_Y 100

// 勾選和叉叉按鈕的尺寸
const int button_width = 50;
const int button_height = 50;

// 設置勾選和叉叉按鈕的位置
SDL_Rect check_button_rect = { 60, HEIGHT, button_width, button_height }; // x, y, width, height
SDL_Rect cross_button_rect = { WIDTH - 60 - button_width, HEIGHT, button_width, button_height }; // x, y, width, height

// 繪製按鈕的函數
void draw_buttons(SDL_Renderer *renderer) {

    // 設置背景為黑色並清除
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // 黑色
    SDL_RenderClear(renderer);

    // 繪製勾選按鈕
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // 設置為綠色
    SDL_RenderFillRect(renderer, &check_button_rect);

    // 繪製叉叉按鈕
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // 設置為紅色
    SDL_RenderFillRect(renderer, &cross_button_rect);
}


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

    // 創建窗口
    *window = SDL_CreateWindow("Client Camera", INIT_WINDOW_POS_X, INIT_WINDOW_POS_Y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL);
    if (!*window) {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        SDL_Quit();
        exit(1);
    }

    // 創建渲染器
    *renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!*renderer) {
        fprintf(stderr, "SDL: could not create renderer - exiting\n");
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }

    // 設置渲染器的縮放品質
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    // 創建紋理，用於顯示影像，紋理的大小應該與影像一致
    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (!*texture) {
        fprintf(stderr, "SDL: could not create texture - exiting\n");
        SDL_DestroyRenderer(*renderer);
        SDL_DestroyWindow(*window);
        SDL_Quit();
        exit(1);
    }
}

void sendAlarmToServer() {

    // use TCP to send alarm message to TCP server
    int tcp_sockfd;
    struct sockaddr_in tcp_server_addr;
    char tcp_buf[TCP_BUFSIZE];

    // 建立套接字
    tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // 初始化服务器地址结构
    memset(&tcp_server_addr, 0, sizeof(tcp_server_addr));
    tcp_server_addr.sin_family = AF_INET;
    tcp_server_addr.sin_port = htons(TCP_PORT);
    tcp_server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 連接伺服器
    if (connect(tcp_sockfd, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) < 0) {
        perror("Error connecting to server");
        close(tcp_sockfd);
        exit(1);
    }

    // 向服务器发送请求以开始接收数据
    strcpy(tcp_buf, "10"); // front傳送10給server
    if (send(tcp_sockfd, tcp_buf, strlen(tcp_buf), 0) < 0) {
        perror("Error sending to server");
        close(tcp_sockfd);
        exit(1);
    }

    printf("Alarm message sent to server\n");
    close(tcp_sockfd);
}


int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buf[BUFSIZE];
    socklen_t server_addr_len = sizeof(server_addr);

    // 创建套接字
    create_client_socket(&sockfd, &server_addr);


    SDL_Rect image_rect;
    image_rect.x = (WINDOW_WIDTH - WIDTH) / 2; // 將影像水平居中
    image_rect.y = 0; // 將影像置於窗口頂部
    image_rect.w = WIDTH; // 影像的原始寬度
    image_rect.h = HEIGHT; // 影像的原始高度

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
    int alarm = 0;
    while (running) {
        int bytes_received = recvfrom(sockfd, buffer, BUFSIZE, 0, NULL, NULL);
        if (bytes_received < 0) {
            // 數據接收完成或錯誤，處理這些情況
            break;
        }
        // printf("Received %d bytes\n", bytes_received);  
        if(bytes_received == 0)  // check block
        {
            // printf("checker block\n");
            if (current_size == MAX_IMAGE_SIZE) {
                // 我們已經收到了一幅完整的影像
                SDL_UpdateTexture(texture, NULL, image_buffer, WIDTH * 2);
                SDL_RenderClear(renderer);
                draw_buttons(renderer); // 繪製勾選和叉叉按鈕
                SDL_RenderCopy(renderer, texture, NULL, &image_rect);
                
                SDL_RenderPresent(renderer);
                // 檢查退出事件
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = 0;
                    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                        int x, y;
                        SDL_GetMouseState(&x, &y);

                        // 檢查是否點擊勾選按鈕
                        if (x >= check_button_rect.x && x < check_button_rect.x + check_button_rect.w &&
                            y >= check_button_rect.y && y < check_button_rect.y + check_button_rect.h) {
                            // 處理勾選按鈕點擊事件
                            printf("Check button clicked 勾勾\n");
                            // 可以在這裡加入確認觀看影像的邏輯
                            running = 0;
                            alarm = 0; // 沒事囉
                        }

                        // 檢查是否點擊叉叉按鈕
                        if (x >= cross_button_rect.x && x < cross_button_rect.x + cross_button_rect.w &&
                            y >= cross_button_rect.y && y < cross_button_rect.y + cross_button_rect.h) {
                            // 處理叉叉按鈕點擊事件
                            printf("Cross button clicked 叉叉\n");
                            // 可以在這裡加入不觀看影像並退出的邏輯
                            running = 0; // 假設點擊叉叉就退出應用程式
                            alarm = 1; // 警報
                        }
                    }
                }
  
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
    }

    printf("Alarm: %d\n", alarm);

    if (alarm == 1)
    {
        printf("alarm\n");
        // 新建立一個tcp來傳資訊給tcp server
        sendAlarmToServer();

    }
    else if (alarm == 0)
    {
        printf("no alarm\n");
        // 重新執行detection
    }

    printf("Over\n");
    close(sockfd);
    return 0;
}