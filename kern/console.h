#ifndef KERN_CONSOLE_H
#define KERN_CONSOLE_H

#define CGA_BASE 0x3D4
#define CGA_BUF 0xB8000

void cons_init(void);
void cons_putc(int c);
int cons_getc(void);

#endif // KERN_CONSOLE_H
