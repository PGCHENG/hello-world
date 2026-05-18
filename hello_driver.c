#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "hello"
#define CLASS_NAME "hello_class"
#define BUFFER_SIZE 256

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PGCHENG");
MODULE_DESCRIPTION("A simple Linux character device driver");
MODULE_VERSION("1.0");

static dev_t dev_num;
static struct cdev hello_cdev;
static struct class *hello_class = NULL;
static struct device *hello_device = NULL;

/* Per-device data structure */
struct hello_dev {
    char *buffer;
    size_t buffer_len;
};

static struct hello_dev *hello_device_data = NULL;

/**
 * open - Device open function
 * @inode: pointer to inode structure
 * @file: pointer to file structure
 *
 * Return: 0 on success, negative error code on failure
 */
static int hello_open(struct inode *inode, struct file *file)
{
    struct hello_dev *dev;

    dev = kmalloc(sizeof(struct hello_dev), GFP_KERNEL);
    if (!dev) {
        pr_err("hello_driver: Failed to allocate device data\n");
        return -ENOMEM;
    }

    dev->buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!dev->buffer) {
        pr_err("hello_driver: Failed to allocate buffer\n");
        kfree(dev);
        return -ENOMEM;
    }

    memset(dev->buffer, 0, BUFFER_SIZE);
    dev->buffer_len = 0;

    file->private_data = dev;
    pr_info("hello_driver: Device opened successfully\n");

    return 0;
}

/**
 * hello_read - Device read function
 * @file: pointer to file structure
 * @buf: user space buffer
 * @count: number of bytes to read
 * @ppos: file position
 *
 * Return: number of bytes read, or negative error code
 */
static ssize_t hello_read(struct file *file, char __user *buf,
                          size_t count, loff_t *ppos)
{
    struct hello_dev *dev = file->private_data;
    char *read_buffer;
    ssize_t read_len;
    int ret;

    if (!dev || !dev->buffer) {
        pr_err("hello_driver: Invalid device data\n");
        return -EINVAL;
    }

    /* Allocate temporary buffer for read operation */
    read_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!read_buffer) {
        pr_err("hello_driver: Failed to allocate read buffer\n");
        return -ENOMEM;
    }

    /* Format the response: "Hello {user_input}" */
    if (dev->buffer_len > 0) {
        ret = snprintf(read_buffer, BUFFER_SIZE, "Hello %s\n", dev->buffer);
        if (ret < 0) {
            kfree(read_buffer);
            return ret;
        }
        read_len = ret;
    } else {
        read_len = snprintf(read_buffer, BUFFER_SIZE, "Hello World\n");
    }

    /* Prevent reading more than available */
    if (count < read_len) {
        read_len = count;
    }

    /* Copy data from kernel space to user space */
    ret = copy_to_user(buf, read_buffer, read_len);
    if (ret != 0) {
        pr_err("hello_driver: Failed to copy data to user space\n");
        kfree(read_buffer);
        return -EFAULT;
    }

    kfree(read_buffer);
    pr_info("hello_driver: Read %ld bytes\n", read_len);

    return read_len;
}

/**
 * hello_write - Device write function
 * @file: pointer to file structure
 * @buf: user space buffer
 * @count: number of bytes to write
 * @ppos: file position
 *
 * Return: number of bytes written, or negative error code
 */
static ssize_t hello_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *ppos)
{
    struct hello_dev *dev = file->private_data;
    ssize_t write_len;
    int ret;

    if (!dev || !dev->buffer) {
        pr_err("hello_driver: Invalid device data\n");
        return -EINVAL;
    }

    /* Limit write size to buffer size - 1 for null terminator */
    if (count > BUFFER_SIZE - 1) {
        write_len = BUFFER_SIZE - 1;
    } else {
        write_len = count;
    }

    /* Copy data from user space to kernel space */
    ret = copy_from_user(dev->buffer, buf, write_len);
    if (ret != 0) {
        pr_err("hello_driver: Failed to copy data from user space\n");
        return -EFAULT;
    }

    /* Null-terminate the string and store length */
    dev->buffer[write_len] = '\0';
    dev->buffer_len = write_len;

    pr_info("hello_driver: Wrote %ld bytes: %s\n", write_len, dev->buffer);

    return write_len;
}

/**
 * hello_release - Device release function
 * @inode: pointer to inode structure
 * @file: pointer to file structure
 *
 * Return: 0 on success
 */
static int hello_release(struct inode *inode, struct file *file)
{
    struct hello_dev *dev = file->private_data;

    if (dev) {
        if (dev->buffer) {
            kfree(dev->buffer);
        }
        kfree(dev);
    }

    pr_info("hello_driver: Device released successfully\n");

    return 0;
}

/* File operations structure */
static struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .read = hello_read,
    .write = hello_write,
    .release = hello_release,
};

/**
 * hello_init - Module initialization function
 *
 * Return: 0 on success, negative error code on failure
 */
static int __init hello_init(void)
{
    int ret;

    pr_info("hello_driver: Initializing the hello device driver\n");

    /* Dynamically allocate device number */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("hello_driver: Failed to allocate device number\n");
        return ret;
    }

    pr_info("hello_driver: Allocated device number: major=%d, minor=%d\n",
            MAJOR(dev_num), MINOR(dev_num));

    /* Initialize cdev structure */
    cdev_init(&hello_cdev, &hello_fops);
    hello_cdev.owner = THIS_MODULE;

    /* Register the character device */
    ret = cdev_add(&hello_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("hello_driver: Failed to register character device\n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    pr_info("hello_driver: Character device registered successfully\n");

    /* Create device class */
    hello_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(hello_class)) {
        pr_err("hello_driver: Failed to create device class\n");
        cdev_del(&hello_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(hello_class);
    }

    pr_info("hello_driver: Device class created successfully\n");

    /* Create device node */
    hello_device = device_create(hello_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(hello_device)) {
        pr_err("hello_driver: Failed to create device node\n");
        class_destroy(hello_class);
        cdev_del(&hello_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(hello_device);
    }

    pr_info("hello_driver: Device node created at /dev/%s\n", DEVICE_NAME);
    pr_info("hello_driver: Module initialized successfully\n");

    return 0;
}

/**
 * hello_exit - Module exit function
 */
static void __exit hello_exit(void)
{
    pr_info("hello_driver: Cleaning up and unloading the module\n");

    /* Destroy device node */
    if (hello_device != NULL) {
        device_destroy(hello_class, dev_num);
    }

    /* Destroy device class */
    if (hello_class != NULL) {
        class_destroy(hello_class);
    }

    /* Unregister character device */
    cdev_del(&hello_cdev);

    /* Release device number */
    unregister_chrdev_region(dev_num, 1);

    pr_info("hello_driver: Module unloaded successfully\n");
}

module_init(hello_init);
module_exit(hello_exit);
