/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_audio.c                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
 *   Copyright (C) 2002 Hacktarux                                          *
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

#include <stdint.h>
#include <string.h>
#include <boolean.h>

#include "common.h"

#include "alist.h"
#include "hle_internal.h"
#include "memory.h"

enum
{
   DMEM_BASE = 0x5c0 
};

/* helper functions */
#define get_address(hle, so)  (alist_get_address((hle), (so), (hle)->alist_audio.segments, N_SEGMENTS))
#define set_address(hle, so)  alist_set_address((hle), (so), (hle)->alist_audio.segments, N_SEGMENTS)
#define clear_segments(hle)   \
   (hle)->alist_audio.segments[0] = 0; \
   (hle)->alist_audio.segments[1] = 0; \
   (hle)->alist_audio.segments[2] = 0; \
   (hle)->alist_audio.segments[3] = 0; \
   (hle)->alist_audio.segments[4] = 0; \
   (hle)->alist_audio.segments[5] = 0; \
   (hle)->alist_audio.segments[6] = 0; \
   (hle)->alist_audio.segments[7] = 0; \
   (hle)->alist_audio.segments[8] = 0; \
   (hle)->alist_audio.segments[9] = 0; \
   (hle)->alist_audio.segments[10] = 0; \
   (hle)->alist_audio.segments[11] = 0; \
   (hle)->alist_audio.segments[12] = 0; \
   (hle)->alist_audio.segments[13] = 0; \
   (hle)->alist_audio.segments[14] = 0; \
   (hle)->alist_audio.segments[15] = 0

/* audio commands definition */
static void SPNOOP(struct hle_t* UNUSED(hle), uint32_t UNUSED(w1), uint32_t UNUSED(w2))
{
}

static void CLEARBUFF(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint16_t dmem  = w1 + DMEM_BASE;
   uint16_t count = w2 & 0xfff;

   if (count != 0)
      alist_clear(hle, dmem, align(count, 16));
}

static void ENVMIXER(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint8_t  flags   = (w1 >> 16);
   uint32_t address = get_address(hle, w2);

   alist_envmix_exp(
         hle,
         flags & A_INIT,
         flags & A_AUX,
         hle->alist_audio.out, hle->alist_audio.dry_right,
         hle->alist_audio.wet_left, hle->alist_audio.wet_right,
         hle->alist_audio.in, hle->alist_audio.count,
         hle->alist_audio.dry, hle->alist_audio.wet,
         hle->alist_audio.vol,
         hle->alist_audio.target,
         hle->alist_audio.rate,
         address);
}

static void ENVMIXER_GE(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint8_t  flags   = (w1 >> 16);
   uint32_t address = get_address(hle, w2);

   alist_envmix_ge(
         hle,
         flags & A_INIT,
         flags & A_AUX,
         hle->alist_audio.out, hle->alist_audio.dry_right,
         hle->alist_audio.wet_left, hle->alist_audio.wet_right,
         hle->alist_audio.in, hle->alist_audio.count,
         hle->alist_audio.dry, hle->alist_audio.wet,
         hle->alist_audio.vol,
         hle->alist_audio.target,
         hle->alist_audio.rate,
         address);
}

static void RESAMPLE(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint8_t  flags   = (w1 >> 16);
   uint16_t pitch   = w1;
   uint32_t address = get_address(hle, w2);

   alist_resample(
         hle,
         flags & 0x1,
         flags & 0x2,
         hle->alist_audio.out,
         hle->alist_audio.in,
         align(hle->alist_audio.count, 16),
         pitch << 1,
         address);
}

static void SETVOL(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   unsigned lr;
   uint8_t flags = (w1 >> 16);

   if (!hle)
      return;

   if (flags & A_AUX)
   {
      hle->alist_audio.dry = w1;
      hle->alist_audio.wet = w2;
      return;
   }

   lr = (flags & A_LEFT) ? 0 : 1;

   if (flags & A_VOL)
      hle->alist_audio.vol[lr]    = w1;
   else
   {
      hle->alist_audio.target[lr] = w1;
      hle->alist_audio.rate[lr]   = w2;
   }
}

static void SETLOOP(struct hle_t* hle, uint32_t UNUSED(w1), uint32_t w2)
{
   if (!hle)
      return;

   hle->alist_audio.loop = get_address(hle, w2);
}

static void ADPCM(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint8_t  flags   = (w1 >> 16);
   uint32_t address = get_address(hle, w2);

   if (!hle)
      return;

   alist_adpcm(
         hle,
         flags & 0x1,
         flags & 0x2,
         false,          /* unsupported in this ucode */
         hle->alist_audio.out,
         hle->alist_audio.in,
         align(hle->alist_audio.count, 32),
         hle->alist_audio.table,
         hle->alist_audio.loop,
         address);
}

static void LOADBUFF(struct hle_t* hle, uint32_t UNUSED(w1), uint32_t w2)
{
   uint32_t address = get_address(hle, w2);

   if (hle->alist_audio.count != 0)
      alist_load(hle, hle->alist_audio.in, address, hle->alist_audio.count);
}

static void SAVEBUFF(struct hle_t* hle, uint32_t UNUSED(w1), uint32_t w2)
{
   uint32_t address = get_address(hle, w2);

   if (hle->alist_audio.count != 0)
      alist_save(hle, hle->alist_audio.out, address, hle->alist_audio.count);
}

static void SETBUFF(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & A_AUX)
    {
        hle->alist_audio.dry_right = w1 + DMEM_BASE;
        hle->alist_audio.wet_left  = (w2 >> 16) + DMEM_BASE;
        hle->alist_audio.wet_right = w2 + DMEM_BASE;
    }
    else
    {
        hle->alist_audio.in    = w1 + DMEM_BASE;
        hle->alist_audio.out   = (w2 >> 16) + DMEM_BASE;
        hle->alist_audio.count = w2;
    }
}

static void DMEMMOVE(struct hle_t* hle, uint32_t w1, uint32_t count)
{
   uint16_t dmemi = w1 + DMEM_BASE;
   uint16_t dmemo = (count >> 16) + DMEM_BASE;

   if (count != 0)
      alist_move(hle, dmemo, dmemi, align(count, 16));
}

static void LOADADPCM(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint16_t count   = w1;
   uint32_t address = get_address(hle, w2);

   if (!hle)
      return;

   dram_load_u16(hle, (uint16_t*)hle->alist_audio.table, address, align(count, 8) >> 1);
}

static void INTERLEAVE(struct hle_t* hle, uint32_t UNUSED(w1), uint32_t w2)
{
   uint16_t left  = (w2 >> 16) + DMEM_BASE;
   uint16_t right = w2 + DMEM_BASE;

   if (hle->alist_audio.count != 0)
      alist_interleave(hle, hle->alist_audio.out,
            left, right, align(hle->alist_audio.count, 16));
}

static void MIXER(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   int16_t  gain  = w1;
   uint16_t dmemi = (w2 >> 16) + DMEM_BASE;
   uint16_t dmemo = w2 + DMEM_BASE;

   if (hle->alist_audio.count != 0)
      alist_mix(hle, dmemo, dmemi, align(hle->alist_audio.count, 32), gain);
}

static void SEGMENT(struct hle_t* hle, uint32_t UNUSED(w1), uint32_t w2)
{
    set_address(hle, w2);
}

static void POLEF(struct hle_t* hle, uint32_t w1, uint32_t w2)
{
   uint8_t  flags   = (w1 >> 16);
   uint16_t gain    = w1;
   uint32_t address = get_address(hle, w2);

   if (hle->alist_audio.count != 0)
      alist_polef(
            hle,
            flags & A_INIT,
            hle->alist_audio.out,
            hle->alist_audio.in,
            align(hle->alist_audio.count, 16),
            gain,
            hle->alist_audio.table,
            address);
}

/* global functions */
void alist_process_audio(struct hle_t* hle)
{
   static const acmd_callback_t ABI[0x10] = {
      SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
      LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
      SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
      MIXER,          INTERLEAVE,     POLEF,          SETLOOP
   };

   clear_segments(hle);
   alist_process(hle, ABI, 0x10);
}

void alist_process_audio_ge(struct hle_t* hle)
{
   static const acmd_callback_t ABI[0x10] = {
      SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER_GE,
      LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
      SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
      MIXER,          INTERLEAVE,     POLEF,          SETLOOP
   };

   clear_segments(hle);
   alist_process(hle, ABI, 0x10);
}

void alist_process_audio_bc(struct hle_t* hle)
{
   static const acmd_callback_t ABI[0x10] = {
      SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER_GE,
      LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
      SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
      MIXER,          INTERLEAVE,     POLEF,          SETLOOP
   };

   clear_segments(hle);
   alist_process(hle, ABI, 0x10);
}
