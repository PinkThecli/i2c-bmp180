#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "bmp180_ioctl.h"

int main()
{
    int device = open("/dev/i2c-bmp180", O_RDWR);

    if ( device < 0 )
    {
        printf("Error: %s\n", strerror(errno));
        return errno;
    }

    printf("----- IOCTL -----\n");

    ioctl(device, IOCTL_BMP180_COEF);
    ioctl(device, IOCTL_BMP180_DBGMSG);

    long temp = 0;
    int res = ioctl(device, IOCTL_BMP180_GET_TEMP, &temp);
    printf("Temp: %.1f 'C    %d bytes\n", (float)(temp*0.1), res);
    
    long pres = 0;
    int oss = -1;
    res = ioctl(device, IOCTL_BMP180_GET_PRES, &pres);
    ioctl(device, IOCTL_BMP180_GET_OSS, &oss);
    printf("Pres: %d Pa ( %.1f mmhg )    %d oss    %d bytes\n", pres, (float)(pres/133.3), oss, res);
    
    long alti = 0;
    res = ioctl(device, IOCTL_BMP180_GET_ALTI, &alti);
    printf("Alti: %.1f m    %d oss    %d bytes\n", (float)(alti*0.1), oss, res);

    oss = 3;
    ioctl(device, IOCTL_BMP180_SET_OSS, &oss);
    pres = 0;
    res = ioctl(device, IOCTL_BMP180_GET_PRES, &pres);
    oss = -1;
    ioctl(device, IOCTL_BMP180_GET_OSS, &oss);
    printf("Pres: %d Pa ( %.1f mmhg )    %d oss    %d bytes\n", pres, (float)(pres/133.3), oss, res);

    alti = 0;
    res = ioctl(device, IOCTL_BMP180_GET_ALTI, &alti);
    printf("Alti: %.1f m    %d oss    %d bytes\n", (float)(alti*0.1), oss, res);

    ioctl(device, IOCTL_BMP180_DBGMSG);

    pause();
    
    close(device);

    return 0;
}
