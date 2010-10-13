
obj-m := proj01-iosched.o

KERNELDIR ?= /root/g8/linux-2.6.34.7/
PWD       := $(shell pwd)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -rf .tmp_versions


