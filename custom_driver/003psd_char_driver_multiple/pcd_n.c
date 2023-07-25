#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>
#include <linux/err.h>

#define RDONLY					0x01
#define WRONLY					0x10
#define RDWR					0x11

#define NO_OF_DEVICES				4
#define DEV_MEM_SIZE_PCDEV0			1024
#define DEV_MEM_SIZE_PCDEV1                     512
#define DEV_MEM_SIZE_PCDEV2                     1024
#define DEV_MEM_SIZE_PCDEV3                     512

/* pseudo device's memory */
char device_buffer_pcdev0[DEV_MEM_SIZE_PCDEV0];
char device_buffer_pcdev1[DEV_MEM_SIZE_PCDEV1];
char device_buffer_pcdev2[DEV_MEM_SIZE_PCDEV2];
char device_buffer_pcdev3[DEV_MEM_SIZE_PCDEV3];

/* Device private data structure */
struct pcdev_private_data{
	char *buffer;
	unsigned size;
	const char *serial_number;
	int perm;
	struct cdev cdev;
};

/* Driver private data structure */
struct pcdrv_private_data{
	int total_devices;
	dev_t device_number;
	struct class *pcd_class;
	struct device *pcd_device;
	struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data pcdrv_data = {
	.total_devices = NO_OF_DEVICES,
	.pcdev_data = {
		[0] = {
			.buffer = device_buffer_pcdev0,
			.size = DEV_MEM_SIZE_PCDEV0,
			.serial_number = "PCDDEV0XYZ",
			.perm = RDONLY
		},
		[1] = {
                        .buffer = device_buffer_pcdev1,
                        .size = DEV_MEM_SIZE_PCDEV1,
                        .serial_number = "PCDDEV1XYZ",
                        .perm = WRONLY
                },
		[2] = {
                        .buffer = device_buffer_pcdev2,
                        .size = DEV_MEM_SIZE_PCDEV2,
                        .serial_number = "PCDDEV2XYZ",
                        .perm = RDWR
                },
		[3] = {
                        .buffer = device_buffer_pcdev3,
                        .size = DEV_MEM_SIZE_PCDEV3,
                        .serial_number = "PCDDEV3XYZ",
                        .perm = RDWR
                }
	}
};

/* This hold the device number */
/* dev_t device_number; */
/* cdev variable (info of character device) */
/* struct cdev pcd_cdev; */

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
	struct pcdev_private_data *pcdev_data = (struct pcdev_private_data*)filp->private_data;
	loff_t temp;
	int max_size;

	max_size = pcdev_data->size;
	pr_info("llseek requested\n");
	pr_info("Current file position: %lld\n", filp->f_pos);
	switch(whence){
		case SEEK_SET:
			if((off > max_size) || (off < 0))
				return -EINVAL;
			filp->f_pos = off;
			break;
		case SEEK_CUR:
			temp = filp->f_pos + off;
			if((temp > max_size) || (temp < 0))
				return -EINVAL;
			filp->f_pos = temp;
			break;
		case SEEK_END:
			temp = max_size + off;
                        if((temp > max_size) || (temp < 0))
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
	int max_size;
	struct pcdev_private_data *pcdev_data;

	pcdev_data = (struct pcdev_private_data*)filp->private_data;
	max_size = pcdev_data->size;

	pr_info("Number of bytes is requested: %zu\n", count);
	pr_info("Current file position: %lld\n", *f_pos);
	/* Adjust the 'count' */
	if((*f_pos + count) > max_size){
		count = max_size - *f_pos;
	}
	/* Copy data to user space */
	if(copy_to_user(buff, pcdev_data->buffer + *f_pos, count)){
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
	int max_size;
        struct pcdev_private_data *pcdev_data;

	pcdev_data = (struct pcdev_private_data*)filp->private_data;
	max_size = pcdev_data->size;

	pr_info("Number of bytes is requested: %zu\n", count);
        pr_info("Current file position: %lld\n", *f_pos);
        /* Adjust the 'count' */
        if((*f_pos + count) > max_size){
                count = max_size - *f_pos;
        }
	if(!count){
		pr_err("No space left on the device\n");
		return -ENOMEM;
	}
        /* Copy data from user space */
        if(copy_from_user(pcdev_data->buffer + *f_pos, buff, count)){
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
	struct pcdev_private_data *pcdev_data;
	int ret;
	int minor;
	
	minor = MINOR(inode->i_rdev);
	/* Get device's private data structure */
	pcdev_data = container_of(inode->i_cdev, struct pcdev_private_data, cdev);
	/* To supplies device private data to other methods of driver */
	filp->private_data = (void*)pcdev_data;

	ret = check_permission(pcdev_data->perm, filp->f_mode);
	(!ret)?pr_info("Open successfully\n"):pr_info("Open unsuccessfully\n");

	return ret;
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

/* struct class *class_pcd; */
/* struct device *pcd_device; */

static int __init pcd_init(void)
{
	int ret;
	int i;
	/* Dynamically allocate a device number */
	ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcd_devices");
	if(ret < 0){
		goto out;
	}
	/* Create device class /sys/class/ */
	pcdrv_data.pcd_class = class_create(THIS_MODULE, "pcd_class");
	if(IS_ERR(pcdrv_data.pcd_class)){
		ret = PTR_ERR(pcdrv_data.pcd_class);
		goto unreg_chrdev;
	}
	for(i=0; i<NO_OF_DEVICES; i++){
		/* Create device file with fops */
	        cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
        	/* Init devive owner */
	        pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
        	/* Register device with the VFS */
       		 ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number + i, 1);
        	if(ret < 0){
                	goto device_destroy;
	        }
		/* Populate the sysfs with device information */
		pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, NULL, pcdrv_data.device_number + i, NULL,\
				"pcd-%d", i);
		if(IS_ERR(pcdrv_data.pcd_device)){
			ret = PTR_ERR(pcdrv_data.pcd_device);
			goto cdev_del;
		}
	}
     
	pr_info("Module was initialized successfully\n");

	return 0;

cdev_del:
device_destroy:
	for(; i>=0; i--){
		device_destroy(pcdrv_data.pcd_class, pcdrv_data.device_number + i);
	        cdev_del(&pcdrv_data.pcdev_data[i].cdev);
	}
	class_destroy(pcdrv_data.pcd_class);
unreg_chrdev:
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
out:
	return ret;
}

static void __exit pcd_exit(void)
{
	int i;
	for(i=0; i<NO_OF_DEVICES; i++){
                device_destroy(pcdrv_data.pcd_class, pcdrv_data.device_number + i);
                cdev_del(&pcdrv_data.pcdev_data[i].cdev);
        }
        class_destroy(pcdrv_data.pcd_class);
	unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);
	pr_info("Module unloaded\n");
}

module_init(pcd_init);
module_exit(pcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HUYEN DO VAN");
MODULE_DESCRIPTION("EXAMPLE CHARACTER DEVICE DRIVER");
