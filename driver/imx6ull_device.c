#include <linux/fs.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "imx6ull_device_uapi.h"

static int device_state;
static bool data_changed;

static DEFINE_MUTEX(device_lock);
static DECLARE_WAIT_QUEUE_HEAD(device_waitq);

static int device_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t device_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *offset)
{
    char value;
    int state;

    if (*offset != 0) {
        return 0;
    }

    if (count < 2) {
        return -EINVAL;
    }

    mutex_lock(&device_lock);
    state = device_state;
    data_changed = false;
    mutex_unlock(&device_lock);

    value = state ? '1' : '0';

    if (copy_to_user(buffer, &value, 1)) {
        return -EFAULT;
    }

    if (copy_to_user(buffer + 1, "\n", 1)) {
        return -EFAULT;
    }

    *offset = 2;
    return 2;
}

static ssize_t device_write(
    struct file *file,
    const char __user *buffer,
    size_t count,
    loff_t *offset)
{
    char value;
    int new_state;

    if (count == 0) {
        return -EINVAL;
    }

    if (copy_from_user(&value, buffer, 1)) {
        return -EFAULT;
    }

    if (value == '0') {
        new_state = 0;
    } else if (value == '1') {
        new_state = 1;
    } else {
        return -EINVAL;
    }

    mutex_lock(&device_lock);
    device_state = new_state;
    data_changed = true;
    mutex_unlock(&device_lock);

    wake_up_interruptible(&device_waitq);

    return count;
}

static long device_ioctl(
    struct file *file,
    unsigned int command,
    unsigned long argument)
{
    int value;

    switch (command) {
    case IMX6ULL_DEVICE_GET:
        mutex_lock(&device_lock);
        value = device_state;
        mutex_unlock(&device_lock);

        if (copy_to_user(
                (int __user *)argument,
                &value,
                sizeof(value))) {
            return -EFAULT;
        }

        return 0;

    case IMX6ULL_DEVICE_SET:
        if (copy_from_user(
                &value,
                (int __user *)argument,
                sizeof(value))) {
            return -EFAULT;
        }

        if (value != 0 && value != 1) {
            return -EINVAL;
        }

        mutex_lock(&device_lock);
        device_state = value;
        data_changed = true;
        mutex_unlock(&device_lock);

        wake_up_interruptible(&device_waitq);
        return 0;

    default:
        return -ENOTTY;
    }
}

static unsigned int device_poll(
    struct file *file,
    poll_table *wait)
{
    unsigned int mask = 0;

    poll_wait(file, &device_waitq, wait);

    mutex_lock(&device_lock);

    if (data_changed) {
        mask |= POLLIN | POLLRDNORM;
    }

    mutex_unlock(&device_lock);

    return mask;
}

static const struct file_operations device_fops = {
    .owner = THIS_MODULE,
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .unlocked_ioctl = device_ioctl,
    .poll = device_poll,
    .llseek = no_llseek,
};

static struct miscdevice device_misc = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "imx6ull_device",
    .fops = &device_fops,
    .mode = 0666,
};

static int __init device_init(void)
{
    int ret;

    ret = misc_register(&device_misc);
    if (ret) {
        pr_err("imx6ull_device: misc_register failed: %d\n", ret);
        return ret;
    }

    pr_info("imx6ull_device: registered\n");
    return 0;
}

static void __exit device_exit(void)
{
    misc_deregister(&device_misc);
    pr_info("imx6ull_device: unregistered\n");
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wingcun");
MODULE_DESCRIPTION("i.MX6ULL character device API training driver");
