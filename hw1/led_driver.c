#include <linux/kernel.h> 
#include <linux/init.h> 
#include <linux/module.h> 
#include <linux/kdev_t.h> 
#include <linux/fs.h> 
#include <linux/cdev.h> 
#include <linux/device.h> 
#include <linux/delay.h> 
#include <linux/uaccess.h>  //copy_to/from_user() 
#include <linux/gpio.h>     //GPIO 
  
//LED is connected to this GPIO 
#define GPIO_10 (10)
#define GPIO_9 (9)
#define GPIO_22 (22)
#define GPIO_5 (5)
#define GPIO_6 (6)
#define GPIO_13 (13)
#define GPIO_19 (19)
#define GPIO_26 (26)
  
dev_t dev = 0; 
static struct class *dev_class; 
static struct cdev etx_cdev; 
  
static int __init etx_driver_init(void); 
static void __exit etx_driver_exit(void); 
  
  
/*************** Driver functions **********************/ 
static int     etx_open(struct inode *inode, struct file *file); 
static int     etx_release(struct inode *inode, struct file *file); 
static ssize_t etx_read(struct file *filp,  
                char __user *buf, size_t len,loff_t * off); 
static ssize_t etx_write(struct file *filp,  
                const char *buf, size_t len, loff_t * off); 
/******************************************************/ 

//File operation structure  
static struct file_operations fops = 
{ 
    .owner          = THIS_MODULE, 
    .read           = etx_read, 
    .write          = etx_write, 
    .open           = etx_open, 
    .release        = etx_release, 
}; 
 
/* 
** This function will be called when we open the Device file 
*/  
static int etx_open(struct inode *inode, struct file *file) 
{ 
    pr_info("Device File Opened...!!!\n"); 
    return 0; 
} 
 
/* 
** This function will be called when we close the Device file 
*/ 
static int etx_release(struct inode *inode, struct file *file) 
{ 
    pr_info("Device File Closed...!!!\n"); 
    return 0; 
} 
 
/* 
** This function will be called when we read the Device file 
*/  

//Henry read not yet
static ssize_t etx_read(struct file *filp,  
                char __user *buf, size_t len, loff_t *off) 
{ 
    uint8_t gpio_state = 0; 

    //reading GPIO value 
    gpio_state = gpio_get_value(GPIO_13); 

    //write to user 
    len = 1; 
    if( copy_to_user(buf, &gpio_state, len) > 0) { 
    pr_err("ERROR: Not all the bytes have been copied to user\n"); 
    } 

    pr_info("Read function : GPIO_21 = %d \n", gpio_state); 

    return 0; 
} 

const int eight_led_pins[8] = {GPIO_10, GPIO_9, GPIO_22, GPIO_5, GPIO_6, GPIO_13, GPIO_19, GPIO_26};

/* 
** This function will be called when we write the Device file 
*/  
//Henry's code
int total_dis = 0;
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    //buf store the char from user (1 byte)
    uint8_t rec_buf[10] = {0};
   
    if (copy_from_user(rec_buf, buf, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }
    // change the rec_buf to int
    total_dis = rec_buf[0] - '0'; // 將字符轉換為整數
    pr_info("Write Function: total_dis = %d\n", total_dis);

    if (rec_buf[0] >= '0' && rec_buf[0] <= '9') {
        for (int remaining_dis = total_dis; remaining_dis >= 0; remaining_dis--)
        {
            // 关闭所有LED
            for (int i = 0; i < total_dis; i++) 
            {
                gpio_set_value(eight_led_pins[i], 0);
            }
            // 打开距离对应数量的LED
            for (int i = 0; i < remaining_dis; i++) 
            {
                gpio_set_value(eight_led_pins[i], 1);
                pr_info("LED %d is on\n", i);
            }
            msleep(1000);

        }
    }else 
    {
        pr_err("Invalid input: Please provide a digit between 0 and 9\n");
    }
   
    return len;
}

/* 
** Module Init function 
*/  
static int __init etx_driver_init(void) 
{ 
    /*Allocating Major number*/ 
    if((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) <0){ 
    pr_err("Cannot allocate major number\n"); 
    goto r_unreg; 
    } 
    pr_info("Major = %d Minor = %d \n",MAJOR(dev), MINOR(dev)); 

    /*Creating cdev structure*/ 
    cdev_init(&etx_cdev,&fops); 

    /*Adding character device to the system*/ 
    if((cdev_add(&etx_cdev,dev,1)) < 0){ 
    pr_err("Cannot add the device to the system\n"); 
    goto r_del; 
    } 

    /*Creating struct class*/ 
    if((dev_class = class_create(THIS_MODULE,"distance_device_class")) == NULL){ 
    pr_err("Cannot create the struct class\n"); 
    goto r_class; 
    } 

    /*Creating device*/ 
    //Henry create the device here call etx_device
    if((device_create(dev_class,NULL,dev,NULL,"distance_device")) == NULL){ 
    pr_err( "Cannot create the Device \n"); 
    goto r_device; 
    } 
   
    for(int i = 0; i < 8; i++) {
        if(gpio_is_valid(eight_led_pins[i]) == false){ 
            pr_err("GPIO %d is not valid\n", eight_led_pins[i]); 
            goto r_device; 
        }
        if(gpio_request(eight_led_pins[i],"eight_led_pins[i]") < 0){ 
            pr_err("ERROR: GPIO %d request\n", eight_led_pins[i]); 
            goto r_gpio; 
        }
        if(gpio_direction_output(eight_led_pins[i], 0) < 0)
        {
            pr_err("ERROR: GPIO %d direction\n", eight_led_pins[i]);
            goto r_gpio;
        }
        gpio_direction_output(eight_led_pins[i], 0);
    }

    /* Using this call the GPIO 21 will be visible in /sys/class/gpio/ 
    ** Now you can change the gpio values by using below commands also. 
    ** echo 1 > /sys/class/gpio/gpio21/value  (turn ON the LED) 
    ** echo 0 > /sys/class/gpio/gpio21/value  (turn OFF the LED) 
    ** cat /sys/class/gpio/gpio21/value  (read the value LED) 
    **  
    ** the second argument prevents the direction from being changed. 
    */ 

    pr_info("Device Driver Insert...Done!!!\n"); 
    return 0; 
  
r_gpio: 
    for(int i = 0; i < 8; i++) 
    {
        gpio_free(eight_led_pins[i]);
    }

r_device: 
    device_destroy(dev_class,dev); 
r_class: 
    class_destroy(dev_class); 
r_del: 
    cdev_del(&etx_cdev); 
r_unreg: 
    unregister_chrdev_region(dev,1); 
   
    return -1; 
} 
 
/* 
** Module exit function 
*/  
static void __exit etx_driver_exit(void) 
{ 
    for(int i = 0; i < 8; i++) 
    {
        gpio_set_value(eight_led_pins[i], 0);
        gpio_free(eight_led_pins[i]);
    }

  device_destroy(dev_class,dev); 
  class_destroy(dev_class); 
  cdev_del(&etx_cdev); 
  unregister_chrdev_region(dev, 1); 
  pr_info("Device Driver Remove...Done!!\n"); 
} 
  
module_init(etx_driver_init); 
module_exit(etx_driver_exit); 
  
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("EmbeTronicX <henry890112@gmail.com>"); 
MODULE_DESCRIPTION("A simple device driver - GPIO Driver"); 
MODULE_VERSION("1.32");