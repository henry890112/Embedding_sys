#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8889

int clientSocket;  // 全域變數，存儲客戶端 socket

void sendCommand(const char *command) {
    send(clientSocket, command, strlen(command), 0);

    char buffer[256] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("%s\n", buffer);
}

int main() {
    struct sockaddr_in serverAddr;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    char input[100];

    while (1) {
        // 提示用戶輸入指令
        printf("Enter command (or 'cancel' to quit):");
        fgets(input, sizeof(input), stdin);

        // 去掉输入字符串中的换行符
        input[strcspn(input, "\n")] = '\0';

        sendCommand(input);
        
    }

    close(clientSocket);

    return 0;
}
