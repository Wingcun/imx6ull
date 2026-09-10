#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>

struct platform_probe_data {
    int led_gpio;
    bool led_active_low;
    int led_state;

    struct mutex lock;
    struct miscdevice miscdev;
};

static const struct of_device_id platform_probe_of_match[] = {
    {
        .compatible = "training,imx6ull-platform-probe",
    },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, platform_probe_of_match);

static void platform_led_set(
    struct platform_probe_data *data,
    int state)
{
    int gpio_value;

    gpio_value = state;

    if (data->led_active_low)
        gpio_value = !gpio_value;

    gpio_set_value(data->led_gpio, gpio_value);
    data->led_state = state;
}

static ssize_t platform_led_read(
    struct file *file,
    char __user *buffer,
    size_t count,
    loff_t *offset)
{
    struct miscdevice *miscdev;
    struct platform_probe_data *data;
    char result[2];

    if (*offset != 0)
        return 0;

    if (count < sizeof(result))
        return -EINVAL;

    miscdev = file->private_data;

    data = container_of(
        miscdev,
        struct platform_probe_data,
        miscdev);

    mutex_lock(&data->lock);
    result[0] = data->led_state ? '1' : '0';
    mutex_unlock(&data->lock);

    result[1] = '\n';

    if (copy_to_user(buffer, result, sizeof(result)))
        return -EFAULT;

    *offset = sizeof(result);
    return sizeof(result);
}

static ssize_t platform_led_write(
    struct file *file,
    const char __user *buffer,
    size_t count,
    loff_t *offset){
    struct miscdevice *miscdev;
    struct platform_probe_data *data;
    char value;
    int state;

    if (count == 0)
        return -EINVAL;

    if (copy_from_user(&value, buffer, 1))
        return -EFAULT;

    if (value == '0')
        state = 0;
    else if (value == '1')
        state = 1;
    else
        return -EINVAL;

    miscdev = file->private_data;
    data = container_of(
        miscdev,
        struct platform_probe_data,
        miscdev);

    mutex_lock(&data->lock);
    platform_led_set(data, state);
    mutex_unlock(&data->lock);

    pr_info(
        "platform_probe: led=%s\n",
        state ? "on" : "off");

    return count;
}

static const struct file_operations platform_led_fops = {
    .owner = THIS_MODULE,
    .read = platform_led_read,
    .write = platform_led_write,
    .llseek = no_llseek,
};

static int platform_probe_probe(struct platform_device *pdev)
{
    struct platform_probe_data *data;
    enum of_gpio_flags flags;
    int ret;

    data = devm_kzalloc(
        &pdev->dev,
        sizeof(*data),
        GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    flags = 0;

    data->led_gpio = of_get_named_gpio_flags(
        pdev->dev.of_node,
        "led-gpios",
        0,
        &flags);

    if (!gpio_is_valid(data->led_gpio)) {
        pr_err(
            "platform_probe: invalid led gpio=%d\n",
            data->led_gpio);
        return data->led_gpio < 0 ? data->led_gpio : -EINVAL;
    }

    data->led_active_low =
        (flags & OF_GPIO_ACTIVE_LOW) != 0;

    /*
     * 应用层状态约定：
     * led_state=0 表示灭，led_state=1 表示亮。
     */
    data->led_state = 0;

    ret = gpio_request(
        data->led_gpio,
        "platform_probe_led");
    if (ret) {
        pr_err(
            "platform_probe: gpio_request failed=%d\n",
            ret);
        return ret;
    }

    /*
     * 初始化为灭：
     * 低电平有效的 LED 需要输出高电平。
     */
    ret = gpio_direction_output(
        data->led_gpio,
        data->led_active_low ? 1 : 0);
    if (ret) {
        pr_err(
            "platform_probe: gpio_direction_output failed=%d\n",
            ret);
        gpio_free(data->led_gpio);
        return ret;
    }

    mutex_init(&data->lock);

    data->miscdev.minor = MISC_DYNAMIC_MINOR;
    data->miscdev.name = "imx6ull_device";
    data->miscdev.fops = &platform_led_fops;
    data->miscdev.mode = 0666;

    ret = misc_register(&data->miscdev);
    if (ret) {
        pr_err(
            "platform_probe: misc_register failed=%d\n",
            ret);

        gpio_set_value(
            data->led_gpio,
            data->led_active_low ? 1 : 0);

        gpio_free(data->led_gpio);
        return ret;
    }

    platform_set_drvdata(pdev, data);

    pr_info(
        "platform_probe: probe successful, led_gpio=%d, active_low=%d\n",
        data->led_gpio,
        data->led_active_low);

    return 0;
}

static int platform_probe_remove(struct platform_device *pdev)
{
    struct platform_probe_data *data;

    data = platform_get_drvdata(pdev);

    if (data && gpio_is_valid(data->led_gpio)) {
        /* 卸载驱动前将 LED 熄灭。 */
        gpio_set_value(
            data->led_gpio,
            data->led_active_low ? 1 : 0);

        gpio_free(data->led_gpio);
    }

    pr_info("platform_probe: remove successful\n");
    return 0;
}

static struct platform_driver platform_probe_driver = {
    .probe = platform_probe_probe,
    .remove = platform_probe_remove,
    .driver = {
        .name = "platform_probe",
        .of_match_table = platform_probe_of_match,
        .owner = THIS_MODULE,
    },
};

static int __init platform_probe_init(void)
{
    return platform_driver_register(&platform_probe_driver);
}

static void __exit platform_probe_exit(void)
{
    platform_driver_unregister(&platform_probe_driver);
}

module_init(platform_probe_init);
module_exit(platform_probe_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wingcun");
MODULE_DESCRIPTION("i.MX6ULL platform device probe test driver");
