#include <linux/module.h>
#include <linux/platform_device.h>
#include "platform.h"

static void device_release(struct device *dev);

struct platform_device_data platform_pcdev_data[] = {
	[0] = {.size = 512, .perm = RDWR, .serial_number = "PCDEV0"},
	[1] = {.size = 1024, .perm = RDWR, .serial_number = "PCDEV1"},
	[2] = {.size = 128, .perm = RDONLY, .serial_number = "PCDEV2"},
        [3] = {.size = 32, .perm = WRONLY, .serial_number = "PCDEV3"}
};

struct platform_device platform_pcdev1 = {
	.name = "pcdev-A1x",
	.id = 0,
	.dev = {
		.platform_data = &platform_pcdev_data[0],
		.release = device_release
	}
};
struct platform_device platform_pcdev2 = {
	.name = "pcdev-B1x",
	.id = 1,
	.dev = {
		.platform_data = &platform_pcdev_data[1],
		.release = device_release
	}
};
struct platform_device platform_pcdev3 = {
        .name = "pcdev-C1x",
        .id = 2,
        .dev = {
                .platform_data = &platform_pcdev_data[2],
                .release = device_release
        }
};
struct platform_device platform_pcdev4 = {
        .name = "pcdev-D1x",
        .id = 3,
        .dev = {
                .platform_data = &platform_pcdev_data[3],
                .release = device_release
        }
};

struct platform_device* platform_pcdevs[] ={
	&platform_pcdev1,
	&platform_pcdev2,
	&platform_pcdev3,
	&platform_pcdev4
};

static void device_release(struct device *dev)
{
	pr_info("Device release\n");
}

static int __init pcdev_platform_init(void)
{
//	platform_device_register(&platform_pcdev1);
//	platform_device_register(&platform_pcdev2);
	platform_add_devices(platform_pcdevs, ARRAY_SIZE(platform_pcdevs));
	pr_info("Insert devices successfully\n");
	return 0;
}

static void __exit pcdev_platform_exit(void)
{
	platform_device_unregister(&platform_pcdev1);
	platform_device_unregister(&platform_pcdev2);
	platform_device_unregister(&platform_pcdev3);
        platform_device_unregister(&platform_pcdev4);
	pr_info("Remove devices successfully\n");
}

module_init(pcdev_platform_init);
module_exit(pcdev_platform_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huyen Do Van");
MODULE_DESCRIPTION("Platform devices");
