CFLAGS := -ffreestanding
CFLAGS += -Wall
CFLAGS += -I.

.PHONY: all clean
all: build/image
clean:
	rm build/*

build/image: build/boot build/kernel
	cat $^ > $@

build/boot: build/boot.o build/main.o
	# bootloader will be loaded at 0x7c00
	ld -Ttext 0x7c00 -e start -o $@ $^
	# It is necessary to extract .text section
	# Otherwise the size of the boootloader exceeds 512 bytes
	objcopy -S -O binary -j .text $@ $@
	# bootloader must be 512 bytes ending with 0xaa55
	perl boot/sign.pl $@

build/boot.o: boot/boot.S
	gcc $(CFLAGS) -o $@ -c $^

# It is necessay to add -Os option.
# Otherwise the size of the bootloader exceeds 512 bytes
build/main.o: boot/main.c
	gcc $(CFLAGS) -Os -o $@ -c $^

build/kernel: build/entry.o build/entrypgdir.o kern/kernel.ld
	ld -T kern/kernel.ld -nostdlib -o $@ $^

build/entry.o: kern/entry.S
	gcc $(CFLAGS) -o $@ -c $^

build/entrypgdir.o: kern/entrypgdir.c
	gcc $(CFLAGS) -o $@ -c $^

