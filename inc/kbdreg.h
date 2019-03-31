#ifndef INC_KBDREG_H
#define INC_KBDREG_H

// Special keycodes
#define KEY_HOME 0xE0
#define KEY_END 0xE1
#define KEY_UP 0xE2
#define KEY_DN 0xE3
#define KEY_LF 0xE4
#define KEY_RT 0xE5
#define KEY_PGUP 0xE6
#define KEY_PGDN 0xE7
#define KEY_INS 0xE8
#define KEY_DEL 0xE9

#define KBDATAP 0x60 // KeyBoard DATA Port

#define KBSTATP 0x64 // KeyBoard STATus Port
#define  KBS_DIB 0x1
#define  KBS_IBF 0x2
#define  KBS_WARM 0x4
#define  KBS_OCMD 0x8
#define  KBS_NOSEC 0x10
#define  KBS_TERR 0x20
#define  KBS_RERR 0x40
#define  KBS_PERR 0x80

#endif // INC_KBDREG_H
