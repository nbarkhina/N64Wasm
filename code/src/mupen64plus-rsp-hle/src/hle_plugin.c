/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - plugin.c                                        *
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

#include <stdarg.h>
#include <stdio.h>

#include "common.h"
#include "hle.h"

#define M64P_PLUGIN_PROTOTYPES 1
#include "m64p_types.h"
#include "m64p_common.h"
#include "m64p_plugin.h"

#define RSP_HLE_VERSION        0x020000
#define RSP_PLUGIN_API_VERSION 0x020000

/* local variables */
static struct hle_t g_hle;

static unsigned l_PluginInit = 0;

/* local function */
static void DebugMessage(int level, const char *message, va_list args)
{
    char msgbuf[1024];

    vsprintf(msgbuf, message, args);
}

/* Global functions needed by HLE core */
void HleVerboseMessage(void* UNUSED(user_defined), const char *message, ...)
{
    va_list args;
    va_start(args, message);
    DebugMessage(M64MSG_VERBOSE, message, args);
    va_end(args);
}

void HleWarnMessage(void* UNUSED(user_defined), const char *message, ...)
{
    va_list args;
    va_start(args, message);
    DebugMessage(M64MSG_WARNING, message, args);
    va_end(args);
}

/* DLL-exported functions */
m64p_error hlePluginStartup(m64p_dynlib_handle UNUSED(CoreLibHandle), void *Context, void (*DebugCallback)(void *, int, const char *))
{
   l_PluginInit = 1;
   return M64ERR_SUCCESS;
}

m64p_error hlePluginShutdown(void)
{
   l_PluginInit = 0;
   return M64ERR_SUCCESS;
}

m64p_error hlePluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
    /* set version info */
    if (PluginType != NULL)
        *PluginType = M64PLUGIN_RSP;

    if (PluginVersion != NULL)
        *PluginVersion = RSP_HLE_VERSION;

    if (APIVersion != NULL)
        *APIVersion = RSP_PLUGIN_API_VERSION;

    if (PluginNamePtr != NULL)
        *PluginNamePtr = "Hacktarux/Azimer High-Level Emulation RSP Plugin";

    if (Capabilities != NULL)
        *Capabilities = 0;

    return M64ERR_SUCCESS;
}

extern RSP_INFO rsp_info;

void hleInitiateRSP(RSP_INFO Rsp_Info, unsigned int* UNUSED(CycleCount))
{
    hle_init(&g_hle,
             rsp_info.RDRAM,
             rsp_info.DMEM,
             rsp_info.IMEM,
             rsp_info.MI_INTR_REG,
             rsp_info.SP_MEM_ADDR_REG,
             rsp_info.SP_DRAM_ADDR_REG,
             rsp_info.SP_RD_LEN_REG,
             rsp_info.SP_WR_LEN_REG,
             rsp_info.SP_STATUS_REG,
             rsp_info.SP_DMA_FULL_REG,
             rsp_info.SP_DMA_BUSY_REG,
             rsp_info.SP_PC_REG,
             rsp_info.SP_SEMAPHORE_REG,
             rsp_info.DPC_START_REG,
             rsp_info.DPC_END_REG,
             rsp_info.DPC_CURRENT_REG,
             rsp_info.DPC_STATUS_REG,
             rsp_info.DPC_CLOCK_REG,
             rsp_info.DPC_BUFBUSY_REG,
             rsp_info.DPC_PIPEBUSY_REG,
             rsp_info.DPC_TMEM_REG,
             NULL);

    l_PluginInit = 1;
}

unsigned int hleDoRspCycles(unsigned int Cycles)
{
   if (!l_PluginInit)
      hleInitiateRSP(rsp_info, NULL);

   hle_execute(&g_hle);
   return Cycles;
}


void hleRomClosed(void)
{
   /* do nothing */
}
