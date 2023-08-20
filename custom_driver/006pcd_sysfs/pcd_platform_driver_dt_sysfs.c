#include "pcd_platform_driver_dt_sysfs.h"

struct pcdrv_private_data pcdrv_data;

static int platform_pcdrv_probe(struct platform_device *platform_pcdev);
static int platform_pcdrv_remove(struct platform_device *platform_pcdev);
ssize_t show_max_size(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t show_serial_num(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t store_max_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
int pcd_sysfs_create_files(struct device *pcd_dev);

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

struct device_config pcdev_config[] = {
	[PCDEVA1X] = {.item1 = 60, .item2 = 21},
	[PCDEVB1X] = {.item1 = 50, .item2 = 22},
	[PCDEVC1X] = {.item1 = 40, .item2 = 23},
	[PCDEVD1X] = {.item1 = 30, .item2 = 24},
};
struct platform_device_id pcdevs_ids[] ={
	[0] = {.name = "pcdev-A1x", .driver_data = PCDEVA1X},
	[1] = {.name = "pcdev-B1x", .driver_data = PCDEVB1X},
	[2] = {.name = "pcdev-C1x", .driver_data = PCDEVC1X},
	[3] = {.name = "pcdev-D1x", .driver_data = PCDEVD1X},
	{}
};

struct of_device_id org_pcdev_dt_match[] = {
	{.compatible = "pcdev-A1x", .data = (void*)PCDEVA1X},
	{.compatible = "pcdev-B1x", .data = (void*)PCDEVB1X},
	{.compatible = "pcdev-C1x", .data = (void*)PCDEVC1X},
	{.compatible = "pcdev-D1x", .data = (void*)PCDEVD1X},
	{}
};

struct platform_driver platform_pcdrv = {
	.probe = platform_pcdrv_probe,
	.remove = platform_pcdrv_remove,
	.id_table = pcdevs_ids,
	.driver = {
		.name = "pcd-char-device",
		.of_match_table = of_match_ptr(org_pcdev_dt_match)
	}
};

static DEVICE_ATTR(max_size, S_IRUGO|S_IWUSR, show_max_size, store_max_size);
static DEVICE_ATTR(serial_num, S_IRUGO, show_serial_num, NULL);

struct attribute* pcd_attrs[] = {
	&dev_attr_max_size.attr,
	&dev_attr_serial_num.attr,
	NULL
};

struct attribute_group pcd_attr_group = {
	.attrs = pcd_attrs
};

ssize_t show_max_size(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* get access to the device private data */
	struct pcdev_private_data *pcdev_data = dev_get_drvdata(dev->parent);

	return sprintf(buf, "%d\n", pcdev_data->pdata.size);
	return 0;
}
ssize_t show_serial_num(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* get access to the device private data */
	struct pcdev_private_data *pcdev_data = dev_get_drvdata(dev->parent);

	return sprintf(buf, "%s\n", pcdev_data->pdata.serial_number);
}
ssize_t store_max_size(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	long value;
	/* get access to the device private data */
	struct pcdev_private_data *pcdev_data = dev_get_drvdata(dev->parent);

	ret = kstrtol(buf, 10, &value);
	if(ret)
		return ret;
	pcdev_data->pdata.size = value;
	pcdev_data->buffer = krealloc(pcdev_data->buffer, pcdev_data->pdata.size, GFP_KERNEL);

	return count;
}

int pcd_sysfs_create_files(struct device *pcd_dev)
{
	int ret;
#if 0
	ret = sysfs_create_file(&pcd_dev->kobj, &dev_attr_max_size.attr);
	if(ret)
		return ret;
	return sysfs_create_file(&pcd_dev->kobj, &dev_attr_serial_num.attr);
#endif
	return sysfs_create_group(&pcd_dev->kobj, &pcd_attr_group);
}

struct platform_device_data* pcdev_get_platform_data_from_dt(struct device* dev)
{
	struct platform_device_data* pdata;
	struct device_node* dev_node = dev->of_node;

	if(!dev_node)
		return NULL;
	pdata = devm_kzalloc(dev, sizeof(struct platform_device_data), GFP_KERNEL);
	if(!pdata){
		dev_info(dev, "Cannot allocate memory\n");
		return ERR_PTR(-ENOMEM);
	}
	if(of_property_read_string(dev_node, "org,device-serial-number", &pdata->serial_number)){
		dev_info(dev, "Missing serial number property\n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node, "org,size", &pdata->size)){
		dev_info(dev, "Missing size\n");
		return ERR_PTR(-EINVAL);
	}
	if(of_property_read_u32(dev_node, "org,perm", &pdata->perm)){
                dev_info(dev, "Missing permission\n");
                return ERR_PTR(-EINVAL);
        }

	return pdata;
}

/* Gets call when matched platform device is found */
static int platform_pcdrv_probe(struct platform_device *platform_pcdev)
{
	int ret;
	struct platform_device_data *pdata;
	struct pcdev_private_data *dev_data;
	/* used to store matched entry of 'of_device_id' list of this driver */
	const struct of_device_id *match;
	struct device *dev = &platform_pcdev->dev;
	int driver_data;

	dev_info(dev,"Device is detected\n");
	/* match will always be NULL if LINUX doesnt support device tree i.e CONFIG_OF is off */
	match = of_match_device(of_match_ptr(platform_pcdev->dev.driver->of_match_table), &platform_pcdev->dev);

	if(match){
		pdata = pcdev_get_platform_data_from_dt(&platform_pcdev->dev);
		if(IS_ERR(pdata)){
        	        return PTR_ERR(pdata);
	        }
		driver_data = (long)match->data;
	}else{
		/* Get the platform data */
                /*pdata = (struct platform_device_data*)platform_pcdev->dev.platform_data;*/
                pdata = (struct platform_device_data*)dev_get_platdata(&platform_pcdev->dev);
		driver_data = platform_pcdev->id_entry->driver_data;
	}

	if(!pdata)
	{
	        dev_info(dev, "No platform data available\n");
               	return -EINVAL;
	}

	/* Dynamically allocate memory for the device private data */
/*	dev_data = kzalloc(sizeof(struct pcdev_private_data), GFP_KERNEL); */
	dev_data = devm_kzalloc(&platform_pcdev->dev, sizeof(struct pcdev_private_data), GFP_KERNEL);
	if(!dev_data){
		dev_info(dev, "Cannot allocate memory\n");
		return -ENOMEM;
	}
	dev_data->pdata.size = pdata->size;
	dev_data->pdata.perm = pdata->perm;
	dev_data->pdata.serial_number = pdata->serial_number;

	dev_info(dev, "Device serial: %s\n", dev_data->pdata.serial_number);
	dev_info(dev, "Device size: %d\n", dev_data->pdata.size);
	dev_info(dev,"Device permission: %d\n", dev_data->pdata.perm);
	dev_info(dev, "Config item1 = %d\n", pcdev_config[driver_data].item1);
	dev_info(dev, "Config item2 = %d\n", pcdev_config[driver_data].item2);
	/* Dynamically allocate memory for the device buffer using size information
	 * from the platform data */
	/* dev_data->buffer = kzalloc(dev_data->pdata.size, GFP_KERNEL); */
	dev_data->buffer = devm_kzalloc(&platform_pcdev->dev, dev_data->pdata.size, GFP_KERNEL);
	if(!(dev_data->buffer)){
		dev_info(dev, "Cannot allocate buffer's memory\n");
		return -ENOMEM;
	}
	/* Get the device number */
	dev_data->device_number = pcdrv_data.device_number_base + pcdrv_data.total_devices;
	/* Do cdev init and cdev_add*/
	cdev_init(&dev_data->cdev, &pcd_fops);
	dev_data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&dev_data->cdev, dev_data->device_number, 1);
	if(ret < 0){
		dev_info(dev, "Cannot add a char device to the system\n");
		return ret;
	}
	/* Create device file for detected platform device */
	pcdrv_data.pcd_device = device_create(pcdrv_data.pcd_class, dev, dev_data->device_number,\
			NULL, "pcd-%d", pcdrv_data.total_devices);
	if(IS_ERR(pcdrv_data.pcd_device)){
		dev_info(dev, "Cannot create device file\n");
		ret = PTR_ERR(pcdrv_data.pcd_class);
		cdev_del(&dev_data->cdev);
		return ret;
	}

	/* Store pcdev_private_data to platform_pcdev->dev.driver_data
	 * It will be used in remove function */
	/* platform_pcdev->dev.driver_data = dev_data; */
	platform_set_drvdata(platform_pcdev, dev_data);
	
	pcdrv_data.total_devices++;
	
	ret = pcd_sysfs_create_files(pcdrv_data.pcd_device);
	if(ret)
	{
		device_destroy(pcdrv_data.pcd_class, dev_data->device_number);
		return ret;
	}

	dev_info(dev ,"The probe was successful\n");

	return 0;
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
	dev_info(&platform_pcdev->dev, "Device unpluged\n");

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
