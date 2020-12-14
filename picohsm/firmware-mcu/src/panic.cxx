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

#include "panic.hxx"
#include "delay.hxx"
#include "stm32f205.hxx"
#include "usart.hxx"


void reset(){
    // Soft-reset by writing SYSRESETREQ in Cortex-M3 AIRCR
    scb_aircr = (0x5fa << 16) | (1 << 2);
    for (;;){}
}


void panic_f(const char* msg){
    debug_println("panic!");
    debug_println(msg);
    delay(100000);
    reset();
}

