obj-m := i2c_bmp180.o

PWD := $(shell pwd)
KERNEL := $(shell uname -r)



all:
	make driver app tree



driver:
	make -C /lib/modules/$(KERNEL)/build M=$(PWD) modules

app: app.c
	gcc app.c -o app

tree:
	dtc -@ -I dts -O dtb -o i2c_bmp180.dtbo i2c_bmp180.dts



clean:
	rm -f *.symvers *.o *.mod *.mod.c *.order .*.cmd

cleanall:
	make -C /lib/modules/$(KERNEL)/build M=$(PWD) clean
	rm -f app
	rm -f i2c_bmp180.dtbo



