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

#include "util.hxx"

int strcmp(const char* s1, const char* s2){
    while (*s1 != '\0' && (*s1 == *s2)){
        s1++; s2++;
    }
    return (*(unsigned char*)s1 - *(unsigned char*)s2);
}

void * memset(void* s, int c, size_t n){
    for (size_t i = 0; i < n; ++i)
        ((uint8_t*)s)[i] = (uint8_t)c;
}

size_t strlen(const char *s){
    size_t l = 0;
    while (*s++ != '\0')
        l++;
    return l;
}

/**
 * Reverse a string in place.
 *
 * @param s String. Null terminated.
 */
void reverse_str(char* s){
    size_t i, j;
    for (i = 0, j = strlen(s)-1; i < j; i++, j--){
        char c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/**
 * Convert an unsigned 32-bits integer to string.
 *
 * @param x Value to convert
 * @param s Destination string. 11 bytes minimum.
 */
void u32_to_str(uint32_t x, char* dest){
    int digit_count = 0;
    do {
        dest[digit_count++] = '0' + (x % 10);
        x /= 10;
    } while (x > 0);
    dest[digit_count] = 0;
    reverse_str(dest);
}


/**
 * Convert a signed 32-bits integer to string.
 *
 * @param x Value to convert
 * @param s Destination string. 12 bytes minimum.
 */
void i32_to_str(int32_t x, char* dest){
    if (x < 0){
        *dest = '-';
        u32_to_str(-(uint32_t)x, dest + 1);
    } else {
        u32_to_str((uint32_t)x, dest);
    }
}


int str_to_u32(const char* s, uint32_t* dest){
    uint32_t result = 0;
    while (*s != 0){
        result *= 10;
        if ((*s < '0') || (*s > '9'))
            return 1;
        result += (*s++) - '0';
    }
    *dest = result;
    return 0;
}


/**
 * Convert a byte to lowercase hexadecimal.
 *
 * @param x Byte
 * @param dest Destination buffer. 2 bytes minimum.
 */
void byte_to_hex(uint8_t x, char* dest){
    const char tab[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
        'a', 'b', 'c', 'd', 'e', 'f'};
    dest[0] = tab[x >> 4];
    dest[1] = tab[x & 0x0f];
}


/**
 * Convert bytes to lowercase hexadecimal.
 *
 * @param bytes Bytes array.
 * @param len Number of bytes in the array.
 * @param s Destination string.
 */
void bytes_to_hex(const uint8_t* bytes, size_t len, char* s){
    for (size_t i = 0; i < len; ++i)
        byte_to_hex(bytes[i], s + i*2);
}


/**
 * Convert an hexacdecimal character to a byte.
 *
 * @param c Character. Can be uppercase or lowercase.
 * @param dest Pointer to a byte where the result is saved.
 * @return 0 in case of success, 1 in case of error.
 */
int hex_char_to_byte(char c, uint8_t* dest){
    if ((c >= '0') && (c <= '9')){
        *dest = c - '0';
        return 0;
    } else if ((c >= 'a') && (c <= 'f')){
        *dest = c - 'a' + 10;
        return 0;
    } else if ((c >= 'A') && (c <= 'F')){
        *dest = c - 'A' + 10;
        return 0;
    } else {
        return 1;
    }
}


/**
 * Convert two hexadecimal characters to a byte.
 *
 * @param s String. Must be two bytes long minimum.
 * @param dest Pointer to a byte where the result is saved.
 * @return 0 in case of success, 1 in case of error.
 */
int hex_to_byte(const char* s, uint8_t* dest){
    uint8_t a, b;
    if (hex_char_to_byte(s[0], &a))
        return 1;
    if (hex_char_to_byte(s[1], &b))
        return 1;
    *dest = (a << 4) | b;
    return 0;
}


/**
 * Convert an hexadecimal string to bytes.
 *
 * @param s String. Null terminated.
 * @param dest Pointer to a buffer where the result is saved.
 * @param dest_len Pointer where the length of the resulting buffer is saved.
 * @return 0 in case of success, 1 in case of error.
 */
int hex_to_bytes(const char* s, uint8_t* dest, size_t* dest_len){
    size_t l = strlen(s);
    if (l % 2 != 0)
        return 1;
    l /= 2;
    for (size_t i = 0; i < l; ++i){
        if (hex_to_byte(s, dest))
            return 1;
        s += 2;
        dest++;
    }
    *dest_len = l;
    return 0;
}
