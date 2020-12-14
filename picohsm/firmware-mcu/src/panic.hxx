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

#ifndef _PANIC_HXX_
#define _PANIC_HXX_

#include "usart.hxx"

void reset();
void panic_f(const char*);

#define TOSTRING2(x) #x
#define TOSTRING(x) TOSTRING2(x)
#define panic(y) panic_f("" y)
#define assert(c) if (!(c)) { panic("assertion failed"); }

#endif
