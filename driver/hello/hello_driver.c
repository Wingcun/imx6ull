#include <linux/init.h>
#include <linux/module.h>

static int __init hello_driver_init(void)
{
    pr_info("hello_driver: loaded\n");
    return 0;
}

static void __exit hello_driver_exit(void)
{
    pr_info("hello_driver: unloaded\n");
}

module_init(hello_driver_init);
module_exit(hello_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("wingcun");
MODULE_DESCRIPTION("i.MX6ULL module environment test");