#include <stdint.h>

void shell_main(){
    uint32_t* addr_a = 0x20002000;
    uint32_t* addr_b = 0xe1002000;
    uint32_t i = 0;
    while (i < 0x2000) {
        if (*addr_a == *addr_b){
            debug_print("==");
            debug_print_u32((uint32_t)addr_a);
            debug_println("");
        } else {
            debug_print("!!");
            debug_print_u32((uint32_t)addr_a);
            debug_println("");
        }
        addr_a++;
        addr_b++;
        i += 4;
    }
}
