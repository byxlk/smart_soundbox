
BOARD_SRC = driver/board/mt76x7_hdk/wifi
MT7687_CHIP = driver/chip/mt7687

# util
include $(SOURCE_DIR)/driver/board/mt76x7_hdk/util/module.mk

# wifi src protected
ifneq ($(wildcard $(strip $(SOURCE_DIR))/driver/board/mt76x7_hdk/wifi/src_protected/),)
include $(SOURCE_DIR)/driver/board/mt76x7_hdk/wifi/src_protected/GCC/module.mk
else
LIBS += $(SOURCE_DIR)/driver/board/mt76x7_hdk/lib/libwifi.a
endif

C_FILES  += $(BOARD_SRC)/src/get_profile_string.c
C_FILES  += $(BOARD_SRC)/src/misc.c
C_FILES  += $(BOARD_SRC)/src/os_util.c
C_FILES  += $(BOARD_SRC)/src/os_utils.c
C_FILES  += $(BOARD_SRC)/src/wifi_profile.c
C_FILES  += $(BOARD_SRC)/src/wifi_wps.c
C_FILES  += $(BOARD_SRC)/src/wifi_init.c
C_FILES  += $(BOARD_SRC)/src/wifi_os_api.c
C_FILES  += $(BOARD_SRC)/src/wifi_default_config.c
C_FILES  += $(BOARD_SRC)/src/wifi_channel.c
C_FILES  += $(MT7687_CHIP)/src/sdio_gen3/connsys_bus.c
C_FILES  += $(MT7687_CHIP)/src/sdio_gen3/connsys_driver.c
C_FILES  += $(MT7687_CHIP)/src/sdio_gen3/connsys_util.c
ifeq ($(MTK_WIFI_CONFIGURE_FREE_ENABLE),y)
C_FILES  += $(BOARD_SRC)/src/sie.c
endif

ifeq ($(MTK_MINICLI_ENABLE),y)
C_FILES  += $(BOARD_SRC)/src/wifi_cli.c
C_FILES  += $(BOARD_SRC)/src/wifi_private_cli.c
C_FILES  += $(BOARD_SRC)/src/wifi_inband_cli.c
ifeq ($(MTK_WIFI_CONFIGURE_FREE_ENABLE),y)
C_FILES  += $(BOARD_SRC)/src/sie_cli.c
endif
endif

#################################################################################
#include path
CFLAGS 	+= -Iinc
CFLAGS  += -I$(SOURCE_DIR)/kernel/service/inc
CFLAGS  += -I$(SOURCE_DIR)/middleware/MTK/nvdm/inc
CFLAGS  += -I$(MT7687_CHIP)/src/sdio_gen3/include

