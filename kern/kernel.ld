OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
  . = 0xF0100000;
  .text : AT(0x100000) {
    *(.text .text.* .gnu.linkonce.t.*)
  }
  PROVIDE(etext = .);
  .rodate : {
    *(.rodate .rodate.* .gnu.linkonce.r.*)
  }
  . = ALIGN(0x1000);
  .data : {
    *(.data)
  }
  .bss : {
    PROVIDE(edata = .);
    *(.bss)
    PROVIDE(end = .);
  }
  /DISCARD/ : {
    *(.eh_frame .note.GNU-stack)
  }
}
