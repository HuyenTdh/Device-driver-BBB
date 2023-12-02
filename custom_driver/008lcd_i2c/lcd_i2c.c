#include <linux/module.h>
#include <linux/of.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <uapi/linux/i2c-lcd1602.h>
#include "lcd_i2c.h"
#include "lcd_driver.h"

static struct lcd_drv_data lcd_drv_data;
struct lcd_pos{
    int x;
    int y;
};

static struct lcd_pos get_lcd_pos(char* buf){
    struct lcd_pos cur_pos;
    char data[20];
    int pos;
    int end;

    for(end = 0; (buf[end] != '\0') && (buf[end] != 0x0A); end++){
        if(buf[end] == ',')
            pos = end;
    }
    strncpy(data, buf, pos);
    data[pos] = '\0';
    if(kstrtoint(data, 10, &cur_pos.x) < 0){
        cur_pos.x = -1;
        return cur_pos;
    }
    strncpy(data, (buf + pos + 1), (end - pos - 1));
    data[end - pos - 1] = '\0';
    if(kstrtoint(data, 10, &cur_pos.y) < 0){
        cur_pos.y = -1;
        return cur_pos;
    }

    return cur_pos;
}

/**************************Create attribute of class***************************/
ssize_t lcdcmd_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count){
    long value;
	int status;
    struct lcd_data* i2c_lcd_data = (struct lcd_data*)dev_get_drvdata(dev);

    status = kstrtol(buf, 0, &value);
    if(!status){
        lcd_send_cmd(i2c_lcd_data->client, (char)value);
    }

    return status ? : count;
}

ssize_t label_show(struct device *dev, struct device_attribute *attr, char *buf){
    struct lcd_data* i2c_lcd_data = (struct lcd_data*)dev_get_drvdata(dev);

    return sprintf(buf, "%s\n", i2c_lcd_data->label);
}

ssize_t lcdtext_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count){
    struct lcd_data* i2c_lcd_data = (struct lcd_data*)dev_get_drvdata(dev);

    if(buf){
        dev_info(dev, "lcdtext: %s\n", buf);
        lcd_send_string(i2c_lcd_data->client, (char*)buf);
    }else{
        return -EINVAL;
    }

    return count;
}

ssize_t lcdxy_show(struct device *dev, struct device_attribute *attr, char *buf){
    struct lcd_data* i2c_lcd_data = (struct lcd_data*)dev_get_drvdata(dev);

    return sprintf(buf, "%s\n", i2c_lcd_data->lcd_xy);
}

ssize_t lcdxy_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count){
    struct lcd_data* i2c_lcd_data = (struct lcd_data*)dev_get_drvdata(dev);
    int status;
    struct lcd_pos cur_pos;

    cur_pos = get_lcd_pos((char*)buf);
    dev_info(dev, "%d,%d", cur_pos.x, cur_pos.y);
    if((cur_pos.x < 0) || (cur_pos.y < 0)){
        return -EFAULT;
    }
    status = sprintf(i2c_lcd_data->lcd_xy, "(%d,%d)", cur_pos.x, cur_pos.y);
    lcd_goto_xy(i2c_lcd_data->client, (unsigned char)cur_pos.x, (unsigned char)cur_pos.y);

    return status;
}

static DEVICE_ATTR_WO(lcdcmd);
static DEVICE_ATTR_WO(lcdtext);
static DEVICE_ATTR_RW(lcdxy);
static DEVICE_ATTR_RO(label);

static struct attribute* i2c_lcd_attr[] = {
    &dev_attr_label.attr,
    &dev_attr_lcdcmd.attr,
    &dev_attr_lcdtext.attr,
    &dev_attr_lcdxy.attr,
    NULL
};

const struct attribute_group lcd_attr_group = {
    .attrs = i2c_lcd_attr
};

const struct attribute_group* lcd_attr_groups[] = {
    &lcd_attr_group,
    NULL
};

/**************************Create file operation****************************************/
long lcd_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    struct lcd_data* i2c_lcd_data;
    struct lcd_pos cur_pos;
    int index = 0;
    char data[20];

    i2c_lcd_data = (struct lcd_data*)filp->private_data;
    switch(cmd){
        case LCD_SEND_STRING:
            do{
                if(copy_from_user(data + index, (char*)arg + index, 1)){
                    return -EFAULT;
                }
                index++;
            }
            while(data[index - 1] != '\0');
            dev_info(lcd_drv_data.dev, "lcdtext: %s\n", data);
            lcd_send_string(i2c_lcd_data->client, data);
            break;
        case LCD_CLEAR:
            dev_info(lcd_drv_data.dev, "Clear LCD\n");
            lcd_clear(i2c_lcd_data->client);
            break;
        case LCD_GOTO_XY:
            if(arg < 0){
                return -EFAULT;
            }
            do{
                if(copy_from_user(data + index, (char*)arg + index, 1)){
                    return -EFAULT;
                }
                index++;
            }
            while(data[index - 1] != '\0');
            data[index] = '\0';
            cur_pos = get_lcd_pos(data);
            if((cur_pos.x < 0) || (cur_pos.y < 0)){
                return -EFAULT;
            }
            dev_info(lcd_drv_data.dev, "LCD goto (%d,%d)\n", cur_pos.x, cur_pos.y);
            index = sprintf(i2c_lcd_data->lcd_xy, "(%d,%d)", cur_pos.x, cur_pos.y);
            if(!index){
                return index;
            }
            lcd_goto_xy(i2c_lcd_data->client, (unsigned char)cur_pos.x, (unsigned char)cur_pos.y);
            break;
        default:
            return -ENOTTY;
    }

    return 0;
}

int lcd_open(struct inode *inode, struct file *filp){
    struct lcd_data* i2c_lcd_data;

    i2c_lcd_data = container_of(inode->i_cdev, struct lcd_data, cdev);
    filp->private_data = (void*)i2c_lcd_data;

    return 0;
}

struct file_operations lcd_ops = {
    .unlocked_ioctl = lcd_unlocked_ioctl,
    .open = lcd_open,
};

/**************************Probe and remove driver's function***************************/
int lcd_probe(struct i2c_client *client, const struct i2c_device_id *id){
    struct lcd_data* i2c_lcd_data;
    struct device* dev = &client->dev;
    struct device_node* of_node = client->dev.of_node;
    const char *name;
    int res;

    i2c_lcd_data = devm_kzalloc(dev, sizeof(struct lcd_data), GFP_KERNEL);
    if(!i2c_lcd_data){
        dev_err(dev, "Cannot allocate memory\n");
        return -1;
    }
    /* Get name of device from Device tree */
    dev_info(dev, "LCD: creating device at address 0x%02x\n", client->addr);
    if((res = of_property_read_string(of_node, "label", &name)) < 0){
            pr_err("Cannot read label of device tree\n");
            return res;
    }else{
        strcpy(i2c_lcd_data->label, name);
        dev_info(dev, "I2C-LCD label: %s\n", i2c_lcd_data->label);
    }
    i2c_lcd_data->client = client;
    sprintf(i2c_lcd_data->lcd_xy, "(0,0)");

    res = alloc_chrdev_region(&lcd_drv_data.dev_number, 0, 1, "lcd1602");
    if(res < 0){
        dev_err(dev, "Cannot allocate device number\n");
        return res;
    }
    cdev_init(&i2c_lcd_data->cdev, &lcd_ops);
    i2c_lcd_data->cdev.owner = THIS_MODULE;
    res = cdev_add(&i2c_lcd_data->cdev, lcd_drv_data.dev_number, 1);
    if(res < 0){
        dev_err(dev, "Cannot add device\n");
        goto free_dev_num;
    }

    /* Create device under /sys/class/i2c-lcd, name is "i2c-lcd"(i2c_lcd_data->label) */
    lcd_drv_data.dev = device_create_with_groups(lcd_drv_data.i2c_lcd_class, dev,\
                            lcd_drv_data.dev_number, (void*)i2c_lcd_data, lcd_attr_groups, i2c_lcd_data->label);
    if(!lcd_drv_data.dev){
        dev_err(dev, "Cannot create device\n");
        res = PTR_ERR(lcd_drv_data.dev);
        goto cdev_del;
    }
    lcd_init(client);

    return 0;

cdev_del:
    cdev_del(&i2c_lcd_data->cdev);
free_dev_num:
    unregister_chrdev_region(lcd_drv_data.dev_number, 1);
    return res;
}

int lcd_remove(struct i2c_client *client){
    dev_info(&client->dev,"Remove function is calling\n");
    device_unregister(lcd_drv_data.dev);

    return 0;
}

/******************* Initiate i2c_driver *******************/
static struct i2c_device_id lcd_idtable[] = {
    {"i2c-lcd", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, lcd_idtable);

static const struct of_device_id lcd_of_id[] = {
    {.compatible = "org,i2c-lcd"},
    {},
};
MODULE_DEVICE_TABLE(of, lcd_of_id);

static struct i2c_driver lcd_driver = {
      .driver = {
              .name   = "i2c-lcd",
              .of_match_table = lcd_of_id,
      },

      .id_table       = lcd_idtable,
      .probe          = lcd_probe,
      .remove         = lcd_remove,
};

/******************* Module Init and Exit*******************/
static int __init i2c_lcd_init(void){
    int res;
    /* Create i2c-lcd class */
    lcd_drv_data.i2c_lcd_class = class_create(THIS_MODULE, "i2c-lcd");
    if(IS_ERR(lcd_drv_data.i2c_lcd_class)){
        pr_err("Cannot create i2c-lcd class\n");
        return PTR_ERR(lcd_drv_data.i2c_lcd_class);
    }
    /* Register the driver module */
    res = i2c_add_driver(&lcd_driver);
    return 0;
}

static void __exit i2c_lcd_exit(void){
    /* Delete the driver module */
    i2c_del_driver(&lcd_driver);
    /* Destroy i2c-lcd class */
    class_destroy(lcd_drv_data.i2c_lcd_class);
}


module_init(i2c_lcd_init);
module_exit(i2c_lcd_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huyen Do Van");