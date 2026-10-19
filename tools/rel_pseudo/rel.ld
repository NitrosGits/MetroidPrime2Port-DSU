ENTRY(_prolog)
SECTIONS {
  . = 0x80800000;
  .text : { *(.text) }
  . = 0x80900000;
  .rodata : { *(.rodata) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  /DISCARD/ : { *(.ctors) *(.dtors) *(.comment) }
}
