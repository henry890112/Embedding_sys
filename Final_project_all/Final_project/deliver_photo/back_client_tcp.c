// back_client_tcp.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "error.h"

#define PORT 6789
#define BUFFER_SIZE 1024
#define SERVER_IP "192.168.50.11" // 更改为服务器的IP地址
// #define SERVER_IP "127.0.0.1" // 更改为服务器的IP地址
#define WIDTH 320
#define HEIGHT 240
#define WINDOW_WIDTH 320
#define WINDOW_HEIGHT 290
#define INIT_WINDOW_POS_X 420
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

int main(int argc, char *argv[]) {

    int client_socket;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    FILE *file;

    // 创建套接字
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 配置服务器地址
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // 连接服务器
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Connection to server failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    // 打開檔案準備寫入
    file = fopen("./receive_back.jpg", "wb");
    if (file == NULL)
        perror("ERROR opening file");
    int bytes_received;
    // 從套接字讀取數據並寫入檔案
    while((bytes_received = read(client_socket, buffer, BUFFER_SIZE)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    // close(client_socket);

    printf("File sent successfully.\n");

    sleep(1);

    SDL_Rect image_rect;
    image_rect.x = (WINDOW_WIDTH - WIDTH) / 2; // 將影像水平居中
    image_rect.y = 0; // 將影像置於窗口頂部
    image_rect.w = WIDTH; // 影像的原始寬度
    image_rect.h = HEIGHT; // 影像的原始高度

    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        exit(1);
    }

    // 初始化 SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        SDL_Quit();
        exit(1);
    }

    // 創建窗口
    SDL_Window* window = SDL_CreateWindow("Server Image Display", INIT_WINDOW_POS_X, INIT_WINDOW_POS_Y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    // 創建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        IMG_Quit();
        SDL_Quit();
        exit(1);
    }

    // 加載 PNG 影像
    int alarm = 0;
    SDL_Surface* loadedSurface = IMG_Load("./receive_back.jpg");
    if (loadedSurface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", "您的PNG檔案路徑.png", IMG_GetError());
    } else {
        // 將 Surface 轉換為 Texture
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
        if (texture == NULL) {
            printf("Unable to create texture from %s! SDL Error: %s\n", "您的PNG檔案路徑.png", SDL_GetError());
        } else {
            // 渲染循環
            SDL_Event event;
            int running = 1;
            while (running) 
            {
                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        running = 0;
                    } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                        int x, y;
                        SDL_GetMouseState(&x, &y);

                        // 檢查是否點擊勾選按鈕, 代表要看即時影像
                        if (x >= check_button_rect.x && x < check_button_rect.x + check_button_rect.w &&
                            y >= check_button_rect.y && y < check_button_rect.y + check_button_rect.h) {
                            // 處理勾選按鈕點擊事件
                            printf("Check button clicked\n");
                            // 可以在這裡加入確認觀看影像的邏輯
                            running = 0; // 假設點擊叉叉就退出應用程式
                            alarm = 1;
                        }

                        // 檢查是否點擊叉叉按鈕
                        if (x >= cross_button_rect.x && x < cross_button_rect.x + cross_button_rect.w &&
                            y >= cross_button_rect.y && y < cross_button_rect.y + cross_button_rect.h) {
                            // 處理叉叉按鈕點擊事件
                            printf("Cross button clicked\n");
                            // 可以在這裡加入不觀看影像並退出的邏輯
                            running = 0; // 假設點擊叉叉就退出應用程式
                            alarm = 0;
                        }
                    }
                }

                SDL_RenderClear(renderer);
                draw_buttons(renderer); // 繪製勾選和叉叉按鈕
                SDL_RenderCopy(renderer, texture, NULL, &image_rect);
                SDL_RenderPresent(renderer);
            }

            SDL_DestroyTexture(texture);
        }

        SDL_FreeSurface(loadedSurface);
    }

    // 清理
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    if (alarm == 1)
    {
        system("../deliver_realtime/back_realtime_client_udp");

    }
    else if (alarm == 0)
    {
        // 利用上面所建立的client_socket tcp傳訊息給server
        char buffer[1024];
        char *message = "exit";
        strcpy(buffer, message);
        if (send(client_socket, buffer, strlen(buffer), 0) < 0) {
            perror("Send failed");
            exit(EXIT_FAILURE);
        }
        printf("沒有危險sent\n");
        close(client_socket);

    }

    return 0;
}