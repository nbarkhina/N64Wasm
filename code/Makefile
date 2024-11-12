MAKEFLAGS += --no-builtin-rules

CFILES := src/glide2gl/src/Glide64/Combine.c \
    src/glide2gl/src/Glide64/Framebuffer_glide64.c \
    src/glide2gl/src/Glide64/glide64_3dmath.c \
    src/glide2gl/src/Glide64/glide64_gDP.c \
    src/glide2gl/src/Glide64/glide64_gSP.c \
    src/glide2gl/src/Glide64/Glide64_Ini.c \
    src/glide2gl/src/Glide64/glide64_rdp.c \
    src/glide2gl/src/Glide64/Glide64_UCode.c \
    src/glide2gl/src/Glide64/glide64_util.c \
    src/glide2gl/src/Glide64/glidemain.c \
    src/glide2gl/src/Glide64/MiClWr.c \
    src/glide2gl/src/Glide64/TexCache.c \
    src/glide2gl/src/Glide64/TexLoad.c \
    src/glide2gl/src/Glitch64/geometry.c \
    src/glide2gl/src/Glitch64/glitch64_combiner.c \
    src/glide2gl/src/Glitch64/glitch64_textures.c \
    src/glide2gl/src/Glitch64/glitchmain.c \
    src/Graphics/3dmaths.c \
    src/Graphics/HLE/Microcode/Fast3D.c \
    src/Graphics/plugins.c \
    src/Graphics/RDP/gDP_funcs_C.c \
    src/Graphics/RDP/gDP_state.c \
    src/Graphics/RDP/RDP_state.c \
    src/Graphics/RSP/gSP_funcs_C.c \
    src/Graphics/RSP/gSP_state.c \
    src/Graphics/RSP/RSP_state.c \
    src/libretro-common/audio/conversion/float_to_s16.c \
    src/libretro-common/audio/conversion/s16_to_float.c \
    src/libretro-common/audio/resampler/audio_resampler.c \
    src/libretro-common/audio/resampler/drivers/nearest_resampler.c \
    src/libretro-common/audio/resampler/drivers/null_resampler.c \
    src/libretro-common/audio/resampler/drivers/sinc_resampler.c \
    src/libretro-common/compat/compat_posix_string.c \
    src/libretro-common/compat/compat_snprintf.c \
    src/libretro-common/compat/compat_strcasestr.c \
    src/libretro-common/compat/compat_strl.c \
    src/libretro-common/compat/fopen_utf8.c \
    src/libretro-common/encodings/encoding_crc32.c \
    src/libretro-common/encodings/encoding_utf.c \
    src/libretro-common/features/features_cpu.c \
    src/libretro-common/file/config_file.c \
    src/libretro-common/file/config_file_userdata.c \
    src/libretro-common/file/file_path.c \
    src/libretro-common/gfx/gl_capabilities.c \
    src/libretro-common/glsm/glsm.c \
    src/libretro-common/glsym/rglgen.c \
    src/libretro-common/hash/rhash.c \
    src/libretro-common/lists/string_list.c \
    src/libretro-common/memmap/memalign.c \
    src/libretro-common/streams/file_stream.c \
    src/libretro-common/string/stdstring.c \
    src/libretro-common/vfs/vfs_implementation.c \
    src/libretro/brumme_crc.c \
    src/libretro/libretronew.c \
    src/mupen64plus-core/src/ai/ai_controller.c \
    src/mupen64plus-core/src/api/callbacks.c \
    src/mupen64plus-core/src/api/common.c \
    src/mupen64plus-core/src/api/config.c \
    src/mupen64plus-core/src/api/frontend.c \
    src/mupen64plus-core/src/api/vidext_libretro.c \
    src/mupen64plus-core/src/dd/dd_controller.c \
    src/mupen64plus-core/src/dd/dd_disk.c \
    src/mupen64plus-core/src/dd/dd_rom.c \
    src/mupen64plus-core/src/gb/gb_cart.c \
    src/mupen64plus-core/src/main/cheat.c \
    src/mupen64plus-core/src/main/device.c \
    src/mupen64plus-core/src/main/eventloop.c \
    src/mupen64plus-core/src/main/main.c \
    src/mupen64plus-core/src/main/md5.c \
    src/mupen64plus-core/src/main/profile.c \
    src/mupen64plus-core/src/main/rom.c \
    src/mupen64plus-core/src/main/savestates.c \
    src/mupen64plus-core/src/main/util.c \
    src/mupen64plus-core/src/memory/m64p_memory.c \
    src/mupen64plus-core/src/pifbootrom/pifbootrom.c \
    src/mupen64plus-core/src/pi/cart_rom.c \
    src/mupen64plus-core/src/pi/flashram.c \
    src/mupen64plus-core/src/pi/pi_controller.c \
    src/mupen64plus-core/src/pi/sram.c \
    src/mupen64plus-core/src/plugin/audio_libretro/audio_backend_libretro.c \
    src/mupen64plus-core/src/plugin/emulate_game_controller_via_libretro.c \
    src/mupen64plus-core/src/plugin/get_time_using_C_localtime.c \
    src/mupen64plus-core/src/plugin/plugin.c \
    src/mupen64plus-core/src/plugin/rumble_via_input_plugin.c \
    src/mupen64plus-core/src/r4300/cached_interp.c \
    src/mupen64plus-core/src/r4300/cp0.c \
    src/mupen64plus-core/src/r4300/cp1.c \
    src/mupen64plus-core/src/r4300/empty_dynarec.c \
    src/mupen64plus-core/src/r4300/exception.c \
    src/mupen64plus-core/src/r4300/instr_counters.c \
    src/mupen64plus-core/src/r4300/interrupt.c \
    src/mupen64plus-core/src/r4300/mi_controller.c \
    src/mupen64plus-core/src/r4300/pure_interp.c \
    src/mupen64plus-core/src/r4300/r4300.c \
    src/mupen64plus-core/src/r4300/r4300_core.c \
    src/mupen64plus-core/src/r4300/recomp.c \
    src/mupen64plus-core/src/r4300/reset.c \
    src/mupen64plus-core/src/r4300/tlb.c \
    src/mupen64plus-core/src/rdp/fb.c \
    src/mupen64plus-core/src/rdp/rdp_core.c \
    src/mupen64plus-core/src/ri/rdram.c \
    src/mupen64plus-core/src/ri/rdram_detection_hack.c \
    src/mupen64plus-core/src/ri/ri_controller.c \
    src/mupen64plus-core/src/rsp/rsp_core.c \
    src/mupen64plus-core/src/si/af_rtc.c \
    src/mupen64plus-core/src/si/cic.c \
    src/mupen64plus-core/src/si/eeprom.c \
    src/mupen64plus-core/src/si/game_controller.c \
    src/mupen64plus-core/src/si/mempak.c \
    src/mupen64plus-core/src/si/n64_cic_nus_6105.c \
    src/mupen64plus-core/src/si/pif.c \
    src/mupen64plus-core/src/si/rumblepak.c \
    src/mupen64plus-core/src/si/si_controller.c \
    src/mupen64plus-core/src/si/transferpak.c \
    src/mupen64plus-core/src/vi/vi_controller.c \
    src/mupen64plus-rsp-cxd4/rsp.c \
    src/mupen64plus-rsp-hle/src/alist.c \
    src/mupen64plus-rsp-hle/src/alist_audio.c \
    src/mupen64plus-rsp-hle/src/alist_naudio.c \
    src/mupen64plus-rsp-hle/src/alist_nead.c \
    src/mupen64plus-rsp-hle/src/audio.c \
    src/mupen64plus-rsp-hle/src/cicx105.c \
    src/mupen64plus-rsp-hle/src/hle.c \
    src/mupen64plus-rsp-hle/src/hle_memory.c \
    src/mupen64plus-rsp-hle/src/hle_plugin.c \
    src/mupen64plus-rsp-hle/src/jpeg.c \
    src/mupen64plus-rsp-hle/src/mp3.c \
    src/mupen64plus-rsp-hle/src/musyx.c \
    src/mupen64plus-rsp-hle/src/re2.c \
    minizip/ioapi.c \
    minizip/miniunz.c \
    minizip/minizip.c \
    minizip/mztools.c \
    minizip/unzip.c \
    minizip/zip.c \
    src/mupen64plus-video-angrylion/interface.c \
    src/mupen64plus-video-angrylion/n64video.c

CPPFILES := mymain.cpp \
	src/Graphics/RDP/gDP_funcs.cpp \
	src/Graphics/RSP/gSP_funcs.cpp \
    src/gles2rice/src/Blender.cpp \
    src/gles2rice/src/Combiner.cpp \
    src/gles2rice/src/ConvertImage.cpp \
    src/gles2rice/src/ConvertImage16.cpp \
    src/gles2rice/src/DecodedMux.cpp \
    src/gles2rice/src/DeviceBuilder.cpp \
    src/gles2rice/src/FrameBuffer.cpp \
    src/gles2rice/src/gDP_rice.cpp \
    src/gles2rice/src/GraphicsContext.cpp \
    src/gles2rice/src/gSP_rice.cpp \
    src/gles2rice/src/OGLCombiner.cpp \
    src/gles2rice/src/OGLDecodedMux.cpp \
    src/gles2rice/src/OGLES2FragmentShaders.cpp \
    src/gles2rice/src/OGLExtCombiner.cpp \
    src/gles2rice/src/OGLExtRender.cpp \
    src/gles2rice/src/OGLGraphicsContext.cpp \
    src/gles2rice/src/OGLRender.cpp \
    src/gles2rice/src/OGLRenderExt.cpp \
    src/gles2rice/src/OGLTexture.cpp \
    src/gles2rice/src/Render.cpp \
    src/gles2rice/src/RenderBase.cpp \
    src/gles2rice/src/RenderExt.cpp \
    src/gles2rice/src/RenderTexture.cpp \
    src/gles2rice/src/RiceConfig.cpp \
    src/gles2rice/src/RiceDebugger.cpp \
    src/gles2rice/src/RSP_Parser.cpp \
    src/gles2rice/src/RSP_S2DEX.cpp \
    src/gles2rice/src/Texture.cpp \
    src/gles2rice/src/TextureManager.cpp \
    src/gles2rice/src/VectorMath.cpp \
    src/gles2rice/src/Video.cpp

FILES := $(patsubst %.c,%.o,$(CFILES)) $(patsubst %.cpp,%.o,$(CPPFILES))

MYFLAGS   := \
	-I. \
	-I./src/glide2gl/src/Glide64 \
	-I./src/gles2rice/src \
	-I./src/mupen64plus-core/src \
	-I./src/mupen64plus-core/src/api \
	-I./src/include \
	-I./src/mupen64plus-core/src/plugin/audio_libretro \
	-I./src/libretro-common/include \
	-I./src/libretro \
	-DHAVE_OPENGLES \
	-DHAVE_OPENGLES2 \
	-DHAVE_GLIDE64 \
    -DHAVE_THR_AL \
    -DHAVE_RICE \
	-DNDEBUG \
	-D__LIBRETRO__ \
	-DM64P_PLUGIN_API \
	-DM64P_CORE_PROTOTYPES \
	-D_ENDUSER_RELEASE \
	-DSINC_LOWER_QUALITY \
	-DNOSSE \
	-DNO_ASM \
	-DNO_LIBCO \
	-DDISABLE_3POINT \
	-Wno-c++11-narrowing \
	-DINLINE="inline" \
	-O3 \
    -flto \
	-s USE_SDL=2 -s USE_SDL_TTF=2 \
    -s USE_ZLIB=1 \
	-s USE_SDL_IMAGE=2

all: n64wasm.js

n64wasm.js: $(FILES)
	emcc -o $@ $(FILES) -s USE_SDL=2 -s USE_SDL_TTF=2 \
    -s USE_ZLIB=1 \
	-s USE_SDL_IMAGE=2 \
    -lGL \
    -s TOTAL_MEMORY=536870912 \
	--use-preload-plugins \
	-s MIN_WEBGL_VERSION=2 \
	-s FULL_ES3=1 \
	-s ASSERTIONS=0 \
	-O3 \
	-s EXIT_RUNTIME=0 \
	-s EXTRA_EXPORTED_RUNTIME_METHODS="['callMain','cwrap']" \
    -s EXPORTED_FUNCTIONS="['_main','_runMainLoop','_neilGetSoundBufferResampledAddress', \
    '_neilGetAudioWritePosition','_neil_serialize','_neil_unserialize','_neil_toast_message', \
    '_neil_export_eep','_neil_export_sra','_neil_export_fla','_neil_reset','_toggleFPS', '_neil_send_mobile_controls', \
    '_neil_set_buffer_remaining','_neil_set_endframe_callback','_neil_set_double_speed']" \
	-s INVOKE_RUN=0 \
    && mv n64wasm.js ../dist/ \
    && mv n64wasm.wasm ../dist/
    

%.o : %.cpp
	emcc -c $< -o $@ \
	$(MYFLAGS)

%.o : %.c
	emcc -c $< -o $@ \
	$(MYFLAGS)

clean:
	@echo "Cleaning"
	@echo $(FILES)
	rm -fr *.o */*.o */*/*.o */*/*/*.o */*/*/*/*.o
	