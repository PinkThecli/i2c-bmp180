#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include "bmp180_ioctl.h"

MODULE_LICENSE("Dual MIT/GPL");

#define DBGMSG(fmt, ...) printk(DEVICE_NAME": "fmt, ##__VA_ARGS__)

/*
 *  Device info
 */
#define DEVICE_NAME  "i2c-bmp180"
#define DEVICE_CLASS "i2c-bmp180-class"
#define CHIP_ID      0x55 // 85

#define SEA_LEVEL_PRES 101325 // Pa

/*
 *  Registers addresses
 */
#define REG_CHIP_ID  0xD0
#define REG_COEF     0xAA
#define REG_CTL_MEAS 0xF4
#define REG_OUT_MEAS 0xF6

/*
 *  Measure control register values
 */
#define MEAS_TEMP  0x2E
#define MEAS_PRES0 0x34
#define MEAS_PRES1 0x74
#define MEAS_PRES2 0xB4
#define MEAS_PRES3 0xF4

#define TIME_TEMP   3 //  2.5 ms
#define TIME_PRES0  3 //  2.5 ms
#define TIME_PRES1  8 //  7.5 ms
#define TIME_PRES2 14 // 13.5 ms
#define TIME_PRES3 26 // 25.5 ms


struct bmp180_coefficients {
    short ac1;
    short ac2;
    short ac3;
    unsigned short ac4;
    unsigned short ac5;
    unsigned short ac6;
    short b1;
    short b2;
    short mb;
    short mc;
    short md;
};



/*
 *  Driver variables
 */
static int deviceMajorNum = 0;
static struct class* deviceClass;
static struct device* deviceHandle;
static int deviceOpenCount = 0;

static struct i2c_client* bmp180_client;
static struct bmp180_coefficients* bmp180_coef;

static char bmp180_oss_meas[] = { MEAS_PRES0, MEAS_PRES1, MEAS_PRES2, MEAS_PRES3 };
static unsigned int bmp180_oss_time[] = { TIME_PRES0, TIME_PRES1, TIME_PRES2, TIME_PRES3 };
static int bmp180_oss = 0;



/*
 *  Driver functions and structures 
 */
static int __init bmp180_init(void);
static void __exit bmp180_exit(void);

static int bmp180_open(struct inode*, struct file*);
static int bmp180_release(struct inode*, struct file*);

static long int bmp180_ioctl(struct file*, unsigned int, unsigned long);

static struct file_operations drv_functions = {
    .owner = THIS_MODULE,
    .open = bmp180_open,
    .release = bmp180_release,
    .unlocked_ioctl = bmp180_ioctl
};



/*
 *  I2C functions and structures
 */
static struct i2c_device_id bmp180_idtable[] = {
    { "bmp180", 85 },
    { }
};
MODULE_DEVICE_TABLE(i2c, bmp180_idtable);

static const struct of_device_id bmp180_of_match[] = {
    { .compatible = "bosch,bmp180" },
    { }
};
MODULE_DEVICE_TABLE(of, bmp180_of_match);

static int bmp180_probe(struct i2c_client*);
static void bmp180_remove(struct i2c_client*);

static struct i2c_driver bmp180_i2c_driver = {
    .driver = {
        .name = "i2c_bmp180",
        .of_match_table = of_match_ptr(bmp180_of_match),
    },
    .id_table = bmp180_idtable,
    .probe_new = bmp180_probe,
    .remove = bmp180_remove,
};



/*
 *  Utility functions
 */
static int bmp180_read_bytes(char, char*, int);
static int bmp180_read_coef(void);
static int bmp180_dbgmsg(void);
static int bmp180_meas_temp(long*);
static int bmp180_meas_pres(long*);
static int bmp180_meas_alti(long*);
static int bmp180_get_oss(int*);
static int bmp180_set_oss(int*);
