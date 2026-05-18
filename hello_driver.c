#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>

#define DEVICE_NAME "hello"
#define BUFFER_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PGCHENG");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("1.0");

static dev_t device_number;
static struct cdev hello_cdev;
static struct class *hello_class = NULL;
static struct device *hello_device = NULL;

/* Per-device data structure */
struct hello_device {
    char *buffer;
    size_t buffer_len;
};

static struct hello_device hello_dev;

/**
 * open - Open the character device
 * @inode: inode structure
 * @filp: file structure
 * Return: 0 on success, negative error code on failure
 */
static int hello_open(struct inode *inode, struct file *filp)
{
    pr_info("hello: device opened\n");
    
    /* Allocate buffer if not already allocated */
    if (hello_dev.buffer == NULL) {
        hello_dev.buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
        if (!hello_dev.buffer) {
            pr_err("hello: Failed to allocate buffer\n");
            return -ENOMEM;
        }
        memset(hello_dev.buffer, 0, BUFFER_SIZE);
        hello_dev.buffer_len = 0;
    }
    
    filp->private_data = &hello_dev;
    return 0;
}

/**
 * hello_read - Read from the character device
 * @filp: file structure
 * @buf: user space buffer
 * @count: number of bytes to read
 * @f_pos: file position (unused)
 * Return: number of bytes read, negative error code on failure
 */
static ssize_t hello_read(struct file *filp, char __user *buf,
                          size_t count, loff_t *f_pos)
{
    struct hello_device *dev = filp->private_data;
    char *output_buffer = NULL;
    size_t output_len;
    int ret = 0;
    
    if (!dev || !dev->buffer) {
        pr_err("hello: Device not properly initialized\n");
        return -EINVAL;
    }
    
    /* Allocate temporary buffer for the output */
    output_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!output_buffer) {
        pr_err("hello: Failed to allocate output buffer\n");
        return -ENOMEM;
    }
    
    /* Format the output message */
    if (dev->buffer_len > 0) {
        output_len = snprintf(output_buffer, BUFFER_SIZE, "Hello %s\n", dev->buffer);
    } else {
        output_len = snprintf(output_buffer, BUFFER_SIZE, "Hello World\n");
    }
    
    /* Limit output to requested count */
    if (output_len > count) {
        output_len = count;
    }
    
    /* Copy data to user space */
    if (copy_to_user(buf, output_buffer, output_len)) {
        pr_err("hello: Failed to copy data to user space\n");
        ret = -EFAULT;
        goto out;
    }
    
    pr_info("hello: Read %zu bytes\n", output_len);
    ret = output_len;
    
out:
    kfree(output_buffer);
    return ret;
}

/**
 * hello_write - Write to the character device
 * @filp: file structure
 * @buf: user space buffer
 * @count: number of bytes to write
 * @f_pos: file position (unused)
 * Return: number of bytes written, negative error code on failure
 */
static ssize_t hello_write(struct file *filp, const char __user *buf,
                           size_t count, loff_t *f_pos)
{
    struct hello_device *dev = filp->private_data;
    
    if (!dev || !dev->buffer) {
        pr_err("hello: Device not properly initialized\n");
        return -EINVAL;
    }
    
    /* Limit write size to buffer size - 1 (for null terminator) */
    if (count > BUFFER_SIZE - 1) {
        count = BUFFER_SIZE - 1;
    }
    
    /* Copy data from user space */
    if (copy_from_user(dev->buffer, buf, count)) {
        pr_err("hello: Failed to copy data from user space\n");
        return -EFAULT;
    }
    
    /* Null-terminate the string and remove trailing newline */
    dev->buffer[count] = '\0';
    dev->buffer_len = count;
    
    /* Remove trailing newline if present */
    if (dev->buffer_len > 0 && dev->buffer[dev->buffer_len - 1] == '\n') {
        dev->buffer[dev->buffer_len - 1] = '\0';
        dev->buffer_len--;
    }
    
    pr_info("hello: Wrote %zu bytes, buffer: '%s'\n", dev->buffer_len, dev->buffer);
    return count;
}

/**
 * hello_release - Close the character device
 * @inode: inode structure
 * @filp: file structure
 * Return: 0 on success
 */
static int hello_release(struct inode *inode, struct file *filp)
{
    pr_info("hello: device closed\n");
    return 0;
}

/* File operations structure */
static const struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .read = hello_read,
    .write = hello_write,
    .release = hello_release,
};

/**
 * hello_init - Module initialization function
 * Return: 0 on success, negative error code on failure
 */
static int __init hello_init(void)
{
    int ret = 0;
    
    pr_info("hello: Initializing character device driver\n");
    
    /* Allocate character device number dynamically */
    ret = alloc_chrdev_region(&device_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("hello: Failed to allocate character device number\n");
        return ret;
    }
    
    pr_info("hello: Allocated device number major=%d, minor=%d\n",
            MAJOR(device_number), MINOR(device_number));
    
    /* Initialize cdev structure */
    cdev_init(&hello_cdev, &hello_fops);
    hello_cdev.owner = THIS_MODULE;
    
    /* Register the character device */
    ret = cdev_add(&hello_cdev, device_number, 1);
    if (ret < 0) {
        pr_err("hello: Failed to add character device\n");
        unregister_chrdev_region(device_number, 1);
        return ret;
    }
    
    /* Create device class */
    hello_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(hello_class)) {
        pr_err("hello: Failed to create device class\n");
        cdev_del(&hello_cdev);
        unregister_chrdev_region(device_number, 1);
        return PTR_ERR(hello_class);
    }
    
    /* Create device node automatically */
    hello_device = device_create(hello_class, NULL, device_number, NULL, DEVICE_NAME);
    if (IS_ERR(hello_device)) {
        pr_err("hello: Failed to create device node\n");
        class_destroy(hello_class);
        cdev_del(&hello_cdev);
        unregister_chrdev_region(device_number, 1);
        return PTR_ERR(hello_device);
    }
    
    pr_info("hello: Character device driver loaded successfully\n");
    pr_info("hello: Device node created at /dev/%s\n", DEVICE_NAME);
    
    return 0;
}

/**
 * hello_exit - Module cleanup function
 */
static void __exit hello_exit(void)
{
    pr_info("hello: Cleaning up character device driver\n");
    
    /* Free kernel buffer */
    if (hello_dev.buffer) {
        kfree(hello_dev.buffer);
        hello_dev.buffer = NULL;
        hello_dev.buffer_len = 0;
    }
    
    /* Remove device node */
    if (hello_device) {
        device_destroy(hello_class, device_number);
    }
    
    /* Destroy device class */
    if (hello_class) {
        class_destroy(hello_class);
    }
    
    /* Unregister character device */
    cdev_del(&hello_cdev);
    
    /* Release allocated device number */
    unregister_chrdev_region(device_number, 1);
    
    pr_info("hello: Character device driver unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);