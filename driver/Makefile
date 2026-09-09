obj-m += imx6ull_device.o
obj-m += platform_probe.o

KDIR := /home/wingcun/imx6ull/linux-imx-rel_imx_4.1.15_2.1.0_ga_alientek
CROSS := /home/wingcun/toolchain/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
DTC := dtc
CPP := cpp
DTS_INCLUDE := -I$(KDIR)/arch/arm/boot/dts -I$(KDIR)/include

ccflags-y += -I$(src)

.PHONY: all clean dtb

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

# dtb compile check for the example device tree node
#
# Example usage:
#   make -C driver dtb

dtb: platform_probe.dtb

platform_probe.dtb: platform_probe.dts
	$(CPP) -nostdinc -undef -x assembler-with-cpp \
		$(DTS_INCLUDE) \
		$< > $@.pre.dts
	$(DTC) -I dts -O dtb -o $@ $@.pre.dts
	rm -f $@.pre.dts