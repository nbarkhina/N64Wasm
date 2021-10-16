/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - memory.c                                        *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <string.h>

#include "memory.h"

/* Global functions */
void load_u8(uint8_t* dst, const unsigned char* buffer, unsigned address, size_t count)
{
   while (count)
   {
      *(dst++) = *u8(buffer, address);
      address += 1;
      --count;
   }
}

void load_u16(uint16_t* dst, const unsigned char* buffer, unsigned address, size_t count)
{
   while (count)
   {
      *(dst++) = *u16(buffer, address);
      address += 2;
      --count;
   }
}

void store_u16(unsigned char* buffer, unsigned address, const uint16_t* src, size_t count)
{
   while (count)
   {
      *u16(buffer, address) = *(src++);
      address += 2;
      --count;
   }
}
