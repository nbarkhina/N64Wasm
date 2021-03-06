#ifndef _LIBRETRO_PRIVATE_H_
#define _LIBRETRO_PRIVATE_H_

#include <stdio.h>
#include <string.h>
#include <boolean.h>
#include <libretro.h>
#include "libretro_perf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int FAKE_SDL_TICKS;

extern retro_get_cpu_features_t perf_get_cpu_features_cb;
extern retro_perf_get_counter_t perf_get_counter_cb;
//extern retro_log_printf_t log_cb;
extern retro_perf_register_t perf_register_cb;
int retro_return(bool just_flipping);

#define SDL_GetTicks() FAKE_SDL_TICKS

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#define strcasecmp stricmp
#endif


#endif
