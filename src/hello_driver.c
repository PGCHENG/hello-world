#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define DEVICE_NAME "hello"
#define BUFFER_SIZE 256

/* Device data structure */
struct hello_device {
    dev_t devno;              /* Device number */
    struct cdev cdev;         /* Character device */
    char buffer[BUFFER_SIZE]; /* Data buffer */
    int buffer_size;          /* Size of data in buffer */
};

static struct hello_device *hello_dev = NULL;

/* Open operation */
static int hello_open(struct inode *inode, struct file *file)
{
    struct hello_device *dev;
    
    dev = container_of(inode->i_cdev, struct hello_device, cdev);
    file->private_data = dev;
    
    printk(KERN_INFO "hello: device opened\n");
    return 0;
}

/* Release operation */
static int hello_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "hello: device released\n");
    return 0;
}

/* Write operation */
static ssize_t hello_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos)
{
    struct hello_device *dev = file->private_data;
    size_t to_copy;
    
    /* Prevent buffer overflow */
    to_copy = count < BUFFER_SIZE ? count : BUFFER_SIZE - 1;
    
    /* Copy data from user space */
    if (copy_from_user(dev->buffer, buf, to_copy)) {
        printk(KERN_ERR "hello: failed to copy data from user\n");
        return -EFAULT;
    }
    
    dev->buffer[to_copy] = '\0';
    dev->buffer_size = to_copy;
    
    printk(KERN_INFO "hello: received data: %s\n", dev->buffer);
    
    return to_copy;
}

/* Read operation */
static ssize_t hello_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct hello_device *dev = file->private_data;
    char output_buffer[BUFFER_SIZE];
    int output_len;
    
    /* Format output as "Hello {string}" */
    output_len = snprintf(output_buffer, sizeof(output_buffer),
                         "Hello %s", dev->buffer);
    
    /* Prevent reading beyond the formatted string */
    if (count < output_len)
        output_len = count;
    
    /* Copy data to user space */
    if (copy_to_user(buf, output_buffer, output_len)) {
        printk(KERN_ERR "hello: failed to copy data to user\n");
        return -EFAULT;
    }
    
    printk(KERN_INFO "hello: sent data: %s\n", output_buffer);
    
    return output_len;
}

/* File operations structure */
static const struct file_operations hello_fops = {
    .owner = THIS_MODULE,
    .open = hello_open,
    .release = hello_release,
    .read = hello_read,
    .write = hello_write,
};

/* Module initialization */
static int __init hello_init(void)
{
    int result;
    struct device *dev_class;
    static struct class *hello_class = NULL;
    
    printk(KERN_INFO "hello: initializing character device driver\n");
    
    /* Allocate device structure */
    hello_dev = kmalloc(sizeof(struct hello_device), GFP_KERNEL);
    if (!hello_dev) {
        printk(KERN_ERR "hello: failed to allocate device structure\n");
        return -ENOMEM;
    }
    
    /* Dynamically allocate device number */
    result = alloc_chrdev_region(&hello_dev->devno, 0, 1, DEVICE_NAME);
    if (result < 0) {
        printk(KERN_ERR "hello: failed to allocate device number\n");
        kfree(hello_dev);
        return result;
    }
    
    printk(KERN_INFO "hello: allocated device number %d:%d\n",
           MAJOR(hello_dev->devno), MINOR(hello_dev->devno));
    
    /* Initialize character device */
    cdev_init(&hello_dev->cdev, &hello_fops);
    hello_dev->cdev.owner = THIS_MODULE;
    
    /* Add character device */
    result = cdev_add(&hello_dev->cdev, hello_dev->devno, 1);
    if (result < 0) {
        printk(KERN_ERR "hello: failed to add character device\n");
        unregister_chrdev_region(hello_dev->devno, 1);
        kfree(hello_dev);
        return result;
    }
    
    /* Create device class */
    hello_class = class_create(THIS_MODULE, "hello_class");
    if (IS_ERR(hello_class)) {
        printk(KERN_ERR "hello: failed to create device class\n");
        cdev_del(&hello_dev->cdev);
        unregister_chrdev_region(hello_dev->devno, 1);
        kfree(hello_dev);
        return PTR_ERR(hello_class);
    }
    
    /* Create device file in /dev */
    dev_class = device_create(hello_class, NULL, hello_dev->devno, NULL, DEVICE_NAME);
    if (IS_ERR(dev_class)) {
        printk(KERN_ERR "hello: failed to create device file\n");
        class_destroy(hello_class);
        cdev_del(&hello_dev->cdev);
        unregister_chrdev_region(hello_dev->devno, 1);
        kfree(hello_dev);
        return PTR_ERR(dev_class);
    }
    
    /* Initialize buffer */
    memset(hello_dev->buffer, 0, BUFFER_SIZE);
    hello_dev->buffer_size = 0;
    
    printk(KERN_INFO "hello: device initialized successfully\n");
    return 0;
}

/* Module cleanup */
static void __exit hello_exit(void)
{
    printk(KERN_INFO "hello: cleaning up character device driver\n");
    
    if (hello_dev) {
        /* Remove device file from /dev */
        device_destroy(NULL, hello_dev->devno);
        
        /* Remove character device */
        cdev_del(&hello_dev->cdev);
        
        /* Deallocate device number */
        unregister_chrdev_region(hello_dev->devno, 1);
        
        /* Free device structure */
        kfree(hello_dev);
    }
    
    printk(KERN_INFO "hello: device cleanup complete\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("PGCHENG");
MODULE_DESCRIPTION("A simple Linux character device driver");
MODULE_VERSION("1.0");
