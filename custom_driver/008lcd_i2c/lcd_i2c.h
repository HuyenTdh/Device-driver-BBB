#ifndef _LCD_I2C_H_
#define _LCD_I2C_H_

#include <linux/device.h>

struct lcd_data {
    char label[20];
    char lcd_xy[8];
    char i2c_addr;
    struct i2c_client* client;
};

struct lcd_drv_data {
    struct class* i2c_lcd_class;
    struct device* dev;
};

#endif /* _LCD_I2C_H_ */