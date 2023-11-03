#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

// 定义菜单项结构
typedef struct {
    char name[50];
    int price;
} MenuItem;

// 定义餐厅结构
typedef struct {
    char name[50];
    char distance[10];
    MenuItem items[2];  // 每家餐厅有两个菜单项
} Restaurant;

int main() {
    int menuOption = 0;
    int orderConfirmed = 0;
    int quantity = 0;

    // //open two fd to write the num
    // int fd1, fd2;
    // fd1 = open("/dev/distance_device", O_RDWR); //用led
    // fd2 = open("/dev/money_device", O_RDWR);   //用7-seg
    
    // if(fd1 < 0 || fd2 < 0){
    //     printf("open device error\n");
    //     return 0;
    // }


    // 定义三家餐厅和其菜单项
    Restaurant restaurants[3];
    strcpy(restaurants[0].name, "Dessert shop");
    strcpy(restaurants[0].distance, "3km");
    strcpy(restaurants[1].name, "Beverage shop");
    strcpy(restaurants[1].distance, "5km");
    strcpy(restaurants[2].name, "Diner");
    strcpy(restaurants[2].distance, "8km");

    // 菜单项
    strcpy(restaurants[0].items[0].name, "cookie");
    restaurants[0].items[0].price = 60;
    strcpy(restaurants[0].items[1].name, "cake");
    restaurants[0].items[1].price = 80;

    strcpy(restaurants[1].items[0].name, "tea");
    restaurants[1].items[0].price = 40;
    strcpy(restaurants[1].items[1].name, "boba");
    restaurants[1].items[1].price = 70;

    strcpy(restaurants[2].items[0].name, "fried rice");
    restaurants[2].items[0].price = 120;
    strcpy(restaurants[2].items[1].name, "egg-drop soup");
    restaurants[2].items[1].price = 50;

    while (1) {
        if (menuOption == 0) {
            // 主選單
            printf("主選單:\n");
            printf("1. shop list\n");
            printf("2. order\n");
            printf("請輸入選項：");
            scanf("%d", &menuOption);
        }
        else if (menuOption == 1) {
            // 顯示餐廳列表
            for (int i = 0; i < 3; i++) {
                printf("%s (%s)\n", restaurants[i].name, restaurants[i].distance);
            }
            printf("按任意鍵返回主選單\n");
            menuOption = 0;
            getchar();
            getchar();
        }
        else if (menuOption == 2) {
            // 開始訂餐
            printf("請選擇餐廳 1~3\n");
            for (int i = 0; i < 3; i++) {
                printf("%d. %s\n", i + 1, restaurants[i].name);
            }
            printf("請輸入選項：");
            int selectedRestaurant;
            scanf("%d", &selectedRestaurant);
            if (selectedRestaurant < 1 || selectedRestaurant > 3) {
                printf("無效選項，請重新輸入\n");
                continue;
            }

            int selectedMenuItem;
            int totalAmount = 0;

            while (1) {
                // 選擇餐點
                printf("%s have these items: \n", restaurants[selectedRestaurant - 1].name);
                printf("1. %s: $%d\n", restaurants[selectedRestaurant - 1].items[0].name, restaurants[selectedRestaurant - 1].items[0].price);
                printf("2. %s: $%d\n", restaurants[selectedRestaurant - 1].items[1].name, restaurants[selectedRestaurant - 1].items[1].price);
                printf("3. 確認\n");
                printf("4. 取消\n");
                printf("請輸入選項：");
                scanf("%d", &selectedMenuItem);

                if (selectedMenuItem == 3) {
                    // 送出訂單
                    if (totalAmount == 0) {
                        printf("請先選擇餐點\n");
                        continue;
                    }

                    printf("訂單已從%s送出，總金額：%d\n", restaurants[selectedRestaurant - 1].name, totalAmount);

                    //Henry change the total amount by int to char
                    char totalAmount_char[10];
                    sprintf(totalAmount_char, "%d", totalAmount);

                    //Henry write the distance and money to my custom device
                    // write(fd1, restaurants[selectedRestaurant - 1].distance, sizeof(restaurants[selectedRestaurant - 1].distance));
                    // write(fd2, &totalAmount, sizeof(totalAmount));
                    orderConfirmed = 1;
                    break;
                }
                else if (selectedMenuItem == 4) {
                    // 取消點餐
                    break;
                }
                else if (selectedMenuItem == 1 || selectedMenuItem == 2) {
                    // 點餐點
                    int menuItemIndex = selectedMenuItem - 1;
                    printf("請輸入訂購份數：");
                    int itemQuantity;
                    scanf("%d", &itemQuantity);
                    totalAmount += (restaurants[selectedRestaurant - 1].items[menuItemIndex].price * itemQuantity);
                }
                else {
                    printf("無效選項，請重新輸入\n");
                }
            }

            // 如果點餐被取消，返回主選單
            if (!orderConfirmed) {
                menuOption = 0;
            }
        }
        else {
            printf("無效選項，請重新輸入\n");
            menuOption = 0;
        }
    }

    return 0;
}
