#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define SYS_LCD_PATH        "/sys/class/i2c-lcd/lcd-16x02"
#define PATH_LEN            100U

void lcd_write_string(char* str){
    char buff[PATH_LEN];
    int fd;

    snprintf(buff, sizeof(buff), SYS_LCD_PATH "/lcdtext");
    fd = open(buff, O_WRONLY);
    if(fd < 0){
        printf("Cannot open %s\n", buff);
        return;
    }
    write(fd, str, strlen(str));
    close(fd);
}

void lcd_clear(void){
    char buff[PATH_LEN];
    int fd;
    char clear_cmd[] = "0x01";

    snprintf(buff, sizeof(buff), SYS_LCD_PATH "/lcdcmd");
    fd = open(buff, O_WRONLY);
    if(fd < 0){
        printf("Cannot open %s\n", buff);
        return;
    }
    write(fd, clear_cmd, sizeof(clear_cmd));
    close(fd);
}

void lcd_goto_xy(char* pos){
    char buff[PATH_LEN];
    int fd;

    snprintf(buff, sizeof(buff), SYS_LCD_PATH "/lcdxy");
    fd = open(buff, O_WRONLY);
    if(fd < 0){
        printf("Cannot open %s\n", buff);
        return;
    }
    write(fd, pos, strlen(pos));
    close(fd);
}

int main(void){
    lcd_write_string("huyendv");
    lcd_goto_xy("0,1");
    lcd_write_string("hello");
    sleep(5);
    lcd_clear();
    lcd_goto_xy("8,0");
    lcd_write_string("test");

    return 0;
}