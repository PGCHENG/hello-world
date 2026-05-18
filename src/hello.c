#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define DEVICE_NAME "hello"
#define BUFFER_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PGCHENG");
MODULE_DESCRIPTION("A simple Linux character device driver");

static dev_t device_number;
static struct cdev hello_cdev;
static struct class *hello_class = NULL;
static char device_buffer[BUFFER_SIZE] = {0};

/* Function prototypes */
static int device_open(struct inode *inode, struct file *file);
static int device_release(struct inode *inode, struct file *file);
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset);
static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset);

/* File operations structure */
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write,
};

/**
 * device_open - Open the device
 */
static int device_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "hello: Device opened\n");
    return 0;
}

/**
 * device_release - Release/Close the device
 */
static int device_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "hello: Device released\n");
    return 0;
}

/**
 * device_read - Read from the device
 * Returns "Hello {string}" where string is the previously written data
 */
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset)
{
    char output_buffer[BUFFER_SIZE];
    size_t output_len;
    int ret;

    /* Build the output string: "Hello {device_buffer}" */
    memset(output_buffer, 0, BUFFER_SIZE);
    
    if (strlen(device_buffer) == 0) {
        snprintf(output_buffer, BUFFER_SIZE, "Hello\n");
    } else {
        snprintf(output_buffer, BUFFER_SIZE, "Hello %s\n", device_buffer);
    }

    output_len = strlen(output_buffer);

    /* Check if offset is out of range */
    if (*offset >= output_len) {
        return 0;
    }

    /* Adjust count if necessary */
    if (*offset + count > output_len) {
        count = output_len - *offset;
    }

    /* Copy data to user space using copy_to_user */
    ret = copy_to_user(user_buffer, output_buffer + *offset, count);
    if (ret != 0) {
        printk(KERN_ERR "hello: Failed to copy data to user space\n");
        return -EFAULT;
    }

    *offset += count;
    printk(KERN_INFO "hello: Read %zu bytes\n", count);
    return count;
}

/**
 * device_write - Write to the device
 * Stores the data sent by the user for later read operations
 */
static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset)
{
    size_t write_len;
    int ret;

    /* Limit the write size to avoid buffer overflow */
    if (count > BUFFER_SIZE - 1) {
        write_len = BUFFER_SIZE - 1;
    } else {
        write_len = count;
    }

    /* Clear the buffer first */
    memset(device_buffer, 0, BUFFER_SIZE);

    /* Copy data from user space using copy_from_user */
    ret = copy_from_user(device_buffer, user_buffer, write_len);
    if (ret != 0) {
        printk(KERN_ERR "hello: Failed to copy data from user space\n");
        return -EFAULT;
    }

    /* Remove trailing newline if present */
    if (write_len > 0 && device_buffer[write_len - 1] == '\n') {
        device_buffer[write_len - 1] = '\0';
        write_len--;
    } else {
        device_buffer[write_len] = '\0';
    }

    printk(KERN_INFO "hello: Wrote %zu bytes: %s\n", write_len, device_buffer);
    return write_len;
}

/**
 * hello_init - Module initialization
 */
static int __init hello_init(void)
{
    int ret;
    struct device *device;

    printk(KERN_INFO "hello: Initializing module\n");

    /* Allocate character device region dynamically */
    ret = alloc_chrdev_region(&device_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ERR "hello: Failed to allocate device number\n");
        return ret;
    }

    printk(KERN_INFO "hello: Allocated device number - Major: %d, Minor: %d\n",
           MAJOR(device_number), MINOR(device_number));

    /* Create device class */
    hello_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(hello_class)) {
        printk(KERN_ERR "hello: Failed to create device class\n");
        unregister_chrdev_region(device_number, 1);
        return PTR_ERR(hello_class);
    }

    /* Initialize and add the character device */
    cdev_init(&hello_cdev, &fops);
    hello_cdev.owner = THIS_MODULE;
    ret = cdev_add(&hello_cdev, device_number, 1);
    if (ret < 0) {
        printk(KERN_ERR "hello: Failed to add character device\n");
        class_destroy(hello_class);
        unregister_chrdev_region(device_number, 1);
        return ret;
    }

    /* Create device file in /dev/ */
    device = device_create(hello_class, NULL, device_number, NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        printk(KERN_ERR "hello: Failed to create device file\n");
        cdev_del(&hello_cdev);
        class_destroy(hello_class);
        unregister_chrdev_region(device_number, 1);
        return PTR_ERR(device);
    }

    printk(KERN_INFO "hello: Device /dev/%s created successfully\n", DEVICE_NAME);
    return 0;
}

/**
 * hello_exit - Module exit
 */
static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello: Exiting module\n");

    /* Remove the device file */
    device_destroy(hello_class, device_number);

    /* Remove the character device */
    cdev_del(&hello_cdev);

    /* Destroy the device class */
    class_destroy(hello_class);

    /* Unregister the device number */
    unregister_chrdev_region(device_number, 1);

    printk(KERN_INFO "hello: Module exited\n");
}

module_init(hello_init);
module_exit(hello_exit);
