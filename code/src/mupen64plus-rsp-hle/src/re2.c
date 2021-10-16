/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - re2.c                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2016 Gilles Siberlin                                    *
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

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include "hle_external.h"
#include "hle_internal.h"
#include "memory.h"

/**************************************************************************
 * Resident evil 2 ucode
 **************************************************************************/
void resize_bilinear_task(struct hle_t* hle)
{
    int data_ptr = *dmem_u32(hle, TASK_UCODE_DATA);

    int src_addr = *dram_u32(hle, data_ptr);
    int dst_addr = *dram_u32(hle, data_ptr + 4);
    int dst_width = *dram_u32(hle, data_ptr + 8);
    int dst_height = *dram_u32(hle, data_ptr + 12);
    int x_ratio = *dram_u32(hle, data_ptr + 16);
    int y_ratio = *dram_u32(hle, data_ptr + 20);
#if 0 /* unused, but keep it for documentation purpose */
    int dst_stride = *dram_u32(hle, data_ptr + 24);
#endif
    int src_offset = *dram_u32(hle, data_ptr + 36);

    int a, b, c ,d, index, y_index, xr, yr, blue, green, red, addr, i, j;
    long long x, y, x_diff, y_diff, one_min_x_diff, one_min_y_diff;
    unsigned short pixel;

    src_addr += (src_offset >> 16) * (320 * 3);
    x = y = 0;

    for(i = 0; i < dst_height; i++)
    {
        yr = (int)(y >> 16);
        y_diff = y - (yr << 16);
        one_min_y_diff = 65536 - y_diff;
        y_index = yr * 320;
        x = 0;

        for(j = 0; j < dst_width; j++)
        {
            xr = (int)(x >> 16);
            x_diff = x - (xr << 16);
            one_min_x_diff = 65536 - x_diff;
            index = y_index + xr;
            addr = src_addr + (index * 3);

            dram_load_u8(hle, (uint8_t*)&a, addr, 3);
            dram_load_u8(hle, (uint8_t*)&b, (addr + 3), 3);
            dram_load_u8(hle, (uint8_t*)&c, (addr + (320 * 3)), 3);
            dram_load_u8(hle, (uint8_t*)&d, (addr + (320 * 3) + 3), 3);

            blue = (int)(((a&0xff)*one_min_x_diff*one_min_y_diff + (b&0xff)*x_diff*one_min_y_diff +
                          (c&0xff)*y_diff*one_min_x_diff         + (d&0xff)*x_diff*y_diff) >> 32);

            green = (int)((((a>>8)&0xff)*one_min_x_diff*one_min_y_diff + ((b>>8)&0xff)*x_diff*one_min_y_diff +
                           ((c>>8)&0xff)*y_diff*one_min_x_diff         + ((d>>8)&0xff)*x_diff*y_diff) >> 32);

            red = (int)((((a>>16)&0xff)*one_min_x_diff*one_min_y_diff + ((b>>16)&0xff)*x_diff*one_min_y_diff +
                         ((c>>16)&0xff)*y_diff*one_min_x_diff         + ((d>>16)&0xff)*x_diff*y_diff) >> 32);

            blue = (blue >> 3) & 0x001f;
            green = (green >> 3) & 0x001f;
            red = (red >> 3) & 0x001f;
            pixel = (red << 11) | (green << 6) | (blue << 1) | 1;

            dram_store_u16(hle, &pixel, dst_addr, 1);
            dst_addr += 2;

            x += x_ratio;
        }
        y += y_ratio;
    }
}
