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

#include "usart.hxx"
#include "panic.hxx"
#include "system.hxx"
#include "util.hxx"


usart_t* usart_debug = 0;
ring_buffer_t<256> usart1_rx_buf;
ring_buffer_t<256> usart2_rx_buf;


extern "C" void usart1_handler(){
    if (usart1.sr & (1 << 5))
        usart1_rx_buf.put(usart1.dr);
}


extern "C" void usart2_handler(){
    if (usart2.sr & (1 << 5))
        usart2_rx_buf.put(usart2.dr);
}


/**
 * Default constructor.
 *
 * USART is not initialized.
 */
usart_t::usart_t():
    dev(0){}


/**
 * Initializes the UART.
 *
 * This does not enable and configure GPIOs.
 *
 * @param no USART number. 1 for USART1, 2 for USART2...
 * @param baudrate USART baudrate.
 */
void usart_t::init(int no, uint32_t baudrate){
    assert(dev == 0);
    switch (no){
        case 1:
            dev = &usart1;
            buf = &usart1_rx_buf;
            rcc.apb2enr |= (1 << 4); // Enable USART1
            rcc.apb2rstr |= (1 << 4); // Reset USART1
            rcc.apb2rstr &= ~(1 << 4);
            break;
        case 2:
            dev = &usart2;
            buf = &usart2_rx_buf;
            rcc.apb1enr |= (1 << 17); // Enable USART2
            rcc.apb1rstr |= (1 << 17); // Reset USART2
            rcc.apb1rstr &= ~(1 << 17);
            break;
        default: panic();
    }
    dev->cr1 = (1 << 13) | (1 << 3) | (1 << 2);
    dev->cr2 = (3 << 12);
    // Baudrate is Fck/(8*(2-OVER8)*DIV)
    // OVER8 = 0
    // DIV = BRR/16
    // So here: BRR = Fck/Baudrate
    dev->brr = sys_freq / baudrate;
    buf->flush();
    // Enable interrupt
    dev->cr1 |= (1 << 5); // RXNEIE
}


/**
 * Drop all received bytes.
 */
void usart_t::flush(){
    buf->flush();
}


/**
 * Send a byte. Blocks until the transceiver is ready to send.
 *
 * @param byte Byte to be sent.
 */
void usart_t::tx(uint8_t byte){
    assert(dev);
    while ((dev->sr & (1 << 6)) == 0){}
    dev->dr = byte;
}


/**
 * Send a buffer.
 *
 * @param buf Input buffer.
 * @param len Buffer length.
 */
void usart_t::tx_buf(const uint8_t* buf, size_t len){
    for (size_t i = 0; i < len; ++i)
        tx(buf[i]);
}


/**
 * Receive a byte. Blocks until a byte has been received.
 *
 * @return Received byte.
 */
uint8_t usart_t::rx(){
    while (!buf->has_data()){}
    return buf->pop();
}


/**
 * Receives many bytes. Blocks until all bytes have been received.
 *
 * @param dest A buffer where the received bytes are written.
 * @param len Number of bytes to be received.
 */
void usart_t::rx_buf(uint8_t* buf, size_t len){
    for (size_t i = 0; i < len; ++i)
        buf[i] = rx();
}


/**
 * Print a null terminated string.
 *
 * @param s Input string.
 */
void usart_t::print(const char* s){
    while (*s != 0)
        tx((uint8_t)(*s++));
}


/**
 * Print a null terminated string and a line return.
 *
 * @param s Input string.
 */
void usart_t::println(const char* s){
    print(s);
    tx((uint8_t)'\n');
}


/**
 * Print a signed 32-bits number.
 *
 * @param x Value to be printed.
 */
void usart_t::print_i32(int32_t x){
    char s[12];
    i32_to_str(x, s);
    print(s);
}


/**
 * Print an unsigned 32-bits number.
 *
 * @param x Value to be printed.
 */
void usart_t::print_u32(uint32_t x){
    char s[11];
    u32_to_str(x, s);
    print(s);
}


/**
 * @return Number of bytes available to be read.
 */
size_t usart_t::avail() const {
    return buf->size();
}


/**
 * Print a null terminated string to the debug USART (if defined).
 *
 * @param s Input string.
 */
void debug_print(const char* s){
    if (usart_debug)
        usart_debug->print(s);
}


/**
 * Print a null terminated string to the debug USART (if defined) and a line
 * return.
 *
 * @param s Input string.
 */
void debug_println(const char* s){
    if (usart_debug)
        usart_debug->println(s);
}


/**
 * Print a signed 32-bits number to the debug USART (if defined).
 *
 * @param x Value to be printed.
 */
void debug_print_i32(int32_t x){
    if (usart_debug)
        usart_debug->print_i32(x);
}


/**
 * Print an unsigned 32-bits number to the debug USART (if defined).
 *
 * @param x Value to be printed.
 */
void debug_print_u32(uint32_t x){
    if (usart_debug)
        usart_debug->print_u32(x);
}
