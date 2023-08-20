#include "pcd_platform_driver_dt_sysfs.h"

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
