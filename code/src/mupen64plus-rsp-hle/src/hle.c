/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - hle.c                                           *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 Bobby Smiles                                       *
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
#include <boolean.h>

#include "hle_external.h"
#include "hle_internal.h"
#include "memory.h"
#include "m64p_plugin.h"

#include "ucodes.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

/* some rsp status flags */
#define SP_STATUS_HALT             0x1
#define SP_STATUS_BROKE            0x2
#define SP_STATUS_INTR_ON_BREAK    0x40
#define SP_STATUS_TASKDONE         0x200

/* some rdp status flags */
#define DP_STATUS_FREEZE            0x2

/* some mips interface interrupt flags */
#define MI_INTR_SP                  0x1


/* helper functions prototypes */
static unsigned int sum_bytes(const unsigned char *bytes, unsigned int size);
static void rsp_break(struct hle_t* hle, unsigned int setbits);
static void forward_gfx_task(struct hle_t* hle);
static bool try_fast_audio_dispatching(struct hle_t* hle);
static bool try_fast_task_dispatching(struct hle_t* hle);
static void normal_task_dispatching(struct hle_t* hle);
static void non_task_dispatching(struct hle_t* hle);

extern RSP_INFO rsp_info;

/* local variables */
static const bool FORWARD_AUDIO = false, FORWARD_GFX = true;

/* Global functions */
void hle_init(struct hle_t* hle,
    unsigned char* dram,
    unsigned char* dmem,
    unsigned char* imem,
    unsigned int* mi_intr,
    unsigned int* sp_mem_addr,
    unsigned int* sp_dram_addr,
    unsigned int* sp_rd_length,
    unsigned int* sp_wr_length,
    unsigned int* sp_status,
    unsigned int* sp_dma_full,
    unsigned int* sp_dma_busy,
    unsigned int* sp_pc,
    unsigned int* sp_semaphore,
    unsigned int* dpc_start,
    unsigned int* dpc_end,
    unsigned int* dpc_current,
    unsigned int* dpc_status,
    unsigned int* dpc_clock,
    unsigned int* dpc_bufbusy,
    unsigned int* dpc_pipebusy,
    unsigned int* dpc_tmem,
    void* user_defined)
{
    hle->dram         = dram;
    hle->dmem         = dmem;
    hle->imem         = imem;
    hle->mi_intr      = mi_intr;
    hle->sp_mem_addr  = sp_mem_addr;
    hle->sp_dram_addr = sp_dram_addr;
    hle->sp_rd_length = sp_rd_length;
    hle->sp_wr_length = sp_wr_length;
    hle->sp_status    = sp_status;
    hle->sp_dma_full  = sp_dma_full;
    hle->sp_dma_busy  = sp_dma_busy;
    hle->sp_pc        = sp_pc;
    hle->sp_semaphore = sp_semaphore;
    hle->dpc_start    = dpc_start;
    hle->dpc_end      = dpc_end;
    hle->dpc_current  = dpc_current;
    hle->dpc_status   = dpc_status;
    hle->dpc_clock    = dpc_clock;
    hle->dpc_bufbusy  = dpc_bufbusy;
    hle->dpc_pipebusy = dpc_pipebusy;
    hle->dpc_tmem     = dpc_tmem;
    hle->user_defined = user_defined;
}

/**
 * Try to figure if the RSP was launched using osSpTask* functions
 * and not run directly (in which case DMEM[0xfc0-0xfff] is meaningless).
 *
 * Previously, the ucode_size field was used to determine this,
 * but it is not robust enough (hi Pokemon Stadium !) because games could write anything
 * in this field : most ucode_boot discard the value and just use 0xf7f anyway.
 *
 * Using ucode_boot_size should be more robust in this regard.
 **/
#define is_task(hle) ((*dmem_u32((hle), TASK_UCODE_BOOT_SIZE) <= 0x1000))

void hle_execute(struct hle_t* hle)
{
   if (is_task(hle))
   {
      if (!try_fast_task_dispatching(hle))
         normal_task_dispatching(hle);
      rsp_break(hle, SP_STATUS_TASKDONE);
      return;
   }

   non_task_dispatching(hle);
   rsp_break(hle, 0);
}

/* local functions */
static unsigned int sum_bytes(const unsigned char *bytes, unsigned int size)
{
    unsigned int sum = 0;
    const unsigned char *const bytes_end = bytes + size;

    while (bytes != bytes_end)
        sum += *bytes++;

    return sum;
}


static void rsp_break(struct hle_t* hle, unsigned int setbits)
{
   *hle->sp_status |= setbits | SP_STATUS_BROKE | SP_STATUS_HALT;

   if ((*hle->sp_status & SP_STATUS_INTR_ON_BREAK))
   {
      *hle->mi_intr |= MI_INTR_SP;
      if (rsp_info.CheckInterrupts)
         rsp_info.CheckInterrupts();
   }
}

static void forward_gfx_task(struct hle_t* hle)
{
   if (rsp_info.ProcessDlistList)
      rsp_info.ProcessDlistList();
}

static bool try_fast_audio_dispatching(struct hle_t* hle)
{
    uint32_t v;
    /* identify audio ucode by using the content of ucode_data */
    uint32_t ucode_data = *dmem_u32(hle, TASK_UCODE_DATA);

    if (*dram_u32(hle, ucode_data) == 0x00000001)
    {
        if (*dram_u32(hle, ucode_data + 0x30) == 0xf0000f00)
        {
           v = *dram_u32(hle, ucode_data + 0x28);
           switch(v)
           {
              case 0x1e24138c: /* audio ABI (most common) */
                 alist_process_audio(hle); return true;
              case 0x1dc8138c: /* GoldenEye */
                 alist_process_audio_ge(hle); return true;
              case 0x1e3c1390: /* BlastCorp, DiddyKongRacing */
                 alist_process_audio_bc(hle); return true;
              default:
                 HleWarnMessage(hle->user_defined, "ABI1 identification regression: v=%08x", v);
           }
        }
        else
        {
           v = *dram_u32(hle, ucode_data + 0x10);
           switch(v)
           {
              case 0x11181350: /* MarioKart, WaveRace (E) */
                 alist_process_nead_mk(hle); return true;
              case 0x111812e0: /* StarFox (J) */
                 alist_process_nead_sfj(hle); return true;
              case 0x110412ac: /* WaveRace (J RevB) */
                 alist_process_nead_wrjb(hle); return true;
              case 0x110412cc: /* StarFox/LylatWars (except J) */
                 alist_process_nead_sf(hle); return true;
              case 0x1cd01250: /* FZeroX */
                 alist_process_nead_fz(hle); return true;
              case 0x1f08122c: /* YoshisStory */
                 alist_process_nead_ys(hle); return true;
              case 0x1f38122c: /* 1080° Snowboarding */
                 alist_process_nead_1080(hle); return true;
              case 0x1f681230: /* Zelda OoT / Zelda MM (J, J RevA) */
                 alist_process_nead_oot(hle); return true;
              case 0x1f801250: /* Zelda MM (except J, J RevA, E Beta), PokemonStadium 2 */
                 alist_process_nead_mm(hle); return true;
              case 0x109411f8: /* Zelda MM (E Beta) */
                 alist_process_nead_mmb(hle); return true;
              case 0x1eac11b8: /* AnimalCrossing */
                 alist_process_nead_ac(hle); return true;
              case 0x00010010: /* MusyX v2 (IndianaJones, BattleForNaboo) */
                 musyx_v2_task(hle); return true;

              default:
                 HleWarnMessage(hle->user_defined, "ABI2 identification regression: v=%08x", v);
           }
        }
    }
    else
    {
       v = *dram_u32(hle, ucode_data + 0x10);
       switch(v)
       {
          /* -- MusyX v1 --
             Star Wars: Rogue Squadron
             Resident Evil 2
             Polaris SnoCross
             007: The World Is Not Enough
             Rugrats In Paris
             NBA ShowTime
             Hydro Thunder
             Tarzan
             Gauntlet Legends
             Rush 2049
             */
          case 0x00000001:
             musyx_v1_task(hle);
             return true;
             /* NAUDIO (many games) */
          case 0x0000127c:
             alist_process_naudio(hle);
             return true;
             /* Banjo Kazooie */
          case 0x00001280:
             alist_process_naudio_bk(hle);
             return true;
             /* Donkey Kong 64 */
          case 0x1c58126c:
             alist_process_naudio_dk(hle);
             return true;
             /* Banjo Tooie
              * Jet Force Gemini
              * Mickey's SpeedWay USA
              * Perfect Dark */
          case 0x1ae8143c:
             alist_process_naudio_mp3(hle);
             return true;
          case 0x1ab0140c:
             /* Conker's Bad Fur Day */
             alist_process_naudio_cbfd(hle);
             return true;
          default:
             HleWarnMessage(hle->user_defined, "ABI3 identification regression: v=%08x", v);
       }
    }

    return false;
}

static bool try_fast_task_dispatching(struct hle_t* hle)
{
    /* identify task ucode by its type */
    switch (*dmem_u32(hle, TASK_TYPE))
    {
       case 1:
          /* Resident evil 2 */
          if (*dmem_u32(hle, TASK_DATA_PTR) == 0)
             return false;

          if (FORWARD_GFX)
          {
             forward_gfx_task(hle);
             return true;
          }
          break;

       case 2:
          if (FORWARD_AUDIO)
          {
             if (rsp_info.ProcessAlistList)
                rsp_info.ProcessAlistList();
             return true;
          }
          if (try_fast_audio_dispatching(hle))
             return true;
          break;

       case 7:
          if (rsp_info.ShowCFB)
             rsp_info.ShowCFB();
          return true;
    }

    return false;
}

static void normal_task_dispatching(struct hle_t* hle)
{
   const unsigned int sum =
      sum_bytes((void*)dram_u32(hle,
               *dmem_u32(hle, TASK_UCODE)),
            min(*dmem_u32(hle, TASK_UCODE_SIZE), 0xf80) >> 1);

   switch (sum)
   {
      /* StoreVe12: found in Zelda Ocarina of Time [misleading task->type == 4] */
      case 0x278:
         /* Nothing to emulate */
         return;

         /* GFX: Twintris [misleading task->type == 0] */
      case 0x212ee:
         if (FORWARD_GFX)
         {
            forward_gfx_task(hle);
            return;
         }
         break;

         /* JPEG: found in Pokemon Stadium J */
      case 0x2c85a:
         jpeg_decode_PS0(hle);
         return;

         /* JPEG: found in Zelda Ocarina of Time, Pokemon Stadium 1, Pokemon Stadium 2 */
      case 0x2caa6:
         jpeg_decode_PS(hle);
         return;

         /* JPEG: found in Ogre Battle, Bottom of the 9th */
      case 0x130de:
      case 0x278b0:
         jpeg_decode_OB(hle);
         return;

         /* Resident evil 2 */
      case 0x29a20: /* USA */
      case 0x298c5: /* Europe */
      case 0x298b8: /* USA Rev A */
      case 0x296d9: /* J */
         resize_bilinear_task(hle);
         return;
   }

   HleWarnMessage(hle->user_defined, "unknown OSTask: sum: %x PC:%x", sum, *hle->sp_pc);
}

static void non_task_dispatching(struct hle_t* hle)
{
   const unsigned int sum = sum_bytes(hle->imem, 44);

   if (sum == 0x9e2)
   {
      /* CIC x105 ucode (used during boot of CIC x105 games) */
      cicx105_ucode(hle);
      return;
   }

   HleWarnMessage(hle->user_defined, "unknown RSP code: sum: %x PC:%x", sum, *hle->sp_pc);
}

