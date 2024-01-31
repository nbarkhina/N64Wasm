#pragma once
void neil_invoke_linker();
void retro_init(void);
void retro_deinit(void);
bool retro_load_game_new(uint8_t* romdata, int size, bool loadEep, bool loadSra, bool loadFla);
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
bool neil_export_eep();
bool neil_export_sra();
bool neil_export_fla();
void retro_reset_new();
int getRegionTiming();
uint32_t* get_video_buffer();

struct NeilCheats {
    uint32_t address;
    int value;
};

extern struct NeilCheats neilCheats[];
extern int neilCheatsLength;