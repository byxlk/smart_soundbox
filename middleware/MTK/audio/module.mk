
###################################################

AUDIO_SRC = middleware/MTK/audio

ifeq ($(MTK_TEMP_REMOVE), y)
else
  ifeq ($(MTK_NVDM_ENABLE), y)
    AUDIO_FILES = $(AUDIO_SRC)/src/audio_middleware_api.c

    ifeq ($(IC_CONFIG),mt2533)
      AUDIO_FILES += $(AUDIO_SRC)/port/mt2533/src/audio_nvdm.c
    else
      AUDIO_FILES += $(AUDIO_SRC)/port/mt2523/src/audio_nvdm.c
    endif

  endif
endif

ifeq ($(MTK_AUDIO_MP3_ENABLED), y)
ifeq ($(IC_CONFIG),mt7687)
AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec_7687.c
LIBS += $(SOURCE_DIR)/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
else
AUDIO_FILES += $(AUDIO_SRC)/mp3_codec/src/mp3_codec.c
LIBS += $(SOURCE_DIR)/middleware/MTK/audio/mp3_codec/lib/arm_cm4/libmp3dec.a
endif
endif



C_FILES += $(AUDIO_FILES) 

###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/inc

ifeq ($(IC_CONFIG),mt2533)
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/mt2533/inc
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/mt2533/inc/mt2533_external_dsp_profile
else
  CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/port/mt2523/inc
endif



ifeq ($(MTK_AUDIO_MP3_ENABLED), y)
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/audio/mp3_codec/inc
endif

