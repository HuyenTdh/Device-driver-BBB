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
#include <linux/delay.h>
#include <linux/string.h>

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
struct nokia_position{
    char x;
    char y;
};

struct nokia5110_dev_data{
    char                label[20];
    struct gpio_desc    *dc_desc;
    struct gpio_desc    *rst_desc;
    struct cdev         cdev;
    struct spi_device   *spi;
    spinlock_t          nokia_spinlock;
    struct list_head    device_entry;
    struct mutex        buf_lock;
    u8                  *tx_buffer;
    u8                  *rx_buffer;
    u32                 speed_hz;
    struct nokia_position pos;
};

struct nokia5110_drv_data{
    struct device*      nokia_device;
    dev_t               dev_number;
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

struct nokia_position nokia_get_postion(struct nokia5110_dev_data *nokiadev,\
            char *buf, size_t count)
{
    struct nokia_position pos;
    int index;
    int i;
    char tmp[20];
    int status;

    strncpy(tmp, buf, count);
    for(i = 0; i < count; i++) {
        switch (tmp[i])
        {
        case ',':
            index = i;
            break;
        case 0x0A:
            tmp[i] = '\0';
            break;

        default:
            break;
        }
    }
    tmp[index] = '\0';
    status = kstrtos8(tmp, 10, &pos.x);
    status = kstrtos8(tmp + index + 1, 10, &pos.y);

    return pos;
}

/*-------------------------------------------------------------------------*/
#define LCD_WIDTH                                   84U
#define LCD_HEIGHT                                  48U

#define NOKIA5110_FUNCTION_SET                      0x20U
#define NOKIA5110_SET_CURSOR_Y                      0x40U
#define NOKIA5110_DISPLAY_CONTROL                   0x08U
#define NOKIA5110_SET_CURSOR_X                      0x80U
#define NOKIA5110_TEMPERATURE_CONTROL               0x04U
#define NOKIA5110_BIASS_SYSTEM                      0x10U
#define NOKIA5110_SET_VOP                           0x80U

#define PCD8544_POWERDOWN                           0x04U
#define PCD8544_ENTRYMODE                           0x02U
#define PCD8544_EXTENDED_INSTRUCTION                0x01U

#define PCD8544_DISPLAY_BLANK                       0x0U
#define PCD8544_DISPLAY_NORMAL                      0x4U
#define PCD8544_DISPLAY_ALLON                       0x1U
#define PCD8544_DISPLAY_INVERTED                    0x5U

static void NOKIA5110_WriteCmd(struct nokia5110_dev_data *nokia_dev, u8 cmd)
{
    mutex_lock(&nokia_dev->buf_lock);
    gpiod_set_value(nokia_dev->dc_desc, 0);
    nokia_dev->tx_buffer[0] = cmd;
    nokia_sync_write(nokia_dev, 1);
    gpiod_set_value(nokia_dev->dc_desc, 1);
    mutex_unlock(&nokia_dev->buf_lock);
}

static void NOKIA5110_WriteData(struct nokia5110_dev_data *nokia_dev, u8 data)
{
    mutex_lock(&nokia_dev->buf_lock);
    gpiod_set_value(nokia_dev->dc_desc, 1);
    nokia_dev->tx_buffer[0] = data;
    nokia_sync_write(nokia_dev, 1);
    mutex_unlock(&nokia_dev->buf_lock);
}

static void NOKIA5110_FunctionSet(struct nokia5110_dev_data *nokia_dev, u8 mode)
{
    NOKIA5110_WriteCmd(nokia_dev, NOKIA5110_FUNCTION_SET | mode);
}

static void NOKIA5110_SetExtendedCmd(struct nokia5110_dev_data *nokia_dev)
{
    NOKIA5110_FunctionSet(nokia_dev, PCD8544_EXTENDED_INSTRUCTION);
}

static void NOKIA5110_SetBasicCmd(struct nokia5110_dev_data *nokia_dev)
{
    NOKIA5110_FunctionSet(nokia_dev, 0);
}

static void NOKIA5110_SetCursorX(struct nokia5110_dev_data *nokia_dev, u8 x)
{
    NOKIA5110_WriteCmd(nokia_dev, NOKIA5110_SET_CURSOR_X | x);
}

static void NOKIA5110_SetCursorY(struct nokia5110_dev_data *nokia_dev, u8 y)
{
    NOKIA5110_WriteCmd(nokia_dev, NOKIA5110_SET_CURSOR_Y | y);
}

static void NOKIA5110_DisplayControl(struct nokia5110_dev_data *nokia_dev, u8 display_mode)
{
    NOKIA5110_WriteCmd(nokia_dev, NOKIA5110_DISPLAY_CONTROL | display_mode);
}

static void NOKIA5110_TemperatureControl(struct nokia5110_dev_data *nokia_dev, u8 tc)
{
    u8 cmd = NOKIA5110_TEMPERATURE_CONTROL | tc;
    NOKIA5110_SetExtendedCmd(nokia_dev);
    NOKIA5110_WriteCmd(nokia_dev, cmd);
    NOKIA5110_SetBasicCmd(nokia_dev);
}

static void NOKIA5110_BiasSystem(struct nokia5110_dev_data *nokia_dev, u8 bs)
{
    u8 cmd = NOKIA5110_BIASS_SYSTEM | bs;
    NOKIA5110_SetExtendedCmd(nokia_dev);
    NOKIA5110_WriteCmd(nokia_dev, cmd);
    NOKIA5110_SetBasicCmd(nokia_dev);
}

static void NOKIA5110_SetVop(struct nokia5110_dev_data *nokia_dev, u8 vop)
{
    u8 cmd = NOKIA5110_SET_VOP | vop;
    NOKIA5110_SetExtendedCmd(nokia_dev);
    NOKIA5110_WriteCmd(nokia_dev, cmd);
    NOKIA5110_SetBasicCmd(nokia_dev);
}

static void NOKIA5110_SetCursor(struct nokia5110_dev_data *nokia_dev, u8 x, u8 y)
{
    NOKIA5110_SetCursorX(nokia_dev, x);
    NOKIA5110_SetCursorY(nokia_dev, y);
}

static void NOKIA5110_Reset(struct nokia5110_dev_data *nokia_dev)
{
    gpiod_set_value(nokia_dev->rst_desc, 0);
    mdelay(2);
    gpiod_set_value(nokia_dev->rst_desc, 1);
    mdelay(2);
}

static void NOKIA5110_Clear(struct nokia5110_dev_data *nokia_dev)
{
    int i;
    for(i = 0; i < LCD_WIDTH * LCD_HEIGHT / 8; i++)
    {
        NOKIA5110_WriteData(nokia_dev, 0x00);
    }
    NOKIA5110_SetCursor(nokia_dev, 0, 0);
}

static void NOKIA5110_WriteChar(struct nokia5110_dev_data *nokia_dev, u8 character)
{
    int i = 0;
    NOKIA5110_WriteData(nokia_dev, 0x00);
    for(i = 0; i < 5; i++)
        NOKIA5110_WriteData(nokia_dev, ASCII[character - 0x20][i]);
    NOKIA5110_WriteData(nokia_dev, 0x00);
}

static void NOKIA5110_WriteString(struct nokia5110_dev_data *nokia_dev, u8 *str)
{
    while((*str) && (*str != 0x0A))
    {
        NOKIA5110_WriteChar(nokia_dev, *str++);
    }
}

static int NOKIA5110_Init(struct nokia5110_dev_data *nokia_dev)
{
    int res;
    /* Set the gpio direction to ouput */
	res = gpiod_direction_output(nokia_dev->dc_desc, 0);
    if(res){
        return res;
    }
    res = gpiod_direction_output(nokia_dev->rst_desc, 0);
    if(res){
        return res;
    }
    gpiod_set_value(nokia_dev->dc_desc, 1);
    NOKIA5110_Reset(nokia_dev);
    NOKIA5110_BiasSystem(nokia_dev, 0x04);
    NOKIA5110_SetVop(nokia_dev, 0x28);
    NOKIA5110_DisplayControl(nokia_dev, PCD8544_DISPLAY_NORMAL);
    NOKIA5110_Clear(nokia_dev);
    NOKIA5110_WriteString(nokia_dev, "hello");

    return 0;
}

/*-------------------------------------------------------------------------*/
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
        gpiod_set_value(nokia_dev->dc_desc, 1);
        status = nokia_sync_write(nokia_dev, count);
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
ssize_t nokiacmd_store(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    struct nokia5110_dev_data *nokia_dev = (struct nokia5110_dev_data*)dev_get_drvdata(dev);
    unsigned long val;
    int status;

    status = kstrtol(buf, 0, &val);
    if(status == 0)
        NOKIA5110_WriteCmd(nokia_dev, val);

    return status;
}

ssize_t nokiatext_store(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    struct nokia5110_dev_data *nokia_dev = (struct nokia5110_dev_data*)dev_get_drvdata(dev);

    dev_info(dev, "text: %s", buf);
    NOKIA5110_WriteString(nokia_dev, (char*)buf);

    return count;
}

ssize_t nokiaxy_store(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    struct nokia5110_dev_data *nokia_dev = (struct nokia5110_dev_data*)dev_get_drvdata(dev);

    nokia_dev->pos = nokia_get_postion(nokia_dev, (char*)buf, count);
    dev_info(dev, "cursor position: %d,%d\n", nokia_dev->pos.x, nokia_dev->pos.y);
    NOKIA5110_SetCursor(nokia_dev, nokia_dev->pos.x, nokia_dev->pos.y);

    return count;
}

ssize_t nokiaxy_show(struct device *dev, struct device_attribute *attr, char *buf)
{

    return 0;
}

static DEVICE_ATTR_WO(nokiacmd);
static DEVICE_ATTR_WO(nokiatext);
static DEVICE_ATTR_RW(nokiaxy);

struct attribute* nokia_attrs[] = {
    &dev_attr_nokiacmd.attr,
    &dev_attr_nokiatext.attr,
    &dev_attr_nokiaxy.attr,
    NULL
};

struct attribute_group nokia_attr_group = {
    .attrs = nokia_attrs
};

const struct attribute_group* nokia_attr_groups[] = {
    &nokia_attr_group,
    NULL
};

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
    nokia_dev_data->dc_desc = gpiod_get_index(dev, "ctrl", 0, GPIOD_ASIS);
    if(IS_ERR(nokia_dev_data->dc_desc)){
        res = PTR_ERR(nokia_dev_data->dc_desc);
        if(res == -ENOENT)
            dev_err(dev, "No GPIO has been assigned to the requested function and/or index\n");
        return res;
    }
    nokia_dev_data->rst_desc = gpiod_get_index(dev, "ctrl", 1, GPIOD_ASIS);
    if(IS_ERR(nokia_dev_data->rst_desc)){
        res = PTR_ERR(nokia_dev_data->rst_desc);
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

    res = NOKIA5110_Init(nokia_dev_data);
    if(res)
    {
        dev_err(dev, "Cannot set ctrl pin direction\n");
        goto device_destroy;
    }

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