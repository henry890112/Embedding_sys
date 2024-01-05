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
#include <pthread.h>
#include <signal.h>

#define DEVICE_PATH "/dev/video2"  // 攝像頭設備路徑
#define WIDTH 320
#define HEIGHT 240
#define BUFFER_COUNT 1
#define PORT 9000
#define BUFSIZE 38400
#define MAX_IMAGE_SIZE 153600
#define INIT_WINDOW_POS_X 100
#define INIT_WINDOW_POS_Y 100


// Global variables
// 初始化摄像头
int fd;
struct v4l2_format format;
struct v4l2_requestbuffers req;
struct v4l2_buffer buffer;
void *buffer_start;
int sockfd;
struct sockaddr_in server_addr;
struct sockaddr_in server_addr, client_addr;
socklen_t client_addr_len = sizeof(client_addr);

void stop_parent(int signum) {
    signal(SIGINT, SIG_DFL);
    close(sockfd);
    printf("Server stopped!!!\n");
    exit(signum);
}

struct camera_data {
    char global_camera_buffer[MAX_IMAGE_SIZE];
    int global_camera_data_size;
    pthread_mutex_t camera_mutex;
};

struct camera_data cam_data;

void init_camera_data()
{
    // 然後初始化
    memset(cam_data.global_camera_buffer, 0, MAX_IMAGE_SIZE);
    cam_data.global_camera_data_size = 0;
    pthread_mutex_init(&cam_data.camera_mutex, NULL);
}

struct client_data {
    int sockfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
};


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
    *window = SDL_CreateWindow("Front Server Camera", INIT_WINDOW_POS_X, INIT_WINDOW_POS_Y, WIDTH, HEIGHT, SDL_WINDOW_OPENGL);
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

void *camera_thread(void *arg) {

     // 初始化 SDL
    int running = 1;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    init_SDL(&window, &renderer, &texture);
    SDL_Event event;
    

    // SDL_Event event;
    while (running) {
        pthread_mutex_lock(&cam_data.camera_mutex);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
                break;
            }
        }
        if (ioctl(fd, VIDIOC_DQBUF, &buffer) == -1) {
            perror("無法從攝像頭讀取數據");
            pthread_mutex_unlock(&cam_data.camera_mutex);
            break;
        }
        // 印出完整影像的大小
        // printf("buffer length: %d\n", buffer.length);
        // 將從攝像頭讀取的數據復制到全局緩衝區
        memcpy(cam_data.global_camera_buffer, buffer_start, buffer.length);
        cam_data.global_camera_data_size = buffer.length;
        
        SDL_UpdateTexture(texture, NULL, buffer_start, WIDTH * 2);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        if (ioctl(fd, VIDIOC_QBUF, &buffer) == -1) {
            perror("無法將數據返回給攝像頭");
            pthread_mutex_unlock(&cam_data.camera_mutex);
            break;
        }
        pthread_mutex_unlock(&cam_data.camera_mutex);
    }
    return NULL;
}

void *handle_client(void *arg)
{
    struct client_data *data = (struct client_data *)arg;
    int client_sockfd = data->sockfd;
    struct sockaddr_in client_addr = data->client_addr;
    socklen_t client_addr_len = data->client_addr_len;

    free(arg);
    printf("Thread ID: %ld\n", pthread_self());

    

    // print the thread id
    while(1)
    {
        // 使用在 camera_thread 中讀取的數據
        // pthread_mutex_lock(&cam_data.camera_mutex);
        char *data_ptr = cam_data.global_camera_buffer;
        int remaining = cam_data.global_camera_data_size;
        // pthread_mutex_unlock(&cam_data.camera_mutex);

        // 印出完整影像的大小
        while (remaining > 0) {
            // 此while會發送完所有的數據, 但client端不一定會收到所有的數據
            // printf("remaining: %d\n", remaining);
            ssize_t sent_bytes = sendto(client_sockfd, data_ptr, BUFSIZE, 0, (struct sockaddr *)&client_addr, client_addr_len);
            if (sent_bytes == -1) {
                perror("無法發送數據");
                break;
            }
            data_ptr += sent_bytes;
            remaining -= sent_bytes;
            usleep(10000);
        }

        // 當remaining == 0時, 代表一幅完整影像已經發送完畢, 傳送一個空的buffer給client端
        if (remaining == 0) {
            // printf("remaining: %d\n", remaining);
            // printf("Send empty buffer to client\n");
            if (sendto(client_sockfd, NULL, 0, 0, (struct sockaddr *)&client_addr, client_addr_len) < 0)
            {
                perror("Error sending to client");
                close(client_sockfd);
                exit(1);
            }
        }
    }
    free(data);
}

int main(int argc, char *argv[]) {
    
    char buf[BUFSIZE];
    signal(SIGINT, stop_parent);

    // 建立socket
    create_server_socket(&sockfd, &server_addr);

    // 初始化摄像头
    fd = open(DEVICE_PATH, O_RDWR);
    if (fd == -1) {
        perror("打开摄像头失败");
        exit(1);
    }
    init_camera(&fd, &format, &req, &buffer, &buffer_start);
    init_camera_data();

    pthread_t cam_thread_id;
    if (pthread_create(&cam_thread_id, NULL, camera_thread, NULL) != 0) {
        perror("Could not create camera thread");
        exit(1);
    }

    printf("Waiting for client...\n");


    // 確定接收到客戶端的地址後，開始發送文件數據
    int n, packet_index = 0;
    unsigned char image_buffer[MAX_IMAGE_SIZE];
    int current_size = 0;
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        char buffer[BUFSIZE];

        // 接收客戶端的連接請求, 皆收到才往閜執行
        if (recvfrom(sockfd, buffer, BUFSIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len) < 0) {
            perror("Error receiving from client");
            continue;
        }

        printf("Received request from client\n");

        struct client_data *new_client_data = malloc(sizeof(struct client_data));
        if (!new_client_data) {
            perror("Failed to allocate memory for client data");
            continue;
        }
        new_client_data->sockfd = sockfd;  // sockfd 每個 thread 都相同
        new_client_data->client_addr = client_addr;
        new_client_data->client_addr_len = client_addr_len;

        pthread_t thread_id;
        printf("Creating thread for client\n");
        if (pthread_create(&thread_id, NULL, handle_client, new_client_data) != 0) {
            perror("Could not create thread");
            free(new_client_data);
            continue;
        }
        pthread_detach(thread_id);
    }


    fclose(F_DUPFD);
    close(sockfd);
    return 0;
}
