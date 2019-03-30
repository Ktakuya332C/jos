CFLAGS := -ffreestanding
CFLAGS += -Wall
CFLAGS += -I.

.PHONY: all clean
all: build/image
clean:
	rm -rf build/

build/image: build/boot/boot build/kern/kernel
	cat $^ > $@

# Specify steps to build build/boot/boot
# from source codes in boot/
include boot/Makefrag

# Specify steps to build build/kern/kernel
# from source codes in kern/ and lib/
include kern/Makefrag

