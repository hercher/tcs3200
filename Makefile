obj-m += tcs3200.o
tcs3200-objs :=  tcs3200_dev.o tcs3200_control.o tcs3200_counter.o

ARM_GCC = /home/flo/workspace/raspberrypi/tools/arm-bcm2708/gcc-linaro-arm-linux-gnueabihf-raspbian/bin/arm-linux-gnueabihf-
KERNEL_BUILD_DIR = /home/flo/workspace/raspberrypi/scratch/kernel
ARCH = arm
CPUS = 6


all:
	make -j $(CPUS) -C $(KERNEL_BUILD_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(ARM_GCC) M=$(PWD) modules

clean:
	make -j $(CPUS) -C $(KERNEL_BUILD_DIR) ARCH=$(ARCH) CROSS_COMPILE=$(ARM_GCC) M=$(PWD) clean
