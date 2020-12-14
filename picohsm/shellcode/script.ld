PROVIDE(frame_size = 0x308);
PROVIDE(goback = 0x08000724);
INCLUDE symbols.ld
SECTIONS
{
  . = shell_addr;
  .text :
  {
    *(.text*)
    *(.rodata*)
    . = frame_size - 4;
    *(.lr*)
  }
}
