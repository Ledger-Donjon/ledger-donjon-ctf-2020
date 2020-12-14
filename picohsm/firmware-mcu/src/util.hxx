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

#ifndef _UTIL_HXX_
#define _UTIL_HXX_

#include <unistd.h>

template <typename T> T min(T a, T b){
    return a < b ? a : b;
}

template <typename T> T max(T a, T b){
    return a > b ? a : b;
}

int strcmp(const char*, const char*);
void * memset(void*, int, size_t);
size_t strlen(const char*);
void reverse_str(char*);
void u32_to_str(uint32_t, char*);
void i32_to_str(int32_t, char*);
int str_to_u32(const char*, uint32_t*);
void byte_to_hex(uint8_t, char*);
void bytes_to_hex(const uint8_t*, size_t, char*);
int hex_char_to_byte(char, uint8_t*);
int hex_to_byte(const char*, uint8_t*);
int hex_to_bytes(const char*, uint8_t*, size_t*);

#endif
