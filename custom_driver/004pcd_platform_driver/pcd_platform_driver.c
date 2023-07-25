#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
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
loff_t pcd_llseek (struct file *filp, loff_t off, int whence);
ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos);
ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
int pcd_open (struct inode *inode, struct file *filp);
int pcd_release (struct inode *inode, struct file *filp);
struct pcdrv_private_data pcdrv_data;
/* File operation */
struct file_operations pcd_fops =
{
        .open = pcd_open,
        .release = pcd_release,
        .llseek = pcd_llseek,
        .write = pcd_write,
        .read = pcd_read,
        .owner = THIS_MODULE
};

static int platform_pcdrv_probe(struct platform_device *platform_pcdev);

static int platform_pcdrv_remove(struct platform_device *platform_pcdev);

struct platform_driver platform_pcdrv = {
	.probe = platform_pcdrv_probe,
	.remove = platform_pcdrv_remove,
	.driver = {
		.name = "pcd-char-device",

	}
};

/* Gets call when matched platform device is found */
static int platform_pcdrv_probe(struct platform_device *platform_pcdev)
{
	int ret;
	struct platform_device_data *pdata;
	struct pcdev_private_data *dev_data;

	pr_info("Device is detected\n");
        /* Get the platform data */
	/*pdata = (struct platform_device_data*)platform_pcdev->dev.platform_data;*/
	pdata = (struct platform_device_data*)dev_get_platdata(&platform_pcdev->dev);
	if(!pdata){
		pr_info("No platform data available\n");
		ret = -EINVAL;
		goto out;
		
	}
	/* Dynamically allocate memory for the device private data */
/*	dev_data = kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL); */
	dev_data = devm_kzalloc(&platform_pcdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
	if(!dev_data){
		pr_info("Cannot allocate memory\n");
/*		ret = -ENOMEM;
*		goto out; */
		return -ENOMEM;
	}
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	pr_info("Device serial: %s\n", dev_data->pdata.serial_number);
	pr_info("Device size: %d\n", dev_data->pdata.size);
	pr_info("Device permission: %d\n", dev_data->pdata.perm);
	/* Dynamically allocate memory for the device buffer using size information
	 * from the platform data */
	/* dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL); */
	dev_data->buffer = devm_kzalloc(&platform_pcdev->dev, dev_data->pdata.size, GFP_KERNEL);
	if(!(dev_data->buffer)){
		pr_info("Cannot allocate buffer's memory\n");
		ret = -ENOMEM;
		goto dev_data_free;
	}
	/* Get the device number */
	dev_data->device_number = pcdrv_data.device_number_base + platform_pcdev->id;
	/* Do cdev init and cdev_add*/
	cdev_init(&dev_data->cdev, &pcd_fops);
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->device_number, 1);
	if(ret < 0){
		pr_info("Cannot add a char device to the system\n");
		goto buffer_memory_free;
	}
	/* Create device file for detected platform device */
	pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, dev_data->device_number,\
			NULL, "pcd-%d", platform_pcdev->id);
	if(IS_ERR(pcdrv_data.pcd_device)){
		pr_info("Cannot create device file\n");
		ret = PTR_ERR(pcdrv_data.pcd_class);
		goto cdev_del;
	}

	/* Store pcdev_private_data to platform_pcdev->dev.driver_data
	 * It will be used in remove function */
	/* platform_pcdev->dev.driver_data = dev_data; */
	platform_set_drvdata(platform_pcdev, dev_data);
	
	pcdrv_data.total_devices++;
	pr_info("The probe was successful\n");
	return 0;
	/* Error handling */
cdev_del:
	cdev_del(&dev_data->cdev);
buffer_memory_free:
	devm_kfree(&platform_pcdev->dev, dev_data->buffer);
dev_data_free:
	devm_kfree(&platform_pcdev->dev, dev_data);
out:
	return ret;
}

/* Gets call when the device is removed from the system */
static int platform_pcdrv_remove(struct platform_device *platform_pcdev)
{
	struct pcdev_private_data *dev_data = (struct pcdev_private_data*)platform_pcdev->dev.driver_data;
	/* Destroy device was created with device_created() */
	device_destroy(pcdrv_data.pcd_class, dev_data->device_number);
	/* Remove a cdev entry from the system */
	cdev_del(&dev_data->cdev);

	/* free memory held by the device */
/*	kfree(dev_data->buffer);
*	kfree(dev_data);
*/
	pcdrv_data.total_devices--;

	pr_info("Device unpluged\n");
        return 0;
}

static int check_permission(int dev_perm, int acc_mode){
	if(dev_perm == RDWR){
		return 0;
	}
	if((dev_perm == RDONLY) && (acc_mode & FMODE_READ) && (!(acc_mode & FMODE_WRITE))){
		return 0;
	}
	if((dev_perm == WRONLY) && (!(acc_mode & FMODE_READ)) && (acc_mode & FMODE_WRITE)){
		return 0;
	}
	return -EPERM;
}

loff_t pcd_llseek (struct file *filp, loff_t off, int whence)
{
	struct pcdev_private_data *cdev_data = (struct pcdev_private_data*)filp->private_data;
	int max_size;
	loff_t temp;

	max_size = cdev_data->pdata.size;
	switch(whence){
		case SEEK_SET:
			if((off > max_size) || (off < 0)){
				return -EINVAL;
			}
			filp->f_pos = off;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > max_size) || (temp < 0)){
				return -EINVAL;
			}
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + off;
			if((temp > max_size) || (temp < 0)){
				return -EINVAL;
			}
			filp->f_pos = temp;
			break;
	}

	return filp->f_pos;
}

ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	int max_size;
	struct pcdev_private_data *cdev_data = (struct pcdev_private_data*)filp->private_data;

	max_size = cdev_data->pdata.size;
	if((*f_pos + count) > max_size){
		count = max_size - *f_pos;
	}
	if(copy_to_user(buff, cdev_data->buffer, count)){
		return -EFAULT;
	}
	*f_pos += count;

	return count;
}

ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	int max_size;
	struct pcdev_private_data *cdev_data = (struct pcdev_private_data*)filp->private_data;

	max_size = cdev_data->pdata.size;
	if((*f_pos + count) > max_size){
		count = max_size - *f_pos;
	}
	if(!count){
		return -ENOMEM;
	}
	if(copy_from_user(cdev_data->buffer, buff, count)){
		return -EFAULT;
	}
	*f_pos += count;

	return count;
}

int pcd_open (struct inode *inode, struct file *filp)
{
	struct pcdev_private_data *cdev_data;
	int ret = 0;
	int minor;

	minor = MINOR(inode->i_rdev);
	pr_info("Minor: %d\n", minor);
	cdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	filp->private_data = cdev_data;
	ret = check_permission(cdev_data->pdata.perm, filp->f_mode);
	(!ret)?pr_info("Open file successfully\n"):pr_info("Open file unsuccessfully\n");
	
	return 0;
}

int pcd_release (struct inode *inode, struct file *filp)
{
	pr_info("release requested\n");
	return 0;
}

static int __init pcd_platform_driver_init(void)
{
	int ret;
	/* Dynamically allocate a device number of MAX_DEVICES */
	ret = alloc_chrdev_region(&pcdrv_data.device_number_base, 0, MAX_DEVICES, "Range of devices");
	if(ret < 0){
		pr_err("Cannot allocate chrdev\n");
		return ret;
	}
	/* Create device class under /sys/class */
	pcdrv_data.pcd_class = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.pcd_class)){
		pr_err("Cannot create device class\n");
		unregister_chrdev_region(pcdrv_data.device_number_base, MAX_DEVICES);
		return PTR_ERR(pcdrv_data.pcd_class);
	}
	/* Register platform driver */
	platform_driver_register(&platform_pcdrv);
	pr_info("Insert driver\n");

	return 0;
}

static void __exit pcd_platform_driver_exit(void)
{
	/* Unregister platform driver */
	platform_driver_unregister(&platform_pcdrv);
	/* Class destroy */
	class_destroy(pcdrv_data.pcd_class);
	/* Unregister device number of MAX_NUMBER */
	unregister_chrdev_region(pcdrv_data.device_number_base, MAX_DEVICES);
	pr_info("Remove driver\n");
}

module_init(pcd_platform_driver_init);
module_exit(pcd_platform_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUYEN DO VAN");
MODULE_DESCRIPTION("EXAMPLE CHARACTER DEVICE DRIVER");
