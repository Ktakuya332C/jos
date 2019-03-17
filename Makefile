CFLAGS := -ffreestanding
CFLAGS += -Wall
CFLAGS += -I.

.PHONY: all clean
all: build/boot
clean:
	rm build/*

build/boot: build/boot.o build/main.o
	# bootloader will be loaded at 0x7c00
	ld -Ttext 0x7c00 -e start -o $@ $^
	# It is necessary to extract .text section
	# Otherwise the size of the boootloader exceeds 512 bytes
	objcopy -S -O binary -j .text $@ $@
	# bootloader must be 512 bytes ending with 0xaa55
	perl boot/sign.pl $@

build/boot.o: boot/boot.S
	as -o $@ $^

# It is necessay to add -Os option.
# Otherwise the size of the bootloader exceeds 512 bytes
build/main.o: boot/main.c
	gcc $(CFLAGS) -Os -o $@ -c $^

