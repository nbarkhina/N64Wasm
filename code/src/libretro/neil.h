#pragma once
void neil_invoke_linker();
void retro_init(void);
void retro_deinit(void);
bool retro_load_game_new(uint8_t* romdata, int size);
void retro_run(void);
void setDeviceId(int id);
int getReadyToSwap();
void resetReadyToSwap();
int getVI_Count();
void resetVI_Count();
int getVIFPS_Count();
void resetVIFPS_Count();
bool neil_serialize();
bool neil_unserialize();