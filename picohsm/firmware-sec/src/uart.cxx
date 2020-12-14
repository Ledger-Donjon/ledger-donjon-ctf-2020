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

#include "uart.hxx"
#include <avr/interrupt.h>


#define UART_RING_BUFFER_SIZE 64
uint8_t uart_ring_buffer[UART_RING_BUFFER_SIZE];
volatile uint8_t uart_ring_write;
volatile uint8_t uart_ring_read;


void uart_ring_buffer_put(uint8_t);
uint8_t uart_ring_buffer_pop();


/**
 * Interrupt for USART byte reception. Store the received byte in the ring
 * buffer. If buffer is full, byte is dropped.
 */
ISR(USART0_RX_vect)
{
    uart_ring_buffer_put(UDR0);
}


void uart_ring_buffer_put(uint8_t data)
{
    uint8_t write_next = (uart_ring_write + 1) % UART_RING_BUFFER_SIZE;
    if (write_next == uart_ring_read)
        /* Buffer is full. Drop byte. */
        return;
    uart_ring_buffer[uart_ring_write] = data;
    uart_ring_write = write_next;
}


uint8_t uart_ring_buffer_pop()
{
    while (uart_ring_read == uart_ring_write){};
    uint8_t value = uart_ring_buffer[uart_ring_read];
    uart_ring_read = (uart_ring_read + 1) % UART_RING_BUFFER_SIZE;
    return value;
}


void uart_write_u8(uint8_t data)
{
    while ((UCSR0A & (1 << UDRE0)) == 0){};
    UCSR0A |= (1 << TXC0);
    UDR0 = data;
}


void uart_write_u32(uint32_t data)
{
    for (uint8_t i = 0; i < 4; ++i)
    {
        uart_write_u8(data & 0xff);
        data >>= 8;
    }
}


uint8_t uart_read_u8()
{
    return uart_ring_buffer_pop();
}


void uart_init(uint32_t baudrate)
{
    /* PD1 (TX) as output pin */
    DDRD |= (1 << 1);
    /* Configure baudrate */
    uint32_t ubrr = (F_CPU / (8 * baudrate)) - 1;
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)(ubrr & 0xff);
    /* Enable receiver and transmiteer.
     * Enable double speed.
     * Enable interrupts on byte reception. */
    UCSR0A = (1 << U2X0);
    UCSR0B = (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0);
    /* Initialize ring buffer */
    uart_ring_write = 0;
    uart_ring_read = 0;
}


void uart_write_buf(uint8_t* buf, uint8_t len){
    for (uint8_t i = 0; i < len; ++i){
        uart_write_u8(buf[i]);
    }
}


void uart_write_str(char* s){
    while (*s != '\0')
        uart_write_u8((uint8_t)(*s++));
}


void uart_read_buf(uint8_t* buf, uint8_t len){
    for (uint8_t i = 0; i < len; ++i){
        buf[i] = uart_read_u8();
    }
}

