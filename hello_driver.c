#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define BUFFER_SIZE 256
#define DEVICE_NAME "hello"

/* Module information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("PGCHENG");
MODULE_DESCRIPTION("A simple Linux character device driver");
MODULE_VERSION("1.0");

/* Global variables */
static dev_t dev_num;                    /* Device number (major, minor) */
static struct cdev hello_cdev;           /* Character device structure */
static struct class *hello_class;        /* Device class */
static struct device *hello_device;      /* Device */
static char *kernel_buffer;              /* Kernel buffer for user data */

/**
 * File operation: open
 * Called when user opens the device file
 */
static int hello_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "hello_driver: Device opened\n");
    return 0;
}

/**
 * File operation: release
 * Called when user closes the device file
 */
static int hello_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "hello_driver: Device closed\n");
    return 0;
}

/**
 * File operation: write
 * Receives user space data and copies to kernel buffer
 */
static ssize_t hello_write(struct file *filp, const char __user *user_buf,
                           size_t count, loff_t *f_pos)
{
    size_t to_copy;
    
    if (count == 0)
        return 0;
    
    /* Limit copy size to buffer size */
    to_copy = (count < BUFFER_SIZE) ? count : (BUFFER_SIZE - 1);
    
    /* Copy data from user space to kernel space */
    if (copy_from_user(kernel_buffer, user_buf, to_copy)) {
        printk(KERN_ERR "hello_driver: Failed to copy data from user space\n");
        return -EFAULT;
    }
    
    /* Null terminate the string */
    kernel_buffer[to_copy] = '\0';
    
    printk(KERN_INFO "hello_driver: Received %zu bytes: %s\n", to_copy, kernel_buffer);
    
    return to_copy;
}

/**
 * File operation: read
 * Returns "Hello {user_input}" to user space
 */
static ssize_t hello_read(struct file *filp, char __user *user_buf,
                          size_t count, loff_t *f_pos)
{
    char read_buffer[BUFFER_SIZE];
    size_t data_len;
    size_t to_copy;
    
    /* Format the response string */
    if (kernel_buffer[0] != '\0') {
        data_len = snprintf(read_buffer, BUFFER_SIZE, "Hello %s\n", kernel_buffer);
    } else {
        data_len = snprintf(read_buffer, BUFFER_SIZE, "Hello World\n");
    }
    
    /* Prevent reading past end of data */
    if (*f_pos >= data_len)
        return 0;
    
    /* Limit copy to remaining data and user buffer size */
    to_copy = (count < (data_len - *f_pos)) ? count : (data_len - *f_pos);
    
    /* Copy data from kernel space to user space */
    if (copy_to_user(user_buf, read_buffer + *f_pos, to_copy)) {
        printk(KERN_ERR "hello_driver: Failed to copy data to user space\n");
        return -EFAULT;
    }
    
    /* Update file position */
    *f_pos += to_copy;
    
    printk(KERN_INFO "hello_driver: Sent %zu bytes to user space\n", to_copy);
    
    return to_copy;
}

/* File operations structure */
static const struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .release = hello_release,
    .read = hello_read,
    .write = hello_write,
};

/**
 * Module initialization function
 * Called when the module is loaded
 */
static int __init hello_init(void)
{
    int ret;
    
    printk(KERN_INFO "hello_driver: Initializing device driver\n");
    
    /* Allocate kernel buffer */
    kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!kernel_buffer) {
        printk(KERN_ERR "hello_driver: Failed to allocate kernel buffer\n");
        return -ENOMEM;
    }
    kernel_buffer[0] = '\0';
    
    /* Dynamically allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "hello_driver: Failed to allocate device number\n");
        kfree(kernel_buffer);
        return ret;
    }
    
    printk(KERN_INFO "hello_driver: Major number: %d, Minor number: %d\n",
           MAJOR(dev_num), MINOR(dev_num));
    
    /* Initialize and add character device */
    cdev_init(&hello_cdev, &hello_fops);
    hello_cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&hello_cdev, dev_num, 1);
    if (ret < 0) {
        printk(KERN_ERR "hello_driver: Failed to add character device\n");
        unregister_chrdev_region(dev_num, 1);
        kfree(kernel_buffer);
        return ret;
    }
    
    /* Create device class */
    hello_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(hello_class)) {
        printk(KERN_ERR "hello_driver: Failed to create device class\n");
        cdev_del(&hello_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(kernel_buffer);
        return PTR_ERR(hello_class);
    }
    
    /* Create device node */
    hello_device = device_create(hello_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(hello_device)) {
        printk(KERN_ERR "hello_driver: Failed to create device node\n");
        class_destroy(hello_class);
        cdev_del(&hello_cdev);
        unregister_chrdev_region(dev_num, 1);
        kfree(kernel_buffer);
        return PTR_ERR(hello_device);
    }
    
    printk(KERN_INFO "hello_driver: Device driver initialized successfully\n");
    
    return 0;
}

/**
 * Module cleanup function
 * Called when the module is unloaded
 */
static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello_driver: Cleaning up device driver\n");
    
    /* Destroy device node */
    device_destroy(hello_class, dev_num);
    
    /* Destroy device class */
    class_destroy(hello_class);
    
    /* Remove and unregister character device */
    cdev_del(&hello_cdev);
    unregister_chrdev_region(dev_num, 1);
    
    /* Free kernel buffer */
    kfree(kernel_buffer);
    
    printk(KERN_INFO "hello_driver: Device driver cleaned up successfully\n");
}

module_init(hello_init);
module_exit(hello_exit);
