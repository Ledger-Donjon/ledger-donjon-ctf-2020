/**
 * This file is part of picoHSM
 * 
 * picoHSM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Copyright 2020 Ledger SAS, written by Olivier Hériveaux
 */

#include <stdint.h>
#include <unistd.h>
#include "stm32f205.hxx"
#include "delay.hxx"
#include "w5500.hxx"
#include "usart.hxx"
#include "panic.hxx"
#include "util.hxx"


#define AES_BLOCK_SIZE 16
#define KEY_COUNT 8


enum sec_ins_t {
    SEC_INS_VERIFY_PIN = 1,
    SEC_INS_ENCRYPT = 2,
    SEC_INS_DECRYPT = 3
};


enum sec_status_t {
    SEC_STATUS_OK = 1,
    SEC_STATUS_BAD_PIN = 2,
    SEC_STATUS_KEY_LOCKED = 3
};


w5500_t w5500(&spi1);
usart_t usart_debug_inst;
usart_t usart_sec;
// Mark it volatile to avoid possible compiler optimization (there is no way
// to set it to true, except with a vuln)
volatile bool debug_enabled = false;
// Key which must be set by attackers to unlock 'debug' mode.
// This prevents stealing flags of other attackers if debug_enabled is set to
// true. Only the current attacker should know his own flag.
volatile uint32_t debug_key = 0;

#ifndef HIDE_SECRETS
static const char flag[] = "CTF{Ju5t_A_W4rmUP}";
uint8_t gateway[] = {192, 168, 60, 1};
uint8_t ip[] = {192, 168, 60, 10};
#else
static const char flag[] = "CTF{?????????????}";
uint8_t gateway[] = {192, 168, 0, 1};
uint8_t ip[] = {192, 168, 0, 10};
#endif
uint8_t mask[] = {255, 255, 255, 0};
uint8_t mac[] = {0x00, 0x08, 0xdc, 0x01, 0x02, 0x03};


/**
 * Turn LED ON or OFF.
 *
 * @param state false to turn-off. true to turn-on.
 */
void led(bool state){
    // LED is connected to PB0
    if (state) {
        gpiob.odr &= ~(1 << 0);
    } else {
        gpiob.odr |= (1 << 0);
    }
}


/**
 * Stir a random number from the RNG.
 *
 * @return Random 32 bits value.
 */
uint32_t rand_u32(){
    while ((rng.sr & 1) == 0){}
    return rng.dr;
}


/**
 * Resets the security MCU.
 */
void sec_reset(){
    gpioa.odr &= ~(1 << 11);
    delay(1000000);
    gpioa.odr |= (1 << 11);
    delay(1000000);
}


/**
 * Parse input string to extract the arguments separated by space. Inputs ends
 * with \n, \0 or \r. The input string is modified to place a terminal 0 at the
 * end of each argument. The method will fill a table to point to the arguments.
 *
 * @param buf Input string
 * @param input_size Input size
 * @param args The table of arguments. Will store pointers to characters in
 *     input.
 * @param args_size Maximum number of arguments. Extra arguments are discarded.
 * @return Number of arguments.
 */
size_t parse_args(char* buf, size_t input_size, char** args,
    size_t args_size){

    size_t argc = 0;
    char* token = buf;
    for (size_t i = 0; i < input_size; ++i){
        switch (buf[i]){
            case ' ':
            case '\n':
            case '\r':
                buf[i] = '\0';
            case '\0':
                if (argc < args_size){
                    if (strlen(token) > 0) {
                        args[argc++] = token;
                    }
                    token = buf + i + 1;
                } else {
                    // Early exit. Also useful for making p0wn possible, this
                    // avoids overwriting lr at the end of the shellcode.
                    return argc;
                }
                break;
            default:;
        }
    }
    return argc;
}


/**
 * Process PIN verification.
 *
 * @param pin Input PIN. 8 digits.
 * @return true if PIN is valid.
 */
bool verify_pin(char* pin){
    usart_sec.flush();
    usart_sec.tx(SEC_INS_VERIFY_PIN);
    usart_sec.tx_buf((uint8_t*)pin, 8);
    uint8_t result = usart_sec.rx();
    return (result == SEC_STATUS_OK);
}


/**
 * Verify an hex string is valid and is a multiple of AES block size.
 *
 * @param sock Socket where error messages are printed.
 * @param s Hex string. Null-terminated.
 * @return true if the string is valid.
 */
bool hex_string_valid(socket_t& sock, const char* s, size_t max_bytes){
    size_t l = strlen(s);
    if (l % 32 != 0){
        sock.print("Data size must be a multiple of 16.\n");
        return false;
    }
    if (l / 2 > max_bytes){
        sock.print("Data too long.\n");
        return false;
    }
    for (size_t i = 0; i < l; i += 2){
        uint8_t a;
        if (hex_to_byte(s + i, &a)){
            sock.print("Invalid data format.\n");
            return false;
        }
    }
    return true;
}


/**
 * Execute the encrypt or decrypt command.
 *
 * @param enc true to encrypt, false to decrypt.
 * @param sock A socket object from the W5500.
 * @param args Arguments
 * @param argc Number of arguments
 */
void execute_command_encrypt_decrypt(bool enc, socket_t& sock, char** args,
    size_t argc){

    // Check number of arguments
    if (argc != 3){
        sock.print("Expected 3 arguments.\n");
        return;
    }

    const char* arg_pin = args[0];
    const char* arg_key = args[1];
    const char* arg_data = args[2];

    // Verify PIN format
    if (strlen(arg_pin) != 8){
        sock.print("PIN must have 8 characters.\n");
        return;
    }

    // Get key id.
    uint32_t key_id;
    if (str_to_u32(arg_key, &key_id)){
        sock.print("Invalid key format.\n");
        sock.print(arg_key);
        return;
    }
    if (key_id >= KEY_COUNT){
        sock.print("Key must be in [0, 7].\n");
        return;
    }

    // Parse data as hex string input
    uint8_t buf[256];
    if (!hex_string_valid(sock, arg_data, sizeof(buf)))
        return;
    size_t byte_count;
    hex_to_bytes(arg_data, buf, &byte_count);

    // Transmit to the security MCU the op-code and the PIN.
    // Expect acknowledge after PIN verification
    usart_sec.tx(enc ? SEC_INS_ENCRYPT : SEC_INS_DECRYPT);
    usart_sec.tx_buf((uint8_t*)arg_pin, 8);
    uint8_t ack = usart_sec.rx();
    switch (ack){
        case SEC_STATUS_OK: break;
        case SEC_STATUS_BAD_PIN:
            sock.print("Invalid PIN.\n");
            return;
        default:
            sock.print("Unexpected error.\n");
            return;
    }

    // Transmit key id and expect acknowledge
    usart_sec.tx((uint8_t)key_id);
    ack = usart_sec.rx();
    switch (ack){
        case SEC_STATUS_OK: break;
        case SEC_STATUS_KEY_LOCKED:
            sock.print("Key is locked and cannot be used.\n");
            return;
        default:
            sock.print("Unexpected error.\n");
            return;
    }

    usart_sec.tx((uint8_t)(byte_count / AES_BLOCK_SIZE));
    // For each AES block
    for (size_t i = 0; i < byte_count; i += AES_BLOCK_SIZE){
        usart_sec.tx_buf(buf + i, AES_BLOCK_SIZE);
        uint8_t response[AES_BLOCK_SIZE];
        usart_sec.rx_buf(response, sizeof(response));
        char hex[32];
        bytes_to_hex(response, sizeof(response), hex);
        sock.write((const uint8_t*)hex, 32);
    }
    sock.print("\n");
}


/**
 * Execute a command parsed from the data sent by the client.
 *
 * @param sock A socket object from the W5500.
 * @param args Arguments
 * @param argc Number of arguments
 */
void execute_command(socket_t& sock, char** args, size_t argc){
    const char* command = args[0];

    if (strcmp(command, "help") == 0){
        sock.print(
            "help - print the list of commands.\n"
            "info - print equipment info.\n"
            "getflag [DEBUGKEY] - you already know what this is for...\n"
            "pin - verify pin.\n"
            "encrypt [PIN] [KEYID] [HEX] - encrypt a data blob.\n"
            "decrypt [PIN] [KEYID] [HEX] - decrypt a data blob.\n"
        );
    } else if (!strcmp(command, "info")){
        sock.print(
            "picoHSM v1.0\n"
            "Ledger Donjon CTF 2020\n");
    } else if (!strcmp(command, "getflag")) {
        if (argc == 2) {
            uint32_t key;
            if (str_to_u32(args[1], &key)) {
                sock.print("Invalid key format.\n");
            } else {
                if (debug_enabled && (key == debug_key)){
                    sock.print(flag);
                    sock.print("\n");
                    debug_enabled = false;
                    debug_key = rand_u32();
                } else {
                    sock.print("Debug is not enabled or key is invalid.\n");
                }
            }
        } else {
            sock.print("Expected 2 arguments.\n");
        }
    } else if (!strcmp(command, "encrypt")) {
        execute_command_encrypt_decrypt(true, sock, args+1, argc-1);
    } else if (!strcmp(command, "decrypt")) {
        execute_command_encrypt_decrypt(false, sock, args+1, argc-1);
    } else if (!strcmp(command, "pin")) {
        if (argc == 2) {
            if (strlen(args[1]) == 8) {
                bool valid = verify_pin(args[1]);
                if (valid){
                    sock.print("PIN OK\nYou can validate CTF{Tada!");
                    sock.write((const uint8_t*)args[1], 8);
                    sock.print("}\n");
                } else {
                    sock.print("Invalid PIN.\n");
                }
            } else {
                sock.print("PIN must have 8 characters.\n");
            }
        } else {
            sock.print("Expected 2 arguments.\n");
        }
    } else {
        sock.print("Unknown command. Use help to get help...\n");
    }
}


/**
 * Waits for an incoming connection, process client command, closes the
 * connection and returns.
 *
 * @param sock A socket object from the W5500.
 */
void handle_client(socket_t& sock){
    sock.print(
        "Hello from picoHSM!\n"
        "Waiting for command...\n"
        "Timeout in 15 seconds...\n");
    // TODO: implement timeout
    while (sock.avail() == 0){}
    char buf[768];
    memset(buf, 0, sizeof(buf));
    // Ooops, a wild vuln appears...
    size_t size = sock.read_avail((uint8_t*)buf, sizeof(buf)+256);

    // Parse command to extract arguments separated by ' '.
    const size_t max_args = 8;
    char* args[max_args];
    int argc = parse_args(buf, size, args, max_args);

    execute_command(sock, args, argc);
}


/**
 * Configure flash latency to be compatible with PLL settings.
 */
void configure_flash(){
    flash_acr = 4;
}


/**
 * Configure MC01 prescaler value.
 */
void set_mco1_prescaler(){
    // Keep this method with no parameters, we don't want challengers to call
    // this method to change RCC_CFGR (method call may be too slow).
    rcc.cfgr = (rcc.cfgr & ~(0b111 << 24)) | (0b111 << 24);
}


/**
 * Configure the clock of the system to use the external crystal as the clock
 * source.
 */
void configure_clock(){
    // Enable High Speed External crystal and wait it to be ready.
    rcc.cr |= (1 << 16);
    while (rcc.cr & (1 << 17) == 0){}
    // Disable PLL
    rcc.cr &= ~(1 << 24);
    // HSE = 25 MHz
    // F = (HSE (N / M) / P)
    // Constraints to be respected:
    // 50 <= N <= 432
    // 2 <= M <= 63
    // HSE / M must be in [1, 2] MHz
    // Here P is 2.
    const int n = 64;
    const int m = 16;
    const int q = 3;
    rcc.pllcfgr = m | (n << 6) | (q << 24) |
        (1 << 22); // PLLSRC set to HSE
    // Enable PLL and wait it to be locked
    rcc.cr |= (1 << 24);
    while (rcc.cr & (1 << 25) == 0){}
    // Switch to PLL clock source for the system
    rcc.cfgr = (0b11 << 21) | // PLL on MCO1
        0b10; // Switch to PLL clock source for the system
    set_mco1_prescaler();
}


/**
 * Configure W5500 network controller.
 */
void setup_network(){
    w5500.reset();
    uint8_t version = 0;
    w5500.read(w5500_reg_t::versionr, 0, &version, 1);
    assert(version == 4);

    w5500.set_mac(mac);
    w5500.set_gateway(gateway);
    w5500.set_mask(mask);
    w5500.set_ip(ip);
}


void init_wdg(){
    // Watchdog configuration to prevent players from locking the device
    iwdg.kr = 0x5555; // key to access PR
    iwdg.pr = 0b110; // /256
    iwdg.kr = 0x5555; // key to access RLR
    iwdg.rlr = 2048; // 16 seconds
    iwdg.kr = 0xcccc; // Enable watchdog
    iwdg.kr = 0xaaaa; // Reload counter from RLR
}


int main(){
    // Set vector table address
    scb_vtor = 0x08000000;

    // Clear BSS
    extern uint32_t __bss_start;
    extern uint32_t __bss_end;
    for (uint32_t addr = (uint32_t)&__bss_start; addr < (uint32_t)&__bss_end; ++addr)
        *((uint8_t*)addr) = 0;

    // Call the static initializers functions
    extern void (*__init_array_start)();
    extern void (*__init_array_end)();
    for (void (**p)() = &__init_array_start; p < &__init_array_end; ++p)
        (*p)();

    // Enable clock for PORT A and PORT B peripherals.
    rcc.ahb1enr |= (1 << 1) | (1 << 0);
    // Enable clock for SPI1
    rcc.apb2enr |= (1 << 12);
    // Configure port A.
    // PA0: ethernet interrupt
    // PA1: ethernet reset
    // PA2: USART2 TX
    // PA3: USART2 RX
    // PA4: SPI1 NSS (ethernet)
    // PA5: SPI1 SCK (ethernet)
    // PA6: SPI1 MISO (ethernet)
    // PA7: SPI1 MOSI (ethernet)
    // PA9: USART1 TX
    // PA10: USART1 RX
    gpioa.odr = 0x00000000;
    gpioa.ospeedr =
        (gpio_speed_t::very_high << 20) | // PA10
        (gpio_speed_t::very_high << 18) | // PA9
        (gpio_speed_t::very_high << 16) | // PA7
        (gpio_speed_t::very_high << 14) | // PA7
        (gpio_speed_t::very_high << 12) | // PA6
        (gpio_speed_t::very_high << 10) | // PA5
        (gpio_speed_t::very_high << 8) | // PA4
        (gpio_speed_t::very_high << 6) | // PA3
        (gpio_speed_t::very_high << 4); // PA2
    // PA5 to PA7 select alternate function AF5: SPI1
    // PA9 and PA10 select alternate function AF7: USART1
    // PA2 and PA3 select alternate function AF7: USART2
    gpioa.afrl = (5 << 28) | (5 << 24) | (5 << 20) | (7 << 12) | (7 << 8);
    gpioa.afrh = (7 << 8) | (7 << 4);
    gpioa.moder =
        (gpio_mode_t::general_purpose_output << 22) | // PA11
        (gpio_mode_t::alternate_function << 20) | // PA10
        (gpio_mode_t::alternate_function << 18) | // PA9
        (gpio_mode_t::alternate_function << 16) | // PA8
        (gpio_mode_t::alternate_function << 14) | // PA7
        (gpio_mode_t::alternate_function << 12) | // PA6
        (gpio_mode_t::alternate_function << 10) | // PA5
        (gpio_mode_t::alternate_function << 6) | // PA3
        (gpio_mode_t::alternate_function << 4) | // PA2
        (gpio_mode_t::general_purpose_output << 8) | // PA4
        (gpio_mode_t::general_purpose_output << 2); // PA1
    // Configure Port B.
    // PB0 is connected to the LED.
    gpiob.moder = (0b01 << 0);

    configure_flash();
    configure_clock();
    usart_debug_inst.init(1, 115200);
    usart_debug = &usart_debug_inst;
    debug_println("Booting...");

    // Enable RNG
    rcc.ahb2enr |= (1 << 6);
    // Stir a random number for the debug key
    rng.cr = (1 << 2); // RNGEN
    rand_u32(); // Drop first number as recommended in datasheet
    debug_key = rand_u32();

    usart_sec.init(2, 625000);

    // Enable interrupts for USART1 and USART2
    // Interrupts will fill the ring buffers
    nvic_iser[1] = (1 << 6) | (1 << 5);

    // Configure SPI
    spi1.cr1 = (1 << 6) | (0b100 << 3) | (1 << 2) |
        (1 << 9) | // SSM
        (1 << 8) | // SSI
        (0 << 1) | // Phase
        (0 << 0); // Polarity

    // Turn on LED
    led(true);

    setup_network();
    init_wdg();

    socket_t sock(&w5500, 0);
    for (;;){
        sec_reset();
        usart_sec.flush();
        debug_println("Waiting for connection...");
        if (sock.listen(1234)){
            debug_println("Connection established!");
            iwdg.kr = 0xaaaa; // Reload watchdog
            handle_client(sock);
            debug_println("Client has been served!");
        } else {
            debug_println("Connection failed!");
        }
        sock.disconnect();
        debug_println("Connection closed.");
    }
    for (;;) {}
}
