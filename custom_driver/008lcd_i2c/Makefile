obj-m := lcd_device_driver.o
lcd_device_driver-objs += lcd_i2c.o lcd_driver.o
ARCH=arm
CROSS_COMPILE=/home/huyendv/Desktop/linux/gcc/gcc-11.3.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-
KDIR=/home/huyendv/Desktop/source/linux/
HOST=/lib/modules/$(shell uname -r)/build/

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) modules
clean:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) clean
help:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KDIR) M=$(PWD) help
host:
	make -C $(HOST) M=$(PWD) modules