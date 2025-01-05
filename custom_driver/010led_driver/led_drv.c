#include <linux/module.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/leds.h>

static struct led_classdev led[3];
const char* name[] = {"led0", "led1", "led2"};
/* static int data[3] = {0,1,2}; */

ssize_t test_show(struct device *dev, struct device_attribute *attr, char *buf){
    /* int* data;

    data = (int*)dev_get_drvdata(dev); */
    return sprintf(buf, "%s\n", "show");
}

ssize_t test_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count){
/*     int i;
    int ret;
    int *data = (int*)dev_get_drvdata(dev);

    ret = kstrtol(buf, 10, (long*)(&i));
    if (ret)
        return ret;
    *data = i;
     */
    pr_info("store\n");
    return count;
}

static DEVICE_ATTR_RW(test);

static struct attribute* gpio_attrs[] = {
	&dev_attr_test.attr,
	NULL
};

static struct attribute_group gpio_attr_group = {
	.attrs = gpio_attrs
};

static const struct attribute_group* gpio_attr_groups[] = {
	&gpio_attr_group,
	NULL
};

static int set_brightness(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	int err = 0;
	
    pr_info("set brightness\n");
	return err;
}

static int set_blink(struct led_classdev *led_cdev,
	unsigned long *delay_on, unsigned long *delay_off)
{
	int err = 0;
	
    pr_info("set blink\n");
	return err;
}


/*-------------------------------------------------------------------------*/
static int __init led_init(void)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < 3; i++) {
        led[i].name = name[i];
        led[i].default_trigger = NULL;
        led[i].brightness = LED_OFF;
        led[i].brightness_set_blocking = set_brightness;
        led[i].blink_set = set_blink;
        led[i].groups = gpio_attr_groups;
        ret = led_classdev_register(NULL,&led[i]);
        if (ret < 0) {
            pr_info("huyendv: couldn't register LED %s\n", led[i].name);
            goto exit;
        }
        /* dev_set_drvdata(led[i].dev, &data[i]); */
    }
    return ret;

exit:
    while (--i >= 0) {
        led_classdev_unregister(&led[i]);
    }
    return ret;
}

static void __exit led_exit(void)
{
    int i = 3;
    while (--i >= 0) {
        led_classdev_unregister(&led[i]);
    }
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huyen Do Van");