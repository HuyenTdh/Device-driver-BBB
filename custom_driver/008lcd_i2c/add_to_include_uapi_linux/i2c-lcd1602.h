#ifndef _UAPI_LCD_I2C_H_
#define _UAPI_LCD_I2C_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#define LCD_IOC_MAGIC                       'k'

#define LCD_CLEAR                           _IO(LCD_IOC_MAGIC, 0)
#define LCD_SEND_STRING                     _IOW(LCD_IOC_MAGIC, 1, int)
#define LCD_GOTO_XY                         _IOW(LCD_IOC_MAGIC, 2, int)

#endif /* _UAPI_LCD_I2C_H_ */