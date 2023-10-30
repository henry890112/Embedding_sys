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
#define GPIO_12 (12)
#define GPIO_16 (16)
#define GPIO_20 (20)
#define GPIO_21 (21) 
  
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
  gpio_state = gpio_get_value(GPIO_21); 
   
  //write to user 
  len = 1; 
  if( copy_to_user(buf, &gpio_state, len) > 0) { 
    pr_err("ERROR: Not all the bytes have been copied to user\n"); 
  } 
   
  pr_info("Read function : GPIO_21 = %d \n", gpio_state); 
   
  return 0; 
} 
 
/* 
** This function will be called when we write the Device file 
*/  
//Henry's code
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    uint8_t rec_buf[10] = {0};
   
    if (copy_from_user(rec_buf, buf, len) > 0) {
        pr_err("ERROR: Not all the bytes have been copied from user\n");
    }

    if (rec_buf[0] >= '0' && rec_buf[0] <= '9') {
        //Henry get the number from the user (echo > 311512049 /dev/etx_device)
        int num;
        for (int i = 0; i < 9; i++)
        {
            pr_info("rec_buf[%d] = %c\n", i, rec_buf[i]);
        

            num = rec_buf[i] - '0'; // 將字符轉換為整數  

            if (num >= 0 && num <= 9) {
                int gpio_values[4];

                // 轉換為四位二進制數
                gpio_values[0] = (num & 1) ? 1 : 0;
                gpio_values[1] = (num & 2) ? 1 : 0;
                gpio_values[2] = (num & 4) ? 1 : 0;
                gpio_values[3] = (num & 8) ? 1 : 0;

                pr_info("Write Function: GPIO Set = %d%d%d%d\n", gpio_values[3], gpio_values[2], gpio_values[1], gpio_values[0]);

                // 設置 GPIO 腳位的值
                gpio_set_value(GPIO_12, gpio_values[0]);
                gpio_set_value(GPIO_16, gpio_values[1]);
                gpio_set_value(GPIO_20, gpio_values[2]);
                gpio_set_value(GPIO_21, gpio_values[3]);
                // 延遲 1 秒
                msleep(1000);
                gpio_set_value(GPIO_12, 0);
                gpio_set_value(GPIO_16, 0);
                gpio_set_value(GPIO_20, 0);
                gpio_set_value(GPIO_21, 0);
                msleep(1000);
            } else {
                pr_err("Invalid digit: Please provide a digit between 0 and 9\n");
            }
        }
    } else {
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
  if((dev_class = class_create(THIS_MODULE,"etx_class")) == NULL){ 
    pr_err("Cannot create the struct class\n"); 
    goto r_class; 
  } 
  
  /*Creating device*/ 
  //Henry create the device here call etx_device
  if((device_create(dev_class,NULL,dev,NULL,"etx_device")) == NULL){ 
    pr_err( "Cannot create the Device \n"); 
    goto r_device; 
  } 
   
    if(gpio_is_valid(GPIO_12) == false){ 
        pr_err("GPIO %d is not valid\n", GPIO_12); 
        goto r_device; 
    }

    if(gpio_is_valid(GPIO_16) == false){ 
        pr_err("GPIO %d is not valid\n", GPIO_16); 
        goto r_device; 
    }

    if(gpio_is_valid(GPIO_20) == false){ 
        pr_err("GPIO %d is not valid\n", GPIO_20); 
        goto r_device; 
    }
    //Checking the GPIO is valid or not 
    if(gpio_is_valid(GPIO_21) == false){ 
        pr_err("GPIO %d is not valid\n", GPIO_21); 
        goto r_device; 
    } 
   
   if(gpio_request(GPIO_12,"GPIO_12") < 0){ 
        pr_err("ERROR: GPIO %d request\n", GPIO_12); 
        goto r_gpio; 
    } 

    if(gpio_request(GPIO_16,"GPIO_16") < 0){ 
        pr_err("ERROR: GPIO %d request\n", GPIO_16); 
        goto r_gpio; 
    } 

    if(gpio_request(GPIO_20,"GPIO_20") < 0){ 
        pr_err("ERROR: GPIO %d request\n", GPIO_20); 
        goto r_gpio; 
    }

    //Requesting the GPIO 
    if(gpio_request(GPIO_21,"GPIO_21") < 0){ 
        pr_err("ERROR: GPIO %d request\n", GPIO_21); 
        goto r_gpio; 
    } 
   
  //configure the GPIO as output 
  gpio_direction_output(GPIO_12, 0);
  gpio_direction_output(GPIO_16, 0);
  gpio_direction_output(GPIO_20, 0);
  gpio_direction_output(GPIO_21, 0); 
   
  /* Using this call the GPIO 21 will be visible in /sys/class/gpio/ 
  ** Now you can change the gpio values by using below commands also. 
  ** echo 1 > /sys/class/gpio/gpio21/value  (turn ON the LED) 
  ** echo 0 > /sys/class/gpio/gpio21/value  (turn OFF the LED) 
  ** cat /sys/class/gpio/gpio21/value  (read the value LED) 
  **  
  ** the second argument prevents the direction from being changed. 
  */ 
  gpio_export(GPIO_12, false);
  gpio_export(GPIO_16, false);
  gpio_export(GPIO_20, false);
  gpio_export(GPIO_21, false);

    pr_info("Device Driver Insert...Done!!!\n"); 
  return 0; 
  
r_gpio: 
  gpio_free(GPIO_12);
  gpio_free(GPIO_16);
  gpio_free(GPIO_20);
  gpio_free(GPIO_21); 
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
  gpio_unexport(GPIO_12);
  gpio_unexport(GPIO_16);
  gpio_unexport(GPIO_20);
  gpio_unexport(GPIO_21); 

  gpio_free(GPIO_12);
  gpio_free(GPIO_16);
  gpio_free(GPIO_20);
  gpio_free(GPIO_21); 

  device_destroy(dev_class,dev); 
  class_destroy(dev_class); 
  cdev_del(&etx_cdev); 
  unregister_chrdev_region(dev, 1); 
  pr_info("Device Driver Remove...Done!!\n"); 
} 
  
module_init(etx_driver_init); 
module_exit(etx_driver_exit); 
  
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>"); 
MODULE_DESCRIPTION("A simple device driver - GPIO Driver"); 
MODULE_VERSION("1.32");