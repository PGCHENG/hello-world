ifneq ($(KERNELRELEASE),)
    obj-m += hello_driver.o
else
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

install:
	sudo insmod hello_driver.ko

uninstall:
	sudo rmmod hello_driver

endif
