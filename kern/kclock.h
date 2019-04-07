#ifndef KERN_KCLOCK_H
#define KERN_KCLOCK_H

#define IO_RTC 0x70 // RTC port

#define MC_NVRAM_START 0xE // Start of NVRAM
#define MC_NVRAM_SIZE 50 // 50 bytes of NVRAM

// NVRAM bytes 7 & 8: base memory size
#define NVRAM_BASELO (MC_NVRAM_START + 7) // Low byte RTC offset 0x15
#define NVRAM_BASEHI (MC_NVRAM_START + 8) // High bytes RTC offset 0x16

// NVRAM bytes 9 & 10: extended memory size (between 1MB and 16MB)
#define NVRAM_EXTLO (MC_NVRAM_START + 9) // Low byte RTC offset 0x17
#define NVRAM_EXTHI (MC_NVRAM_START + 10) // High byte RTC offset 0x18

// NVRAM bytes 38 & 39: extended memory size (between 16MB and 4GB)
#define NVRAM_EXT16LO (MC_NVRAM_START + 38) // Low byte RTC offset 0x34
#define NVRAM_EXT16HI (MC_NVRAM_START + 39) // High byte RTC offset 0x35

unsigned mc146818_read(unsigned reg);
void mc146818_write(unsigned reg, unsigned datum);

#endif // KERN_KCLOCK_H
