#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/gpio.h>    // Required for the GPIO functions
#include <linux/delay.h>   // Using this header for the msleep() function
#include <linux/uaccess.h> // Required for the copy to user function
#include <linux/timer.h>

#define GPIO_7 (7)  // LED connected to GPIO 7
#define GPIO_8 (8)  // LED connected to GPIO 8

static dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

/*************** Driver Functions **********************/
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);

// File operation structure
static struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .read = etx_read,
        .write = etx_write,
        .open = etx_open,
        .release = etx_release,
    };

static struct timer_list etx_timer; // 定義計時器

// Timer 回調函數
static void etx_timer_callback(struct timer_list *timer) {
    gpio_set_value(GPIO_7, 0); // 關閉 GPIO 7
    gpio_set_value(GPIO_8, 0); // 關閉 GPIO 8
}

/*************** Module Init Function **********************/
static int __init etx_driver_init(void) {
    // Allocating Major number
    if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0) {
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d\n", MAJOR(dev), MINOR(dev));

    // Creating cdev structure
    cdev_init(&etx_cdev, &fops);

    // Initialize the timer
    timer_setup(&etx_timer, etx_timer_callback, 0);

    // Adding character device to the system
    if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_del;
    }

    // Creating struct class
    if ((dev_class = class_create(THIS_MODULE, "etx_class")) == NULL) {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }

    // Creating device
    if ((device_create(dev_class, NULL, dev, NULL, "etx_device")) == NULL) {
        pr_err("Cannot create the Device\n");
        goto r_device;
    }

    // Checking the GPIO is valid or not
    if (!gpio_is_valid(GPIO_7)) {
        pr_err("GPIO %d is not valid\n", GPIO_7);
        goto r_gpio7;
    }
    if (!gpio_is_valid(GPIO_8)) {
        pr_err("GPIO %d is not valid\n", GPIO_8);
        goto r_gpio8;
    }

    // Requesting the GPIO
    if (gpio_request(GPIO_7, "GPIO_7") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_7);
        goto r_gpio7;
    }
    if (gpio_request(GPIO_8, "GPIO_8") < 0) {
        pr_err("ERROR: GPIO %d request\n", GPIO_8);
        goto r_gpio8;
    }

    // Configure the GPIO as output
    gpio_direction_output(GPIO_7, 0);
    gpio_direction_output(GPIO_8, 0);

    pr_info("Device Driver Insert...Done!!!\n");
    return 0;

r_gpio8:
    gpio_free(GPIO_7);
r_gpio7:
    device_destroy(dev_class, dev);
r_device:
    class_destroy(dev_class);
r_class:
    cdev_del(&etx_cdev);
r_del:
    unregister_chrdev_region(dev, 1);
    return -1;
}

/*************** Module Exit Function **********************/
static void __exit etx_driver_exit(void) {
    gpio_set_value(GPIO_7, 0);  // Turn off LED on GPIO 7
    gpio_unexport(GPIO_7);
    gpio_free(GPIO_7);

    gpio_set_value(GPIO_8, 0);  // Turn off LED on GPIO 8
    gpio_unexport(GPIO_8);
    gpio_free(GPIO_8);

    del_timer(&etx_timer);
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!\n");
}

/*************** Driver Functions Implementation **********************/
static int etx_open(struct inode *inode, struct file *file) {
    pr_info("Device File Opened...!!!\n");
    return 0;
}

static int etx_release(struct inode *inode, struct file *file) {
    pr_info("Device File Closed...!!!\n");
    return 0;
}

static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    uint8_t gpio_state = gpio_get_value(GPIO_7);
    len = 1;
    if (copy_to_user(buf, &gpio_state, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied to user\n");
    }
    pr_info("Read function : GPIO_7 = %d\n", gpio_state);
    return 0;
}

static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    char rec_buf[3] = {0};

    if (copy_from_user(rec_buf, buf, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
        return -EFAULT;
    }

    // Check the input and turn on/off LEDs accordingly
    if (strncmp(rec_buf, "01", 2) == 0) {
        // gpio_set_value(GPIO_7, 0);
        gpio_set_value(GPIO_8, 1);
        mod_timer(&etx_timer, jiffies + msecs_to_jiffies(10000));
    } else if (strncmp(rec_buf, "10", 2) == 0) {
        gpio_set_value(GPIO_7, 1);
        // gpio_set_value(GPIO_8, 0);
        mod_timer(&etx_timer, jiffies + msecs_to_jiffies(10000));

    }
    pr_info("Write function: received %s\n", rec_buf);
    return len;
}

/*************** Register Module Init and Exit Functions **********************/
module_init(etx_driver_init);
module_exit(etx_driver_exit);

/*************** Module Information **********************/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Angus");
MODULE_DESCRIPTION("A simple device driver for controlling GPIO LEDs");
