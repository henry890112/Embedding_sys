#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define PORT 12345
#define BUFFER_SIZE 1024

int sockfd, newsockfd;

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void* handleClient(void* arg) {
    printf("Thread ID: %ld\n", pthread_self());
    int client_sockfd = *(int*)arg;
    char buffer[BUFFER_SIZE];
    FILE *file;

    file = fopen("./pic.jpg", "rb");
    printf("file address: %p\n", file);
    if (file == NULL) {
        perror("ERROR opening file");
        close(client_sockfd);
        pthread_exit(NULL);
    }

    int n;
    while ((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        if (write(client_sockfd, buffer, n) < 0) {
            perror("ERROR writing to socket");
            fclose(file);
            close(client_sockfd);
            pthread_exit(NULL);
        }
    }
    printf("傳送照片成功\n");
    fclose(file);

    // while (1) {
    //     bzero(buffer, BUFFER_SIZE);
    //     n = read(client_sockfd, buffer, BUFFER_SIZE);
    //     if (n < 0) {
    //         perror("ERROR reading from socket");
    //         close(client_sockfd);
    //         pthread_exit(NULL);
    //     }
    //     printf("收到來自client的訊息: %s\n", buffer);
    //     if (strcmp(buffer, "exit") == 0) {
    //         break;
    //     }
    // }
    close(client_sockfd);
    pthread_exit(NULL);
}

void stop_child(int signum) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void safe_close_routine(int signum) {
    close(sockfd);
    exit(signum);            
}

int main() {
    printf("Back PID: %d\n", getpid());
    
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    signal(SIGCHLD, stop_child);
    signal(SIGINT, safe_close_routine);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("Error setting socket option");
        exit(EXIT_FAILURE);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        perror("ERROR on binding");

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t*)&clilen);
        if (newsockfd < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
        }

        pthread_t thread;
        if (pthread_create(&thread, NULL, handleClient, &newsockfd) != 0) {
            perror("Error creating thread");
            close(newsockfd);
        }
        pthread_detach(thread);
    }

    close(sockfd);
    return 0;
}