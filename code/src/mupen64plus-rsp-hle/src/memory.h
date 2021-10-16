/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - memory.h                                        *
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

#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <retro_inline.h>

#include "hle_internal.h"

#ifdef MSB_FIRST
#define S 0
#define S16 0
#define S8 0
#else
#define S 1
#define S16 2
#define S8 3
#endif

enum {
    TASK_TYPE               = 0xfc0,
    TASK_FLAGS              = 0xfc4,
    TASK_UCODE_BOOT         = 0xfc8,
    TASK_UCODE_BOOT_SIZE    = 0xfcc,
    TASK_UCODE              = 0xfd0,
    TASK_UCODE_SIZE         = 0xfd4,
    TASK_UCODE_DATA         = 0xfd8,
    TASK_UCODE_DATA_SIZE    = 0xfdc,
    TASK_DRAM_STACK         = 0xfe0,
    TASK_DRAM_STACK_SIZE    = 0xfe4,
    TASK_OUTPUT_BUFF        = 0xfe8,
    TASK_OUTPUT_BUFF_SIZE   = 0xfec,
    TASK_DATA_PTR           = 0xff0,
    TASK_DATA_SIZE          = 0xff4,
    TASK_YIELD_DATA_PTR     = 0xff8,
    TASK_YIELD_DATA_SIZE    = 0xffc
};

static INLINE unsigned int align(unsigned int x, unsigned amount)
{
   --amount;
   return (x + amount) & ~amount;
}

#define u8(buffer, address)   ((uint8_t*)((buffer) + ((address) ^ S8)))
#define u16(buffer, address)  ((uint16_t*)((buffer) + ((address) ^ S16)))
#define u32(buffer, address)  ((uint32_t*)((buffer) + (address)))

void load_u8 (uint8_t*  dst, const unsigned char* buffer, unsigned address, size_t count);
void load_u16(uint16_t* dst, const unsigned char* buffer, unsigned address, size_t count);

#define load_u32(dst, buffer, address, count)   memcpy((dst), u32((buffer), (address)), (count) * sizeof(uint32_t))

void store_u16(unsigned char* buffer, unsigned address, const uint16_t* src, size_t count);
void store_u32(unsigned char* buffer, unsigned address, const uint32_t* src, size_t count);

#define store_u32(buffer, address, src, count)  memcpy(u32((buffer), (address)), (src), (count) * sizeof(uint32_t))

/* convenient functions for DMEM access */
#define dmem_u8(hle, address)    (u8((hle)->dmem,  (address) & 0xFFF)
#define dmem_u16(hle, address)   (u16((hle)->dmem, (address) & 0xfff);
#define dmem_u32(hle, address)   (u32((hle)->dmem, (address) & 0xfff))
#define dmem_load_u8(hle, dst, address, count)  load_u8((dst),  (hle)->dmem, (address) & 0xfff, (count))
#define dmem_load_u16(hle, dst, address, count)  load_u16((dst), (hle)->dmem, (address) & 0xfff, (count))
#define dmem_load_u32(hle, dst, address, count)  load_u32((dst), (hle)->dmem, (address) & 0xfff, (count))
#define dmem_store_u16(hle, src, address, count) store_u16((hle)->dmem, (address) & 0xfff, (src), (count))
#define dmem_store_u32(hle, src, address, count) store_u32((hle)->dmem, (address) & 0xfff, (src), (count))

/* convenient functions DRAM access */
#define dram_u8(hle, address)    (u8((hle)->dram, (address) & 0xffffff))
#define dram_u16(hle, address)   (u16((hle)->dram, (address) & 0xffffff))
#define dram_u32(hle, address)   (u32((hle)->dram, (address) & 0xffffff))

#define dram_load_u8(hle, dst, address, count)   load_u8((dst), (hle)->dram, (address) & 0xffffff, (count))
#define dram_load_u16(hle, dst, address, count)  load_u16((dst), (hle)->dram, (address) & 0xffffff, (count))
#define dram_load_u32(hle, dst, address, count)  load_u32((dst), (hle)->dram, (address) & 0xffffff, (count))
#define dram_store_u8(hle, src, address, count)  store_u8((hle)->dram,  (address) & 0xffffff, (src), (count))
#define dram_store_u16(hle, src, address, count) store_u16((hle)->dram, (address) & 0xffffff, (src), (count))
#define dram_store_u32(hle, src, address, count) store_u32((hle)->dram, (address) & 0xffffff, (src), (count))

#endif

