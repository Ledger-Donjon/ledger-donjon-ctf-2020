  BITS 64
  DEFAULT REL

  global train_asm:function
  global xor__:function

  section .text

align 4096
train_asm:
  call   [r14]
  movzx  edi,bpl
  movzx  esi,al
  call   [r13]
  jmp    train_asm

align 4096
pad:
  nop

xor__:
  mov    eax,esi
  xor    eax,edi
  ret
