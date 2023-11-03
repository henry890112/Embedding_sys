#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kdev_t.h> 
#include <linux/cdev.h> 
#include <linux/device.h> 
#include <linux/delay.h> 
#include <linux/uaccess.h> 

// dev_t dev = 0; 
// static struct class *dev_class; 
// static struct cdev etx_cdev; 
  
static int __init my_init(void); 
static void __exit my_exit(void);

// define my driver functions
static ssize_t my_read(struct file *fp, char *buf, size_t count, loff_t *fpos);
static ssize_t my_write(struct file *fp, const char *buf, size_t count, loff_t *fpos);
static int my_open(struct inode *inode, struct file *fp);
static int my_release(struct inode *inode, struct file *fp); 


// static const int seg_for_c[27][16] = {
//     {1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1}, // A
//     {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1}, // b
//     {1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // C
//     {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1}, // d
//     {1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1}, // E
//     {1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1}, // F
//     {1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 0, 0, 0}, // G
//     {0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0, 0, 1}, // H
//     {1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0}, // I
//     {1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0}, // J
//     {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0}, // K
//     {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0}, // L
//     {1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // M
//     {1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // N
//     {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // O
//     {1, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1}, // P
//     {0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0}, // Q
//     {1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1}, // R
//     {1, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}, // S
//     {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0}, // T
//     {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0}, // U
//     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0}, // V
//     {1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0}, // W
//     {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0}, // X
//     {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0}, // Y
//     {1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0}, // Z
//     {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}  // Space
// };

static const char *seg_for_c_str[27] = {
    "1111001100010001", // A
    "0000011100000101", // b
    "1100111100000000", // C
    "0000011001000101", // d
    "1000011100000001", // E
    "1000001100000001", // F
    "1001111100010000", // G
    "0011001100010001", // H
    "1100110001000100", // I
    "1100010001000100", // J
    "0000000001101100", // K
    "0000111100000000", // L
    "0011001110100000", // M
    "0011001110001000", // N
    "1111111100000000", // O
    "1000001101000001", // P
    "0111000001010000", // Q
    "1110001100011001", // R
    "1101110100010001", // S
    "1100000001000100", // T
    "0011111100000000", // U
    "0000001100100010", // V
    "0011001100001010", // W
    "0000000010101010", // X
    "0000000010100100", // Y
    "1100110000100010", // Z
    "0000000000000000"  // Space
};


static char LETTER[1]; // 默认字母为A

static ssize_t my_read(struct file *fp, char __user *buf, size_t count, loff_t *fpos)
{
    char letter = LETTER[0];
    if (letter < 'A' || letter > 'Z') {
        pr_err("ERROR: Invalid letter '%c'\n", letter);
        return -EINVAL;
    }

    // char seg_data = seg_for_c_str[letter - 'A']; // 使用指向 const int 的指针

    if (copy_to_user(buf, seg_for_c_str[letter - 'A'], 16) > 0) {
        pr_err("ERROR: Not all the bytes have been copied to user\n");
        return -EFAULT;
    }

    printk("Read:");
    printk(seg_for_c_str[letter - 'A']);
    printk("\n");

    return count;
}

static ssize_t my_write(struct file *fp, const char __user *buf, size_t count, loff_t *fpos)
{
  if (copy_from_user(LETTER, buf, sizeof(LETTER)) > 0) 
  {
    pr_err("ERROR: Not all the bytes have been copied from user\n");
    return -EFAULT;
  }
  printk("call write\n");
  pr_info("Write : New letter = %s\n", LETTER);

  return count;
}

static int my_open(struct inode *inode, struct file *fp) 
{
  printk("call open\n");
  return 0;
}

static int my_release(struct inode *inode, struct file *fp) 
{
  printk("call release\n");
  return 0;
}

//File operation structure  
static struct file_operations my_fops = 
{
  read: my_read,
  write: my_write,
  open: my_open,
  release: my_release,
};


/* 
** Module Init function 
*/  
#define MAJOR_NUM 456
#define DEVICE_NAME "mydev"

static int my_init(void) {
    printk("call init\n");
    if(register_chrdev(MAJOR_NUM, DEVICE_NAME, &my_fops) < 0) {
        printk("Can not get major %d\n", MAJOR_NUM);
        return (-EBUSY);
    }

    printk("My device is started and the major is %d\n", MAJOR_NUM);
    return 0;
}
static void my_exit(void) 
{
    unregister_chrdev(MAJOR_NUM, DEVICE_NAME);
    printk("call exit\n");
}

module_init(my_init);
module_exit(my_exit);


MODULE_AUTHOR("Henry Chen");
MODULE_DESCRIPTION("16-segment Display Driver");
MODULE_LICENSE("GPL");
