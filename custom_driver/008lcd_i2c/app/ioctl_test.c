#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "i2c-lcd1602.h"

int main(void){
    int fd;

    fd = open("/dev/lcd-16x02", O_RDWR);
    ioctl(fd, LCD_SEND_STRING, "huyendv");
    ioctl(fd, LCD_GOTO_XY, "0,1");
    ioctl(fd, LCD_SEND_STRING, "hello");
    sleep(5);
    ioctl(fd, LCD_CLEAR, "hello");
    ioctl(fd, LCD_GOTO_XY, "8,0");
    ioctl(fd, LCD_SEND_STRING, "test");
    close(fd);

    return 0;
}