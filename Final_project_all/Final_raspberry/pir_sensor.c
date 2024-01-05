#include <gpiod.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define GPIO_CHIP "/dev/gpiochip0"
#define PIR_SENSOR_PIN_FRONT 20  // 前門的 PIR 傳感器連接到 GPIO 20
#define PIR_SENSOR_PIN_BACK 21   // 後門的 PIR 傳感器連接到 GPIO 21

int main(int argc, char *argv[]) {
    struct gpiod_chip *chip;
    struct gpiod_line *line_front;
    struct gpiod_line *line_back;
    int value_front, value_back, pid1, pid2;

    if (argc != 3) {
        printf("Usage: %s <pid_of_python_front> <pid_of_python_back>\n", argv[0]);
        return 1;
    }

    pid1 = atoi(argv[1]);
    pid2 = atoi(argv[2]);

    chip = gpiod_chip_open(GPIO_CHIP);
    if (!chip) {
        perror("gpiod_chip_open");
        return 1;
    }

    line_front = gpiod_chip_get_line(chip, PIR_SENSOR_PIN_FRONT);
    line_back = gpiod_chip_get_line(chip, PIR_SENSOR_PIN_BACK);
    if (!line_front || !line_back) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        return 1;
    }

    if (gpiod_line_request_input(line_front, "pir_sensor_read_front") < 0 ||
        gpiod_line_request_input(line_back, "pir_sensor_read_back") < 0) {
        perror("gpiod_line_request_input");
        gpiod_line_release(line_front);
        gpiod_line_release(line_back);
        gpiod_chip_close(chip);
        return 1;
    }

    printf("監測前門和後門的 PIR 運動感測器...\n");
    while (1) {
        value_front = gpiod_line_get_value(line_front);
        value_back = gpiod_line_get_value(line_back);
        if (value_front < 0 || value_back < 0) {
            perror("gpiod_line_get_value");
            gpiod_line_release(line_front);
            gpiod_line_release(line_back);
            gpiod_chip_close(chip);
            return 1;
        }

        if (value_front == 1) {
            printf("前門檢測到有人了！\n");
            kill(pid1, SIGUSR1);
        }
        else{
            printf("前門沒人\n");
        }
        if (value_back == 1) {
            printf("後門檢測到有人了！\n");
            kill(pid2, SIGUSR1); 
        }
        else{
            printf("後門沒人\n");
        }
        sleep(1);  // 每秒檢查一次
    }

    gpiod_line_release(line_front);
    gpiod_line_release(line_back);
    gpiod_chip_close(chip);
    return 0;
}
