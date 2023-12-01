#include <linux/delay.h>
#include <linux/i2c.h>
#include "lcd_i2c.h"

/* LCD commands */
#define LCD_CLEAR_DISPLAY                   0x01U
#define LCD_RETURN_HOME                     0x02U
#define LCD_DISPLAY_CONTROL                 0x08U
#define LCD_SETDDRAMADDR                    0x80U
/* Flags for display on/off control */
#define LCD_DISPLAY_ON                      0x04U
#define LCD_DISPLAY_OFF                     0x00U
#define LCD_CURSOR_ON                       0x02U
#define LCD_CURSOR_OFF                      0x00U
#define LCD_BLINK_ON                        0x01U
#define LCD_BLINK_OFF                       0x00U
/*  Flags for display/cursor shift */
#define LCD_DISPLAY_MOVE                    0x08U
#define LCD_CURSOR_MOVE                     0x00U
#define LCD_MOVE_RIGHT                      0x04U
#define LCD_MOVE_LEFT                       0x00U
/* Flags for backlight control */
#define LCD_BACKLIGHT                       0x08U
#define LCD_NO_BACKLIGHT                    0x00U

void lcd_send_cmd(const struct i2c_client *client, char cmd){
    char data_u;
    char data_l;
    unsigned char data_t[4];

    data_u = cmd & 0xf0;
	data_l = (cmd << 4) & 0xf0;
    /* en=1, rs=0 */
    data_t[0] = data_u|0x0C;
    /* en=0, rs=0 */
	data_t[1] = data_u|0x08;
    /* en=1, rs=0 */
	data_t[2] = data_l|0x0C;
    /* en=0, rs=0 */
	data_t[3] = data_l|0x08;
    i2c_master_send(client, data_t, 4);
}

static void lcd_send_data(const struct i2c_client *client, char data){
    char data_u;
    char data_l;
    unsigned char data_t[4];

    data_u = data & 0xf0;
    data_l = (data << 4) & 0xf0;
    /* en=1, rs=1 */
    data_t[0] = data_u|0x0D;
    /* en=0, rs=1 */
	data_t[1] = data_u|0x09;
    /* en=1, rs=1 */
	data_t[2] = data_l|0x0D;
    /* en=0, rs=1 */
	data_t[3] = data_l|0x09;
    i2c_master_send(client, data_t, 4);
}

void lcd_init(const struct i2c_client *client){
    mdelay(50);
    lcd_send_cmd(client, 0x30);
    mdelay(5);
    lcd_send_cmd(client, 0x30);
    mdelay(1);
    lcd_send_cmd(client, 0x30);
    mdelay(10);
    lcd_send_cmd(client, 0x20);
    mdelay(10);
    lcd_send_cmd(client, 0x28);
    mdelay(1);
    lcd_send_cmd(client, LCD_DISPLAY_MOVE);
    mdelay(1);
    lcd_send_cmd(client, LCD_CLEAR_DISPLAY);
	mdelay(1);
	mdelay(1);
    lcd_send_cmd(client, 0x06);
	mdelay(1);
	lcd_send_cmd(client, 0x0C);
}

void lcd_send_string(const struct i2c_client *client, char* str, int length){
    while((*str != 0x0A) && (*str != '\0')){
        lcd_send_data(client, *str);
        str++;
    }
}

void lcd_clear(const struct i2c_client *client){
    lcd_send_cmd(client, LCD_CLEAR_DISPLAY);
    mdelay(3);
}

void lcd_home(const struct i2c_client *client){
    lcd_send_cmd(client, LCD_RETURN_HOME);
    mdelay(3);
}
/* Turn display off */
void lcd_no_display(const struct i2c_client *client){
    lcd_send_cmd(client, LCD_DISPLAY_CONTROL | LCD_DISPLAY_OFF);
}
/* Turn display on */
void lcd_display(const struct i2c_client *client){
    lcd_send_cmd(client, LCD_DISPLAY_CONTROL | LCD_DISPLAY_ON);
}
/* Turn cursor off */
void lcd_no_cursor(const struct i2c_client *client){
	lcd_send_cmd(client, LCD_DISPLAY_CONTROL | LCD_CURSOR_OFF);
}
/* Turn cursor on */
void lcd_cursor(const struct i2c_client *client){
	lcd_send_cmd(client, LCD_DISPLAY_CONTROL | LCD_CURSOR_ON);
}
/* Set cursor's position */
void lcd_goto_xy(const struct i2c_client *client, unsigned char x, unsigned char y){
    unsigned char address;

    if(!y){
        address = (0x80 + x);
    }
    else{
        address = (0xc0 + x);
    }
    mdelay(1);
    lcd_send_cmd(client, address);
    mdelay(1);
}