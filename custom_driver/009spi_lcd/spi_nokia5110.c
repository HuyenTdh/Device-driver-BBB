#include <linux/module.h>
#include <linux/err.h>
#include <linux/spi/spi.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/gpio/consumer.h>

/*------------------------------------FONT---------------------------------*/
static const unsigned short ASCII[][5] =
{
 {0x00, 0x00, 0x00, 0x00, 0x00} // 20
,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c ¥
,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j
,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e .
,{0x00, 0x06, 0x09, 0x09, 0x06} // 7f .
};

/*-------------------------------------------------------------------------*/
struct nokia5110_dev_data{
    char                label[20];
    struct gpio_desc*   dc_desc;
    struct cdev         cdev;
    struct spi_device   *spi;
    spinlock_t          nokia_spinlock;
    struct list_head	device_entry;
    struct mutex		buf_lock;
    u8			        *tx_buffer;
	u8			        *rx_buffer;
	u32			        speed_hz;
};

struct nokia5110_drv_data{
    struct device* nokia_device;
    dev_t dev_number;
};

static struct class* nokia5110_class;
struct nokia5110_drv_data nokia_drv_data;
static DEFINE_MUTEX(device_list_lock);

static unsigned bufsiz = 4096;

/*-------------------------------------------------------------------------*/
static ssize_t
nokia_sync(struct nokia5110_dev_data *nokiadev, struct spi_message *message)
{
    int status;
    struct spi_device *spi;

    spin_lock_irq(&nokiadev->nokia_spinlock);
    spi = nokiadev->spi;
    spin_unlock_irq(&nokiadev->nokia_spinlock);

    if(spi == NULL)
        status = -ESHUTDOWN;
    else
        status = spi_sync(spi, message);

    if(status == 0)
        status = message->actual_length;
    
    return status;
}

static ssize_t
nokia_sync_write(struct nokia5110_dev_data *nokiadev, size_t len)
{
    struct spi_transfer t = {
        .tx_buf = nokiadev->tx_buffer,
        .len    = len,
        .speed_hz = nokiadev->speed_hz,
    };
    struct spi_message m;

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);
    return nokia_sync(nokiadev, &m);    
}

static ssize_t
nokia_write(struct file *filp, const char __user *buf,
                size_t count, loff_t *f_pos)
{
    struct nokia5110_dev_data *nokia_dev;
    ssize_t status = 0;
    unsigned long missing;

    if(count > bufsiz)
        return -EMSGSIZE;
    
    nokia_dev = filp->private_data;

    mutex_lock(&nokia_dev->buf_lock);
    missing = copy_from_user(nokia_dev->tx_buffer, buf, count);
    if(missing == 0) {
        gpiod_set_value(nokia_dev->dc_desc, 0);
        status = nokia_sync_write(nokia_dev, count);
        gpiod_set_value(nokia_dev->dc_desc, 1);
    }
    else
        status = -EFAULT;
    mutex_unlock(&nokia_dev->buf_lock);

    return status;
}

static int nokia_open(struct inode *inode, struct file *filp)
{
    struct nokia5110_dev_data *nokia_dev;

    mutex_lock(&device_list_lock);
    nokia_dev = container_of(inode->i_cdev, struct nokia5110_dev_data, cdev);
    filp->private_data = nokia_dev;
    mutex_unlock(&device_list_lock);

    return 0;
}

const struct file_operations nokia5110_ops = {
    .open = nokia_open,
    .write = nokia_write
};

/*-------------------------------------------------------------------------*/
static struct of_device_id nokia5110_dt_ids[] = {
    {.compatible = "org,nokia-5110"},
    {}
};
MODULE_DEVICE_TABLE(of, nokia5110_dt_ids);

/*-------------------------------------------------------------------------*/

const struct attribute_group* nokia_attr_groups[];

/*-------------------------------------------------------------------------*/
int nokia5110_probe(struct spi_device *spi)
{
    struct nokia5110_dev_data* nokia_dev_data;
    struct device* dev = &spi->dev;
    const char* name;
    int res;

    nokia_dev_data = devm_kzalloc(dev, sizeof(struct nokia5110_dev_data), GFP_KERNEL);
    if(!nokia_dev_data){
        dev_err(dev, "Cannot allocate device data\n");
        return -ENOMEM;
    }
    nokia_dev_data->tx_buffer = devm_kzalloc(dev, bufsiz, GFP_KERNEL);
    if(!nokia_dev_data){
        dev_err(dev, "Cannot allocate tx buffer\n");
        return -ENOMEM;
    }
    if((res = of_property_read_string(dev->of_node, "label", &name)) < 0)
    {
        dev_err(dev, "Cannot get label property\n");
        return res;
    }
    else
    {
        strcpy(nokia_dev_data->label, name);
        dev_info(dev, "SPI-LCD label: %s\n", nokia_dev_data->label);
    }
    nokia_dev_data->speed_hz = spi->max_speed_hz;
    nokia_dev_data->spi = spi;
    nokia_dev_data->dc_desc = gpiod_get_index(dev, "dc", 0, GPIOD_ASIS);
    if(IS_ERR(nokia_dev_data->dc_desc)){
        res = PTR_ERR(nokia_dev_data->dc_desc);
        if(res == -ENOENT)
            dev_err(dev, "No GPIO has been assigned to the requested function and/or index\n");
        return res;
    }

    res = alloc_chrdev_region(&nokia_drv_data.dev_number, 0, 1, "nokia-5110");
    if(res < 0)
    {
        dev_err(dev, "Cannot allocate device number\n");
        return res;
    }
    cdev_init(&nokia_dev_data->cdev, &nokia5110_ops);
    nokia_dev_data->cdev.owner = THIS_MODULE;
    res = cdev_add(&nokia_dev_data->cdev, nokia_drv_data.dev_number, 1);
    if(res < 0){
        dev_err(dev, "Cannot add device\n");
        goto free_dev_num;
    }
    nokia_drv_data.nokia_device = device_create_with_groups(nokia5110_class, dev,\
                nokia_drv_data.dev_number, nokia_dev_data, nokia_attr_groups, nokia_dev_data->label);
    if(IS_ERR(nokia_drv_data.nokia_device)) {
        dev_err(dev, "Cannot create device file\n");
        res = PTR_ERR(nokia_drv_data.nokia_device);
        goto cdev_del;
    }

    dev_set_drvdata(&spi->dev, nokia_dev_data);

    /* Set the gpio direction to ouput */
	res = gpiod_direction_output(nokia_dev_data->dc_desc, 0);
	if(res){
		dev_err(dev, "gpio direction set failed\n");
		goto device_destroy;
	}
    gpiod_set_value(nokia_dev_data->dc_desc, 1);

    return 0;
device_destroy:
    device_destroy(nokia5110_class, nokia_drv_data.dev_number);
cdev_del:
    cdev_del(&nokia_dev_data->cdev);
free_dev_num:
    unregister_chrdev_region(nokia_drv_data.dev_number, 1);
    return res;
}

int nokia5110_remove(struct spi_device *spi)
{
    struct nokia5110_dev_data *nokia_dev_data = (struct nokia5110_dev_data*)dev_get_drvdata(&spi->dev);
    device_destroy(nokia5110_class, nokia_drv_data.dev_number);
    cdev_del(&nokia_dev_data->cdev);
    unregister_chrdev_region(nokia_drv_data.dev_number, 1);
    return 0;
}


static struct spi_driver nokia5110_spi_driver = {
    .driver = {
        .name = "nokia-5110",
        .of_match_table = nokia5110_dt_ids,
    },
    .probe = nokia5110_probe,
    .remove = nokia5110_remove,
};

/*-------------------------------------------------------------------------*/
static int __init nokia5110_init(void)
{
    int status;

    nokia5110_class = class_create(THIS_MODULE, "nokia-5110");
    if(IS_ERR(nokia5110_class)){
        pr_err("Cannot create class\n");
        return PTR_ERR(nokia5110_class);
    }
    status = spi_register_driver(&nokia5110_spi_driver);

    return status;
}

static void __exit nokia5110_exit(void)
{
    spi_unregister_driver(&nokia5110_spi_driver);
    class_destroy(nokia5110_class);
}

module_init(nokia5110_init);
module_exit(nokia5110_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huyen Do Van");