#include <linux/module.h>
#include <linux/of.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/i2c.h>
#include "lcd_i2c.h"
#include "lcd_driver.h"

static struct lcd_drv_data lcd_drv_data;

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
    long value;
    int x;
    int y;

    status = kstrtol(buf, 10, &value);
    if(status < 0){
        return status;
    }
    x = value / 10;
    y = value % 10;
    status = sprintf(i2c_lcd_data->lcd_xy, "(%d,%d)", x, y);
    lcd_goto_xy(i2c_lcd_data->client, (unsigned char)x, (unsigned char)y);

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
    /* Create device under /sys/class/i2c-lcd, name is "i2c-lcd"(i2c_lcd_data->label) */
    lcd_drv_data.dev = device_create_with_groups(lcd_drv_data.i2c_lcd_class, dev,\
                            0, (void*)i2c_lcd_data, lcd_attr_groups, i2c_lcd_data->label);
    if(!lcd_drv_data.dev){
        dev_err(dev, "Cannot create device\n");
        return PTR_ERR(lcd_drv_data.dev);
    }

    lcd_init(client);

    return 0;
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