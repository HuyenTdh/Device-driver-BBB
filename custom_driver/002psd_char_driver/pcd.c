#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#define DEV_MEM_SIZE			512

/* pseudo device's memory */
char device_buffer[DEV_MEM_SIZE];
/* This hold the device number */
dev_t device_number;
/* cdev variable (info of character device) */
struct cdev pcd_cdev;

loff_t pcd_llseek (struct file *filp, loff_t off, int whence)
{
	loff_t temp;
	pr_info("llseek requested\n");
	pr_info("Current file position: %lld\n", filp->f_pos);
	switch(whence){
		case SEEK_SET:
			if((off > DEV_MEM_SIZE) || (off < 0))
				return -EINVAL;
			filp->f_pos = off;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > DEV_MEM_SIZE) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = DEV_MEM_SIZE + off;
                        if((temp > DEV_MEM_SIZE) || (temp < 0))
                                return -EINVAL;
			filp->f_pos = temp;
			break;
		default:
			return -EINVAL;
	}
	pr_info("Current file position: %lld\n", filp->f_pos);
	return filp->f_pos;
}

ssize_t pcd_read (struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("Number of bytes is requested: %zu\n", count);
	pr_info("Current file position: %lld\n", *f_pos);
	/* Adjust the 'count' */
	if((*f_pos + count) > DEV_MEM_SIZE){
		count = DEV_MEM_SIZE - *f_pos;
	}
	/* Copy data to user space */
	if(copy_to_user(buff, device_buffer + *f_pos, count)){
		return -EFAULT;
	}
	/* Update current file position */
	*f_pos += count;
	pr_info("Number of bytes is read: %zu\n", count);
	pr_info("Current file position: %lld\n", *f_pos);
	/* Return number of bytes which have been successfully read */
	return count;
}

ssize_t pcd_write (struct file *filp, const char __user *buff, size_t count, loff_t *f_pos)
{
	pr_info("Number of bytes is requested: %zu\n", count);
        pr_info("Current file position: %lld\n", *f_pos);
        /* Adjust the 'count' */
        if((*f_pos + count) > DEV_MEM_SIZE){
                count = DEV_MEM_SIZE - *f_pos;
        }
	if(!count){
		pr_err("No space left on the device\n");
		return -ENOMEM;
	}
        /* Copy data from user space */
        if(copy_from_user(device_buffer + *f_pos, buff, count)){
                return -EFAULT;
        }
        /* Update current file position */
        *f_pos += count;
        pr_info("Number of bytes is written: %zu\n", count);
        pr_info("Current file position: %lld\n", *f_pos);
	/* Number of bytes which have been successfully writen */
	return count;
}

int pcd_open (struct inode *inode, struct file *filp)
{
	pr_info("open requested\n");
	return 0;
}

int pcd_release (struct inode *inode, struct file *filp)
{
	pr_info("release requested\n");
	return 0;
}

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

struct class *class_pcd;
struct device *pcd_device;

static int __init pcd_init(void)
{
	int ret;
	/* Dynamically allocate a device number */
	ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");
	if(ret < 0){
		goto out;
	}
	pr_info("%s : Device number <major>:<minor> = %d:%d\n", __func__, MAJOR(device_number), MINOR(device_number));
	/* Create device file with fops */
	cdev_init(&pcd_cdev, &pcd_fops);
	/* Init devive owner */
	pcd_cdev.owner = THIS_MODULE;
	/* Register device with the VFS */
	ret = cdev_add(&pcd_cdev, device_number, 1);
	if(ret < 0){
		goto unreg_chrdev;
	}
	/* Create device class /sys/class/ */
	class_pcd = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(class_pcd)){
		ret = PTR_ERR(class_pcd);
		goto cdev_del;
	}
	/* Populate the sysfs with device information */
	pcd_device = device_create(class_pcd, NULL, device_number, NULL, "pcd");
	if(IS_ERR(pcd_device)){
                ret = PTR_ERR(pcd_device);
                goto class_destroy;
        }

	pr_info("Module was initialized successfully\n");

	return 0;

class_destroy:
	class_destroy(class_pcd);
cdev_del:
	cdev_del(&pcd_cdev);
unreg_chrdev:
	unregister_chrdev_region(device_number, 1);
out:
	return ret;
}

static void __exit pcd_exit(void)
{
	device_destroy(class_pcd, device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number, 1);
	pr_info("Module unloaded\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUYEN DO VAN");
MODULE_DESCRIPTION("EXAMPLE CHARACTER DEVICE DRIVER");
