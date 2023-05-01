# Bosch bmp180 driver
## Description
Raspberry Pi I2C driver for bmp180. Study project.  
Tested on Raspbian 6.1.21-v8+ (hash: 0afb5e98488aed7017b9bf321b575d0177feb7ed).


## Build
To build all artifacts use:
```sh
make all clean
```

To build only kernel module, device tree overlay or test app use one of the following:
```sh
make driver clean
```
```sh
make tree
```
```sh
make app
```

To delete all files except artifacts and sources use:
```sh
make clean
```

To delete all files except sources use:
```sh
make cleanall
```

## Install
To apply device tree overlay (if bmp180 not already in device tree) use:
```sh
sudo dtoverlay i2c_bmp180.dtbo
```

To remove device tree overlay reboot or use:
```sh
sudo dtoverlay -r i2c_bmp180
```

To install module use:
```sh
sudo insmod i2c_bmp180.ko
```

To remove module use:
```sh
sudo rmmod i2c_bmp180
```
