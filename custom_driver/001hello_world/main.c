#include <linux/module.h>

static int __init helloworld_init(void)
{
	pr_info("Hello world\n");
	return 0;
}

static void __exit helloworld_exit(void)
{
	pr_info("Goodbye module\n");
}

module_init(helloworld_init);
module_exit(helloworld_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huyen Do Van");
MODULE_DESCRIPTION("Hello world driver");
