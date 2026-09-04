obj-m += imx6ull_device.o

KDIR := /home/wingcun/imx6ull/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek
CROSS := /home/wingcun/toolchain/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

ccflags-y += -I$(src)

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) \
		M=$(CURDIR) \
		ARCH=arm \
		CROSS_COMPILE=$(CROSS) \
		LOCALVERSION=-g3dc0a4b \
		modules

clean:
	$(MAKE) -C $(KDIR) \
		M=$(CURDIR) \
		ARCH=arm \
		CROSS_COMPILE=$(CROSS) \
		clean