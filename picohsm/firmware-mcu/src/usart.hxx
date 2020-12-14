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

#ifndef _USART_HPP_
#define _USART_HPP_


#include <unistd.h>
#include "stm32f205.hxx"
#include "ring_buffer.hxx"


/**
 * USART peripheral helper.
 */
class usart_t {
    public:
        usart_t();
        void init(int, uint32_t);
        void flush();
        void tx(uint8_t);
        void tx_buf(const uint8_t*, size_t);
        uint8_t rx();
        void rx_buf(uint8_t*, size_t);
        void print(const char*);
        void println(const char*);
        void print_i32(int32_t);
        void print_u32(uint32_t);
        size_t avail() const;

    private:
        volatile usart_regs_t* dev;
        ring_buffer_t<256>* buf;
};


extern usart_t* usart_debug;
void debug_print(const char*);
void debug_println(const char*);
void debug_print_i32(int32_t);
void debug_print_u32(uint32_t);


#endif
