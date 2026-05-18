OUT = hello
DIR = $(shell pwd)/src
SRCS = $(DIR)/*.c
OBJS = $(SRCS:.c=.o)
HELLO_OBJS = $(DIR)/hello.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: all clean module test dmesg

all: module test

module:
	$(MAKE) -C $(KDIR) M=$(PWD)/src modules

test: $(DIR)/test.c
	gcc -o test_hello $(DIR)/test.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD)/src clean
	rm -f test_hello

dmesg:
	dmesg | tail -20
