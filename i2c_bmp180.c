#include "i2c_bmp180.h"

static int __init bmp180_init(void)
{
    DBGMSG("▼ bmp180_init called\n");

    deviceMajorNum = register_chrdev(0, DEVICE_NAME, &drv_functions);
    if ( deviceMajorNum < 0 )
    {
        DBGMSG("|  register device failed\n");
        DBGMSG("▲ bmp180_init failed\n");
        return deviceMajorNum;
    }
    DBGMSG("|  device major number: %d\n", deviceMajorNum);

    deviceClass = class_create(THIS_MODULE, DEVICE_CLASS);
    if ( deviceClass == NULL )
    {
        DBGMSG("|  create device class failed\n");
        DBGMSG("▲ bmp180_init failed\n");
        unregister_chrdev(deviceMajorNum, DEVICE_NAME);
        return -1;
    }

    deviceHandle = device_create(deviceClass, NULL, MKDEV(deviceMajorNum, 0), NULL, DEVICE_NAME);
    if ( deviceHandle == NULL )
    {
        DBGMSG("|  create device failed\n");
        DBGMSG("▲ bmp180_init failed\n");
        class_destroy(deviceClass);
        unregister_chrdev(deviceMajorNum, DEVICE_NAME);
        return -1;
    }

    bmp180_coef = kmalloc(sizeof(struct bmp180_coefficients), GFP_KERNEL);
    if ( bmp180_coef == NULL )
    {
        DBGMSG("|  kmalloc failed\n");
        DBGMSG("▲ bmp180_init failed\n");
        device_destroy(deviceClass, MKDEV(deviceMajorNum, 0));
        class_destroy(deviceClass);
        unregister_chrdev(deviceMajorNum, DEVICE_NAME);
        return -ENOMEM;
    }
    memset(bmp180_coef, 0, sizeof(struct bmp180_coefficients));

    int res = i2c_add_driver(&bmp180_i2c_driver);
    if ( res < 0 )
    {
        DBGMSG("|  i2c add driver failed\n");
        DBGMSG("▲ bmp180_init failed\n");
        kfree(bmp180_coef);
        device_destroy(deviceClass, MKDEV(deviceMajorNum, 0));
        class_destroy(deviceClass);
        unregister_chrdev(deviceMajorNum, DEVICE_NAME);
        return res;
    }

    if ( bmp180_client == NULL )
    {
        DBGMSG("|  i2c probe failed\n");
        DBGMSG("▲ bmp180_init failed\n");
        i2c_del_driver(&bmp180_i2c_driver);
        kfree(bmp180_coef);
        device_destroy(deviceClass, MKDEV(deviceMajorNum, 0));
        class_destroy(deviceClass);
        unregister_chrdev(deviceMajorNum, DEVICE_NAME);
        return -ENODEV;
    }

    DBGMSG("▲ bmp180_init completed\n");

    return 0;
}
module_init(bmp180_init);

static void __exit bmp180_exit(void)
{
    DBGMSG("▼ bmp180_exit called\n");

    i2c_del_driver(&bmp180_i2c_driver);
    kfree(bmp180_coef);
    device_destroy(deviceClass, MKDEV(deviceMajorNum, 0));
    class_destroy(deviceClass);
    unregister_chrdev(deviceMajorNum, DEVICE_NAME);

    DBGMSG("▲ bmp180_exit completed\n");
}
module_exit(bmp180_exit);

static int bmp180_probe(struct i2c_client* client)
{
    DBGMSG("|  ▼ bmp180_probe called\n");

    char buf[] = { REG_CHIP_ID };
    i2c_master_send(client, buf, 1);
    buf[0] = 0;
    i2c_master_recv(client, buf, 1);
    if (buf[0] != CHIP_ID) {
        DBGMSG("|  |  error chip id: expected %d, received %d\n", CHIP_ID, buf[0]);
        DBGMSG("|  ▲ bmp180_probe filed\n");
        return -ENODEV;
    }

    DBGMSG("|  |  chip id: %d\n", buf[0]);
    bmp180_client = client;
    
    int res = bmp180_read_coef();
    DBGMSG("|  |  %d bytes of calibration coefficients received\n", res);

    DBGMSG("|  ▲ bmp180_probe completed\n");
    return 0;
}

static void bmp180_remove(struct i2c_client* client)
{
    DBGMSG("|  ▼ bmp180_remove called\n");
    memset(bmp180_coef, 0, sizeof(struct bmp180_coefficients));
    DBGMSG("|  ▲ bmp180_remove completed\n");
}

static int bmp180_open(struct inode* pinode, struct file* pfile)
{
    DBGMSG("▼ bmp180_open called\n");
    if ( deviceOpenCount > 0 )
    {
        DBGMSG("▲ bmp180_open failed\n");
        return -EBUSY;
    }
    deviceOpenCount++;
    DBGMSG("▲ bmp180_open completed\n");
    return 0;
}

static int bmp180_release(struct inode* pinode, struct file* pfile)
{
    DBGMSG("▼ bmp180_release called\n");
    deviceOpenCount--;
    DBGMSG("▲ bmp180_release completed\n");
    return 0;
}

static long int bmp180_ioctl(struct file* pfile, unsigned int cmd, unsigned long arg)
{
    DBGMSG("▼ bmp180_ioctl called\n");

    int res = 0;

    switch (cmd)
    {
        case IOCTL_BMP180_GET_TEMP:
            DBGMSG("|  cmd: IOCTL_BMP180_GET_TEMP\n");
            long t = 0;
            res = bmp180_meas_temp(&t);
            copy_to_user((long*)arg, &t, sizeof(long));
            break;

        case IOCTL_BMP180_GET_PRES:
            DBGMSG("|  cmd: IOCTL_BMP180_GET_PRES\n");
            long p = 0;
            res = bmp180_meas_pres(&p);
            copy_to_user((long*)arg, &p, sizeof(long));
            break;

        case IOCTL_BMP180_GET_ALTI:
            DBGMSG("|  cmd: IOCTL_BMP180_GET_ALTI\n");
            long a = 0;
            res = bmp180_meas_alti(&a);
            copy_to_user((long*)arg, &a, sizeof(long));
            break;

        case IOCTL_BMP180_GET_OSS:
            DBGMSG("|  cmd: IOCTL_BMP180_GET_OSS\n");
            res = bmp180_get_oss((int*)arg);
            break;

        case IOCTL_BMP180_SET_OSS:
            DBGMSG("|  cmd: IOCTL_BMP180_SET_OSS\n");
            res = bmp180_set_oss((int*)arg);
            break;

        case IOCTL_BMP180_COEF:
            DBGMSG("|  cmd: IOCTL_BMP180_COEF\n");
            res = bmp180_read_coef();
            break;

        case IOCTL_BMP180_DBGMSG:
            DBGMSG("|  cmd: IOCTL_BMP180_DBGMSG\n");
            res = bmp180_dbgmsg();
            break;
    }

    DBGMSG("▲ bmp180_ioctl completed\n");

    return res;
}

static int bmp180_read_bytes(char addr, char* buf, int count)
{
    i2c_master_send(bmp180_client, &addr, 1);
    return i2c_master_recv(bmp180_client, buf, count);
}

static int bmp180_read_coef()
{
    int res = bmp180_read_bytes(REG_COEF, (char*)bmp180_coef, (int)sizeof(struct bmp180_coefficients));
    for ( int i = 0; i<(int)(sizeof(struct bmp180_coefficients)/2); i++)
    {
        ((unsigned short*)bmp180_coef)[i] = ((unsigned short)(((unsigned short*)bmp180_coef)[i] << 8)) | (((unsigned short*)bmp180_coef)[i] >> 8);
    }
    return res;

}

static int bmp180_dbgmsg()
{
    DBGMSG("|  ▼ counter:\n");
    DBGMSG("|  |  opened devices: %d\n", deviceOpenCount);
    DBGMSG("|  ▼ calibration coefficients:\n");
    DBGMSG("|  |  AC1: %6d\n", bmp180_coef->ac1);
    DBGMSG("|  |  AC2: %6d\n", bmp180_coef->ac2);
    DBGMSG("|  |  AC3: %6d\n", bmp180_coef->ac3);
    DBGMSG("|  |  AC4: %6d\n", bmp180_coef->ac4);
    DBGMSG("|  |  AC5: %6d\n", bmp180_coef->ac5);
    DBGMSG("|  |  AC6: %6d\n", bmp180_coef->ac6);
    DBGMSG("|  |  B1:  %6d\n", bmp180_coef->b1);
    DBGMSG("|  |  B2:  %6d\n", bmp180_coef->b2);
    DBGMSG("|  |  MB:  %6d\n", bmp180_coef->mb);
    DBGMSG("|  |  MC:  %6d\n", bmp180_coef->mc);
    DBGMSG("|  |  MD:  %6d\n", bmp180_coef->md);
    DBGMSG("|  ▼ oversampling setting:\n");
    DBGMSG("|  |  oss: %d\n", bmp180_oss);
    return 0;
}

static int bmp180_meas_temp(long* pt)
{
    char buf[] = { REG_CTL_MEAS, MEAS_TEMP }; 
    i2c_master_send(bmp180_client, buf, 2);
    msleep(TIME_TEMP);
    memset(buf, 0, 2);
    int res = bmp180_read_bytes(REG_OUT_MEAS, buf, 2);
    long ut = (buf[0] << 8) | buf[1];
    long x1 = ((ut - bmp180_coef->ac6) * bmp180_coef->ac5) >> 15;
    long x2 = (bmp180_coef->mc << 11) / (x1 + bmp180_coef->md);
    long b5 = x1 + x2;
    *pt = (b5 + 8) >> 4;
    return res;
}

static int bmp180_meas_pres(long* pp)
{
    char buf_t[] = { REG_CTL_MEAS, MEAS_TEMP }; 
    i2c_master_send(bmp180_client, buf_t, 2);
    msleep(TIME_TEMP);
    memset(buf_t, 0, 2);
    int res = bmp180_read_bytes(REG_OUT_MEAS, buf_t, 2);
    char buf_p[] = { REG_CTL_MEAS, bmp180_oss_meas[bmp180_oss], 0 }; 
    i2c_master_send(bmp180_client, buf_p, 2);
    msleep(bmp180_oss_time[bmp180_oss]);
    memset(buf_p, 0, 3);
    res += bmp180_read_bytes(REG_OUT_MEAS, buf_p, 3);
    long ut = (buf_t[0] << 8) | buf_t[1];
    long up = ((buf_p[0] << 16) | (buf_p[1] << 8) | buf_p[2]) >> (8 - bmp180_oss);
    long x1 = ((ut - bmp180_coef->ac6) * bmp180_coef->ac5) >> 15;
    long x2 = (bmp180_coef->mc << 11) / (x1 + bmp180_coef->md);
    long b5 = x1 + x2;
    long b6 = b5 - 4000;
    x1 = (bmp180_coef->b2 * (b6 * b6 >> 12)) >> 11;
    x2 = bmp180_coef->ac2 * b6 >> 11;
    long x3 = x1 + x2;
    long b3 = (((bmp180_coef->ac1 * 4 + x3) << bmp180_oss) + 2) >> 2;
    x1 = bmp180_coef->ac3 * b6 >> 13;
    x2 = (bmp180_coef->b1 * (b6 * b6 >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    unsigned long b4 = bmp180_coef->ac4 * (unsigned long)(x3 + 32768) >> 15;
    unsigned long b7 = ((unsigned long)up - b3) * (50000 >> bmp180_oss);
    if ( b7 < 0x80000000)
        *pp = (b7 * 2) / b4;
    else
        *pp = (b7 / b4) * 2;
    x1 = (*pp >> 8) * (*pp >> 8);
    x1 = (x1 * 3038) >> 16;
    x2 = (-7357 * (*pp)) >> 16;
    *pp = *pp + ((x1 + x2 + 3791) >> 4);
    return res;
}

/*
 *  Original equation: altitude = 44330 * (1 - (p / p_sea)^(1 / 5.255))
 *  (p / p_sea)^(1 / 5.255) replaced with Taylor series near p_sea
 */
static int bmp180_meas_alti(long* pa)
{
    long p = 0;
    int res = bmp180_meas_pres(&p);
    long a = (1 << 16) * 1000 / 5255;
    long x = ((p - SEA_LEVEL_PRES) << 16) / SEA_LEVEL_PRES;
    long pow1 = (a * x) >> 16;
    long pow2 = ((pow1 * (a - (1 << 16))) * x) >> 32;
    long pow3 = ((pow2 * (a - (2 << 16))) * x) >> 32;
    long pow = (1 << 16) + pow1 + pow2 / 2 + pow3 / 6;
    *pa = (44330 * ((1 << 16) - pow) * 10) >> 16;
    return res;
}

static int bmp180_get_oss(int* poss)
{
    int n = copy_to_user(poss, &bmp180_oss, sizeof(int));
    return sizeof(int) - n;
}

static int bmp180_set_oss(int* poss)
{
    int oss = 0;
    int n = copy_from_user(&oss, poss, sizeof(int));
    if ( oss < 0 )
        bmp180_oss = 0;
    else if ( oss > 3 )
        bmp180_oss = 3;
    else
        bmp180_oss = oss;
    return sizeof(int) - n;
}
