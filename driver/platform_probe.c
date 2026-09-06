#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

static const struct of_device_id platform_probe_of_match[] = {
    {
        .compatible = "training,imx6ull-platform-probe",
    },
    { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, platform_probe_of_match);

static int platform_probe_probe(struct platform_device *pdev)
{
    pr_info("platform_probe: probe successful\n");
    return 0;
}

static int platform_probe_remove(struct platform_device *pdev)
{
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
