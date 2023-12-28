#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <limits.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <errno.h>

#define SERVER_PORT 8888
#define NUM_DELIVERY_PERSONS 2
#define MAX_CLIENTS 1000

int totalIncome = 0;
int totalCustomer = 0;
int sem_id;

int serverSocket;  // 全域變數，存儲客戶端 socket
void stop_parent(int signum) {
    signal(SIGINT, SIG_DFL);
    close(serverSocket);

    semctl(sem_id, 0, IPC_RMID, 0); // 移除信號量
    //產生"result.txt"後斷線並結束程式
    FILE *fp;
    
    fp = fopen("result.txt", "w");
    fprintf(fp, "customer: %d\n", totalCustomer);
    fprintf(fp, "income: %d$\n", totalIncome);
    fclose(fp);
    printf("Server closed.\n");
    printf("customer: %d\n", totalCustomer);
    printf("income: %d$\n", totalIncome);
    exit(signum);
}

int P(int s) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = -1;
    sop.sem_flg = 0;
    if (semop(s, &sop, 1) < 0) {
        perror("P(): semop failed");
        exit(EXIT_FAILURE);
    } else {
        return 0;
    }
}

int V(int s) {
    struct sembuf sop;
    sop.sem_num = 0;
    sop.sem_op = 1;
    sop.sem_flg = 0;
    if (semop(s, &sop, 1) < 0) {
        perror("V(): semop failed");
        exit(EXIT_FAILURE);
    } else {
        return 0;
    }
}

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

// DeliveryPerson結構
// orderQueue存取所有要送的訂單
// recieveTime紀錄送貨員木訂單送貨狀態, 決定什麼時後要dequeue
// DeliveryPerson為global!!!
struct DeliveryPerson {
    int id;
    pthread_mutex_t mutex;
    // 此mutex控制特定送貨員要在第1個client執行完才能執行第2個client
    pthread_mutex_t mutex_control_queue;
    pthread_cond_t condition;
    int secondsRemaining;
    int recieveTime;  // 當下訂單送貨員的時間 0~8
    int orderQueue[MAX_CLIENTS];  // Queue to store distances
    int front, rear;
    // 加上此2來去判斷是否要使特定thread開始跑單
    int totalClient;
    int canRunClientId;
};


// 初始化结构体
void delivery_person_init(struct DeliveryPerson *deliveryPersons) {
    for(int i = 0; i < NUM_DELIVERY_PERSONS; i++) {
        deliveryPersons[i].id = i;
        deliveryPersons[i].secondsRemaining = 0;
        deliveryPersons[i].recieveTime = 0;
        deliveryPersons[i].front = deliveryPersons[i].rear = -1;
        deliveryPersons[i].totalClient = 0;
        deliveryPersons[i].canRunClientId = 1;
        // 初始化互斥鎖
        if (pthread_mutex_init(&deliveryPersons[i].mutex, NULL) != 0) {
            printf("互斥鎖初始化失敗\n");
            return;
        }
        if (pthread_mutex_init(&deliveryPersons[i].mutex_control_queue, NULL) != 0) {
            printf("互斥鎖初始化失敗\n");
            return;
        }
    }
}

struct DeliveryPerson deliveryPersons[NUM_DELIVERY_PERSONS];

// 將distance加入queue
void enqueueDistance(struct DeliveryPerson *deliveryPersons, int distance) {

    if (deliveryPersons->rear == -1) {
        deliveryPersons->front = deliveryPersons->rear = 0;
    } else {
        deliveryPersons->rear = (deliveryPersons->rear + 1) % MAX_CLIENTS;
    }
    deliveryPersons->orderQueue[deliveryPersons->rear] = distance;
}

// 從queue中取出distance
void dequeueDistance(struct DeliveryPerson *deliveryPersons) {

    int distance = deliveryPersons->orderQueue[deliveryPersons->front];

    if (deliveryPersons->front == deliveryPersons->rear) {
        deliveryPersons->front = deliveryPersons->rear = -1;
    } else {
        deliveryPersons->front = (deliveryPersons->front + 1) % MAX_CLIENTS;
    }
}

// 找到最不忙的送餐员
int findLeastBusyPerson(struct DeliveryPerson *deliveryPersons) {
   
    int cur_personId = 0;
    int shortestTime = INT_MAX;
    for(int i = 0; i < NUM_DELIVERY_PERSONS; i++) {
        if(deliveryPersons[i].secondsRemaining < shortestTime) {
            shortestTime = deliveryPersons[i].secondsRemaining;
            cur_personId = i; 
        }
    }
    return cur_personId;
}



int dispatchOrder(struct OrderInfo order, struct Shop *shops, struct DeliveryPerson *deliveryPersons, const char *condition, int personId) {
    // 派单函数  // condition variable 若送單則加時間, 若送完則減時間
    /*
    //Henry  dispatchOrder要處理當下thread的問題, 若多個thread同時用deliveryPersons, 若在裡面做加減法會有問題！！！！    我要在dispatchOrder做的事情應該只有
    1. confirm後addTime到secondsRemaining, 後回傳secondsRemaining給client(也就是特定thread)去sleep
    3. 當多個thread同時用deliveryPersons時, 會變成減很多次secondsRemaining這樣不對
    4. 當有送貨原在送貨時此送貨員就用mutex鎖住, 並且在送完貨後解鎖且減去它所送的時間
    */
    // 找到最不忙的送餐员
    int addedTime = shops[order.shopIndex].distance;
    int predict_recieveTime;
    if (strstr(condition, "add") != NULL) 
    {
        deliveryPersons[personId].secondsRemaining += addedTime;
        enqueueDistance(&deliveryPersons[personId], addedTime);
        printf("ID: %d, enqueue增加第%d單!!!!!!!!!!!!!!!!!!!!!\n", personId, deliveryPersons[personId].canRunClientId);
    }
    else if (strstr(condition, "minus_one") != NULL)
    {
        if(deliveryPersons[personId].secondsRemaining > 0) {
            deliveryPersons[personId].secondsRemaining --;
            deliveryPersons[personId].recieveTime ++;
            printf("ID: %d; recieveTime: %d; secondsRemaining: %d\n", personId, deliveryPersons[personId].recieveTime, deliveryPersons[personId].secondsRemaining);
            if(deliveryPersons[personId].orderQueue[deliveryPersons[personId].front] == deliveryPersons[personId].recieveTime) {
                dequeueDistance(&deliveryPersons[personId]);
                printf("ID: %d, dequeue送完第%d單!!!!!!!!!!!!!!!!!!!!!!\n", personId, deliveryPersons[personId].canRunClientId);
                deliveryPersons[personId].recieveTime = 0;
            }
        }
    }
    else if (strstr(condition, "check_time") != NULL)
    {
        predict_recieveTime = deliveryPersons[personId].secondsRemaining + addedTime;
        return predict_recieveTime;
    }

    return deliveryPersons[personId].secondsRemaining;
}


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
void handleCommand(int clientSocket, struct Shop *shops, struct OrderInfo *orderInfo, struct DeliveryPerson *deliveryPersons) {
    char buffer[256] = {0};
    int personId, deliveryTime, predict_recieveTime, currentClientId;
    recv(clientSocket, buffer, sizeof(buffer), 0);
    // print current thread id
    printf("Current thread id: %ld\n", pthread_self());
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
            // strcat(response, "\n");
            send(clientSocket, response, 256, 0);
        } else {
            sprintf(response, "Cannot place a new order. There is an existing order in progress.\n");
        }
    //Henry main point
    } else if (strstr(buffer, "confirm") != NULL) {
        // 如果是 "confirm" 指令，確認訂單狀態
        if (orderInfo->status == ORDER_IN_PROGRESS) {
            //Henry section1
            // 計算外送所需的時間（假設1km花1秒）
            sprintf(response, "Please wait a few minutes...");
            send(clientSocket, response, 256, 0);
            
            /*
            確認當時least busy的外送員, 
            並將所需要的時間利用dispatchOrder加到訂單狀態中
            */
            //Henry 此處的mutex一次鎖所有送貨員, 因為secondRemaining兩個送貨原在checkTime要在同一個時間點看
            // 找到當下最不忙的送餐員c
            pthread_mutex_lock(&deliveryPersons->mutex);
            personId = findLeastBusyPerson(deliveryPersons);
            predict_recieveTime = dispatchOrder(*orderInfo, shops, deliveryPersons, "check_time", personId);
            printf("least busy personId: %d; predict_recieveTime: %d ?????????????????????????????\n", personId, predict_recieveTime);

            if (predict_recieveTime > 30)
            {
                sprintf(response, "Delivery has arrived and you need to pay %d$", orderInfo->totalAmount);
                send(clientSocket, response, 256, 0);
                orderInfo->status = ORDER_CANCELED;
                pthread_mutex_unlock(&deliveryPersons->mutex);

            }else{
                // 確定此送貨員要送貨後, 將所需要的當下時間加到secondsRemaining中
                dispatchOrder(*orderInfo, shops, deliveryPersons, "add", personId);
                // deliveryTime只會是3, 5, 8
                deliveryTime = shops[orderInfo->shopIndex].distance;
                // totalClient加一, 排序用
                deliveryPersons[personId].totalClient++;
                currentClientId = deliveryPersons[personId].totalClient;
                
                /*
                currentClientId就是當下的client id
                canRunClientId就是當下可以開始送貨的client id
                canRunClientId<=currentClientId
                */
                printf("ID: %d, currentClientId: %d, canRunClientId: %d\n", personId, currentClientId, deliveryPersons[personId].canRunClientId);
                fflush(stdout);
                pthread_mutex_unlock(&deliveryPersons->mutex);

                //Henry section2
                //  利用此mutex來控制多個thread同時用deliveryPersons時, 不會有問題
                pthread_mutex_lock(&deliveryPersons[personId].mutex_control_queue);
                // 等待該線程的順序到來
                pthread_cond_signal(&deliveryPersons[personId].condition);
                while (currentClientId != deliveryPersons[personId].canRunClientId) {
                    pthread_cond_wait(&deliveryPersons[personId].condition, &deliveryPersons[personId].mutex_control_queue);
                }
                
                for (int i = 0; i < deliveryTime; i++) {
                    // 減去特定送貨員的secondsRemaining
                    pthread_mutex_lock(&deliveryPersons->mutex);
                    dispatchOrder(*orderInfo, shops, deliveryPersons, "minus_one", personId);
                    pthread_mutex_unlock(&deliveryPersons->mutex);
                    // sleep 要在鎖解開後才能執行, 不然其他thread無法執行
                    sleep(1); 
                }
                
                // 更新訂單狀態
                orderInfo->status = ORDER_DELIVERED;
                // 回應客戶端
                sprintf(response, "Delivery has arrived and you need to pay %d$", orderInfo->totalAmount);
                send(clientSocket, response, 256, 0);
                printf("ID: %d,送到囉\n", personId);

                // 避免兩個thread同時用deliveryPersons時, 只會加1次
                pthread_mutex_lock(&deliveryPersons->mutex);
                totalCustomer++;
                totalIncome += orderInfo->totalAmount;
                pthread_mutex_unlock(&deliveryPersons->mutex);
                // canRunClientId加一, 可以開始送下一單了
                deliveryPersons[personId].canRunClientId++;

                pthread_mutex_unlock(&deliveryPersons[personId].mutex_control_queue);
                // 之後此client不會再傳訊息給server(同cancel)
                orderInfo->status = ORDER_CANCELED;
            }

        } else {
            sprintf(response, "Please order some meals");
            send(clientSocket, response, 256, 0);
        }

    } else if (strstr(buffer, "shop list") != NULL) {
        // 處理 "shop list" 指令的邏輯
        // getShopList(shops, response);
        sprintf(response, "Dessert shop:3km\n- cookie:$60|cake:$80\nBeverage shop:5km\n- tea:$40|boba:$70\nDiner:8km\n- fried-rice:$120|Egg-drop-soup:$50\n");
        send(clientSocket, response, 256, 0);

    } else if (strstr(buffer, "cancel") != NULL) {
        // 如果是 "cancel" 指令，取消訂單, 此client不會再傳訊息給server
        orderInfo->status = ORDER_CANCELED;
        sprintf(response, "Order canceled. Connection will be closed.\n");
        send(clientSocket, response, 256, 0);
        
    } 
    else {
        // 未知指令的回應
        sprintf(response, "Unknown command");
        send(clientSocket, response, 256, 0);
    }    
}


void* handleClient(void* arg) {
    // 不能在此初始化deliveryPersons, 因為deliveryPersons是一個server的全域變數
    // 每次新開一個client都要有自己的initShopList等資訊
    struct Shop shops[3];          // 三家商店
    struct OrderInfo orderInfo = {0};  // 初始化訂單狀態

    orderInfo.status = ORDER_PENDING;   // 新訂單皆要初始化成ORDER_PENDING
    int clientSocket = *(int *)arg;
    initShopList(shops);  // 初始化商品清單
    while(1){
        if (clientSocket == -1) {
        perror("Accept failed");
        continue;
        }
        handleCommand(clientSocket, shops, &orderInfo, deliveryPersons);
        if (orderInfo.status == ORDER_CANCELED) {
            close(clientSocket);
            break;
        }
    }
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: ./hw3 <port>\n");
        return 1; // indicate error
    }

    uint16_t port = atoi(argv[1]);

    signal(SIGINT, stop_parent);

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
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

    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d...\n", port);
    
    // initialize deliveryPersons
    delivery_person_init(deliveryPersons);

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while(1){
        int client_socket = accept(serverSocket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket == -1) {
            perror("Error accepting connection");
            continue;
        }

        pthread_t client_thread;
        int *client_socket_ptr = (int *)malloc(sizeof(int));
        *client_socket_ptr = client_socket;
        if (pthread_create(&client_thread, NULL, handleClient, (void *)client_socket_ptr) != 0) {
            perror("pthread_create");
            close(client_socket);
            free(client_socket_ptr);
        }
        pthread_detach(client_thread);
    }
    
    close(serverSocket);
    return 0;
}
