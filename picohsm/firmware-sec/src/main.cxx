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

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.hxx"
#include "aes.hxx"
#include "keys.hxx"
#include "pin.hxx"


static const uint8_t* aes_keys[8] = {
    aes_key_0,
    aes_key_1,
    aes_key_2,
    aes_key_3,
    aes_key_4,
    aes_key_5,
    aes_key_6,
    aes_key_7
};


static const bool aes_key_locked[8] = {
    false, false, true, false, false, false, false, false
};


static const uint8_t aes_iv_zero[16] = {0};


enum instruction_t {
    INS_VERIFY_PIN = 1,
    INS_ENCRYPT = 2,
    INS_DECRYPT = 3
};


enum status_t {
    STATUS_OK = 1,
    STATUS_BAD_PIN = 2,
    STATUS_KEY_LOCKED = 3
};


/**
 * Process PIN verification. Reads 8 digits from the UART and compare with the
 * correct PIN. Does not reply on UART.
 *
 * @return 1 if PIN is correct, 0 otherwise.
 */
uint8_t verify_pin(){
    // Get input pin from UART
    uint8_t buffer[8];
    for (uint8_t i = 0; i < 8; ++i)
        buffer[i] = uart_read_u8();
    uint8_t good = 1;
    for (uint8_t i = 0; i < 8; ++i){
        if (pin[i] != buffer[i]){
            good = 0;
            break;
        }
    }
    return good;
}


int main()
{
    DDRA = 1;
    PORTA = 0;

    uart_init(625000);
    sei();

    for (;;)
    {
        PORTA &= ~1; // Turn LED ON
        uint8_t ins = uart_read_u8();
        PORTA |= 1; // Turn LED OFF
        switch (ins) {
            case INS_VERIFY_PIN: {
                // Verify pin
                uint8_t good = verify_pin();
                if (good == 1){
                    uart_write_u8(STATUS_OK);
                } else {
                    uart_write_u8(STATUS_BAD_PIN);
                }
                break;
            }

            case INS_ENCRYPT:
            case INS_DECRYPT: {
                // Verify pin
                uint8_t good = verify_pin();
                if (good){
                    uart_write_u8(STATUS_OK);
                    // Get key number.
                    // Protect with modulo (quick and dirty)
                    uint8_t key_id = uart_read_u8() % 8;
                    // Verify key is enabled
                    if (!aes_key_locked[key_id]){
                        uart_write_u8(STATUS_OK); // Send ack
                        AES_ctx ctx;
                        AES_init_ctx_iv(&ctx, aes_keys[key_id], aes_iv_zero);
                        // Get the number of blocks to be encrypted
                        uint8_t block_count = uart_read_u8();
                        // Encrypt and return on the fly
                        for (uint8_t i = 0; i < block_count; ++i){
                            uint8_t buf[16];
                            uart_read_buf(buf, 16);
                            if (ins == INS_ENCRYPT)
                                AES_CBC_encrypt_buffer(&ctx, buf, 16);
                            else
                                AES_CBC_decrypt_buffer(&ctx, buf, 16);
                            uart_write_buf(buf, 16);
                        }
                    } else {
                        // Send error byte to indicate key is locked
                        uart_write_u8(STATUS_KEY_LOCKED);
                    }
                } else {
                    uart_write_u8(STATUS_BAD_PIN);
                }
                break;
            }

            default:;
        }
    }
}

