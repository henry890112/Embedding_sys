#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 8888

// 商品結構
struct Item {
    char name[50];
    int price;
    int total_quantity;
};

// 商店結構
struct Shop {
    char name[50];
    int distance;
    struct Item items[10];  // 每家商店最多10種商品
    int numItems;
};

// // 結構用來存儲已點餐的資訊
// struct Order {
//     char itemName[50];
//     // add the quantity of the certain product
//     int total_quantity;
// };

// 訂單狀態
enum OrderStatus {
    ORDER_PENDING,
    ORDER_IN_PROGRESS,
    ORDER_DELIVERED,
    ORDER_CANCELED
};

// 訂單結構
struct OrderInfo {
    // struct Order order;
    int shopIndex; // 商店的索引
    enum OrderStatus status;
    int totalAmount; // 記錄同一間餐廳的餐點總金額
};

// 初始化商品清單
void initShopList(struct Shop *shops) {
    // 第一家商店
    strcpy(shops[0].name, "Dessert shop");
    shops[0].distance = 3;
    strcpy(shops[0].items[0].name, "cookie");
    shops[0].items[0].price = 60;
    strcpy(shops[0].items[1].name, "cake");
    shops[0].items[1].price = 80;
    shops[0].numItems = 2;
    // certain product's quantity
    shops[0].items[0].total_quantity = 0;
    shops[0].items[1].total_quantity = 0;

    // 第二家商店
    strcpy(shops[1].name, "Beverage shop");
    shops[1].distance = 5;
    strcpy(shops[1].items[0].name, "tea");
    shops[1].items[0].price = 40;
    strcpy(shops[1].items[1].name, "boba");
    shops[1].items[1].price = 70;
    shops[1].numItems = 2;
    // certain product's quantity
    shops[1].items[0].total_quantity = 0;
    shops[1].items[1].total_quantity = 0;
 
    // 第三家商店
    strcpy(shops[2].name, "Diner");
    shops[2].distance = 8;
    strcpy(shops[2].items[0].name, "fried-rice");
    shops[2].items[0].price = 120;
    strcpy(shops[2].items[1].name, "Egg-drop-soup");
    shops[2].items[1].price = 50;
    shops[2].numItems = 2;
    // certain product's quantity
    shops[2].items[0].total_quantity = 0;
    shops[2].items[1].total_quantity = 0;
}

// 回傳商店清單
void getShopList(struct Shop *shops, char *response) {
    for (int i = 0; i < 3; ++i) {
        // 商店名稱和距離
        sprintf(response + strlen(response), "%s:%dkm\n", shops[i].name, shops[i].distance);
        strcat(response + strlen(response), "- ");

        // 商品資訊
        for (int j = 0; j < shops[i].numItems; ++j) {
            sprintf(response + strlen(response), "%s:$%d", shops[i].items[j].name, shops[i].items[j].price);

            // 如果不是最後一個商品，加上分隔符 "|"
            if (j < shops[i].numItems - 1) {
                strcat(response + strlen(response), "|");
            }
        }
        // 如果不是最後一家商店，加上分隔符 "\n"
        if (i < 2) {
            strcat(response + strlen(response), "\n");
        }
    }
}

// 找尋餐廳索引的輔助函數
int findShopIndexByItem(struct Shop *shops, const char *itemName) {
    for (int i = 0; i < 3; i++) {  // 假設有3家商店
        for (int j = 0; j < shops[i].numItems; j++) {
            if (strcmp(shops[i].items[j].name, itemName) == 0) {
                return i;  // 返回找到的餐廳索引
            }
        }
    }
    return -1;  // 未找到
}

char totalResponse[256] = {0};
// 處理指令
void handleCommand(int clientSocket, struct Shop *shops, struct OrderInfo *orderInfo) {
    char buffer[256] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    printf("Received command from client: %s\n", buffer);

    char response[256] = {0};  // 回傳給客戶端的訊息
    // 使用 strstr 檢查是否包含 "order" 字串
    if (strstr(buffer, "order") != NULL) {
        // 如果是 "order" 指令，解析餐點名稱和數量
        char itemName[50];
        int quantity;
        char quantityStr[50];
        sscanf(buffer, "order %s %d", itemName, &quantity);

        // 檢查是否可以點餐
        if (orderInfo->status == ORDER_PENDING || orderInfo->status == ORDER_IN_PROGRESS) {
            // 假設這裡是一個簡單的資料結構，用來存儲已點餐的資訊
            if (orderInfo->status == ORDER_PENDING) {
                // 如果是新的訂單，初始化 totalAmount 和 shopIndex
                orderInfo->totalAmount = 0;
                orderInfo->shopIndex = findShopIndexByItem(shops, itemName); // 以第一份点的餐点为准
            } 

            // 更新 totalAmount
            // 根據itemName找到對應的餐點價格, 並更新quantity
            for (int i = 0; i < shops[orderInfo->shopIndex].numItems; i++) {
                if (strcmp(shops[orderInfo->shopIndex].items[i].name, itemName) == 0) {
                    orderInfo->totalAmount += shops[orderInfo->shopIndex].items[i].price * quantity;
                    shops[orderInfo->shopIndex].items[i].total_quantity += quantity;
                    break;
                }
            }

            int count = 0;
            for (int i = 0; i < shops[orderInfo->shopIndex].numItems; i++){
                if (shops[orderInfo->shopIndex].items[i].total_quantity != 0){
                    //不等於0才要印出來代表有定東西
                    if (count > 0){
                        strcat(response, "|");
                    }
                    // 將數量轉換成字串
                    sprintf(quantityStr, "%d", shops[orderInfo->shopIndex].items[i].total_quantity);
                    strcat(response, shops[orderInfo->shopIndex].items[i].name);
                    strcat(response, " ");
                    strcat(response, quantityStr);
                    count++;
                }
            }
            orderInfo->status = ORDER_IN_PROGRESS;
            // save the previous order response in prevResponse
            strcat(response, "\n");
            send(clientSocket, response, 256, 0);
        } else {
            sprintf(response, "Cannot place a new order. There is an existing order in progress.\n");
        }
    } else if (strstr(buffer, "confirm") != NULL) {
        // 如果是 "confirm" 指令，確認訂單狀態
        if (orderInfo->status == ORDER_IN_PROGRESS) {
            // 計算外送所需的時間（假設1km花1秒）
            sprintf(response, "Please wait a few minutes...\n");
            send(clientSocket, response, 256, 0);
            int deliveryTime = shops[orderInfo->shopIndex].distance;
            // sleep in deliveryTime seconds
            sleep(deliveryTime);
            // 更新訂單狀態
            orderInfo->status = ORDER_DELIVERED;

            // 回應客戶端
            sprintf(response, "Delivery has arrived and you need to pay %d$\n", orderInfo->totalAmount);
            send(clientSocket, response, 256, 0);

            // 之後此client不會再傳訊息給server(同cancel)
            orderInfo->status = ORDER_CANCELED;
            // sprintf(response, "Order finished. Connection will be closed.\n");
            // send(clientSocket, response, 256, 0);
            // close(clientSocket);

        } else {
            sprintf(response, "Please order some meals\n");
            send(clientSocket, response, 256, 0);
        }
    } else if (strstr(buffer, "shop list") != NULL) {
        // 處理 "shop list" 指令的邏輯
        // getShopList(shops, response);
        sprintf(response, "Dessert shop:3km\n- cookie:$60|cake:$80\nBeverage shop:5km\n- tea:$40|boba:$70\nDiner:8km\n- fried-rice:$120|Egg-drop-soup:$50\n");
        send(clientSocket, response, 256, 0);

    // 之後此client不會再傳訊息給server
    } else if (strstr(buffer, "cancel") != NULL) {
        // 如果是 "cancel" 指令，取消訂單
        orderInfo->status = ORDER_CANCELED;
        sprintf(response, "Order canceled. Connection will be closed.\n");
        send(clientSocket, response, 256, 0);
        // close(clientSocket);
        

    } else {
        // 未知指令的回應
        sprintf(response, "Unknown command");
        send(clientSocket, response, 256, 0);
    }    
}

int main(int argc, char *argv[]) {
    struct Shop shops[3];          // 三家商店
    struct OrderInfo orderInfo = {0};  // 初始化訂單狀態

    if (argc != 2) {
        printf("Usage: ./hw2 <port>\n");
        return 1; // indicate error
    }

    uint16_t port = atoi(argv[1]);

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 可以使用相同的port
    int yes = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("Error setting socket option");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1) {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 5) == -1) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);


    //多一個while是避免可以一直accept新的client導致無法close
    //每次循環都會accept到同一個client
    while(1){
        int clientSocket = accept(serverSocket, NULL, NULL);
        orderInfo.status = ORDER_PENDING;   // 新訂單皆要初始化成ORDER_PENDING
        initShopList(shops);  // 初始化商品清單
        while(1){
            if (clientSocket == -1) {
            perror("Accept failed");
            continue;
            }
            handleCommand(clientSocket, shops, &orderInfo);
            if (orderInfo.status == ORDER_CANCELED) {
                close(clientSocket);
                break;
            }
        }
        
    }
    
    close(serverSocket);

    return 0;
}
