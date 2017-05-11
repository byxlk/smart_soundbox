
BOARD_SRC = driver/board/mt25x3_hdk
COMPONENT_SRC = driver/board/component
BT_CODEC_SRC = driver/board/mt25x3_hdk/bt_codec

C_FILES  += $(BOARD_SRC)/lcd/mt25x3_hdk_lcd.c
C_FILES  += $(COMPONENT_SRC)/lcm/ST7789H2/lcd.c
C_FILES  += $(COMPONENT_SRC)/lcm/RM69032/lcd_RM69032.c
C_FILES  += $(BOARD_SRC)/backlight/mt25x3_hdk_backlight.c

#bt codec driver source
ifeq ($(MTK_BT_CODEC_ENABLED),y)
  C_FILES  += $(BT_CODEC_SRC)/src/bt_a2dp_codec.c
  C_FILES  += $(BT_CODEC_SRC)/src/bt_hfp_codec.c
endif


#################################################################################
#include path
CFLAGS 	+= -Iinclude
CFLAGS  += -I$(SOURCE_DIR)/kernel/service/inc
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/nvdm/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/inc
CFLAGS  += -I$(SOURCE_DIR)/driver/chip/mt2523/inc
CFLAGS  += -I$(BOARD_SRC)/lcd

CFLAGS  += -I$(SOURCE_DIR)/driver/board/component/common

#bt codec driver include path
ifeq ($(MTK_BT_CODEC_ENABLED),y)
  CFLAGS  += -I$(SOURCE_DIR)/$(BT_CODEC_SRC)/inc
  CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/bluetooth/inc
  CFLAGS  += -I$(SOURCE_DIR)/driver/chip/mt2523/inc
  CFLAGS  += -I$(SOURCE_DIR)/driver/board/component/bt_codec/inc
endif

ifeq ($(MTK_BT_AWS_ENABLE), y)
CFLAGS += -D__BT_A2DP_CODEC_AWS_SUPPORT__
endif

# add for TE enable
ifeq ($(MTK_TE_ENABLE), y)
CFLAGS	+= -DMTK_TE_ENABLE
endif
