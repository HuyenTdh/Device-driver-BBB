#ifndef _LCD_DRIVER_H_
#define _LCD_DRIVER_H_

void lcd_send_cmd(const struct i2c_client *client, char cmd);
void lcd_init(const struct i2c_client *client);
void lcd_send_string(const struct i2c_client *client, char* str);
void lcd_clear(const struct i2c_client *client);
void lcd_home(const struct i2c_client *client);
void lcd_no_display(const struct i2c_client *client);
void lcd_display(const struct i2c_client *client);
void lcd_no_cursor(const struct i2c_client *client);
void lcd_cursor(const struct i2c_client *client);
void lcd_goto_xy(const struct i2c_client *client,\
                unsigned char x, unsigned char y);

#endif /* _LCD_DRIVER_H_ */