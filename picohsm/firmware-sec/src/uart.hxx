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

#ifndef _UART_HXX_
#define _UART_HXX_


#include <avr/io.h>


void uart_ring_buffer_put(uint8_t);
uint8_t uart_ring_buffer_pop();
void uart_write_u8(uint8_t);
void uart_write_u32(uint32_t);
uint8_t uart_read_u8();
void uart_init(uint32_t);
void uart_write_buf(uint8_t*, uint8_t);
void uart_write_str(char*);
void uart_read_buf(uint8_t*, uint8_t);


#endif
