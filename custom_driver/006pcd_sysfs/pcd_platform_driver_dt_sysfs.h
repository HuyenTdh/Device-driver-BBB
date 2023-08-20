#ifndef PCD_PLATFORM_DT_SYSFS_H
#define PCD_PLATFORM_DT_SYSFS_H

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sysfs.h>
#include "platform.h"

#define MAX_DEVICES					4

struct pcdev_private_data{
	struct platform_device_data pdata;
	dev_t device_number;
	char *buffer;
	struct cdev cdev;
};

struct pcdrv_private_data{
	int total_devices;
	dev_t device_number_base;
	struct class *pcd_class;
	struct device *pcd_device;
};

enum pcdev_names{
	PCDEVA1X,
	PCDEVB1X,
	PCDEVC1X,
	PCDEVD1X
};
struct device_config{
	int item1;
	int item2;
};

loff_t pcd_llseek (struct file *filp, loff_t off, int whence);
ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int pcd_open (struct inode *inode, struct file *filp);
int pcd_release (struct inode *inode, struct file *filp);

#endif /* PCD_PLATFORM_DT_SYSFS_H */
