
###################################################
# Sources
ATCI_SRC = middleware/MTK/atci
ATCI_FILES = $(ATCI_SRC)/src/atci_adapter.c \
                     $(ATCI_SRC)/src/atci_handler.c \
                     $(ATCI_SRC)/src/atci_main.c
             
ifeq ($(MTK_AT_CMD_DISABLE), y)
 ATCMD_FILES = $(ATCI_SRC)/at_command/at_command.c
else
 ATCMD_FILES = $(ATCI_SRC)/at_command/at_command.c\
	     $(ATCI_SRC)/at_command/at_command_sdkinfo.c \
             $(ATCI_SRC)/at_command/at_command_gnss.c \
             $(ATCI_SRC)/at_command/at_command_charger.c \
             $(ATCI_SRC)/at_command/at_command_pmu.c \
	     $(ATCI_SRC)/at_command/at_command_external_pmu.c \
             $(ATCI_SRC)/at_command/at_command_external_gauge.c \
             $(ATCI_SRC)/at_command/at_command_msdc.c \
             $(ATCI_SRC)/at_command/at_command_nvdm.c \
             $(ATCI_SRC)/at_command/at_command_led.c \
             $(ATCI_SRC)/at_command/at_command_gpio.c \
             $(ATCI_SRC)/at_command/at_command_crypto.c \
             $(ATCI_SRC)/at_command/at_command_ctp.c \
             $(ATCI_SRC)/at_command/at_command_keypad.c \
             $(ATCI_SRC)/at_command/at_command_bt.c \
             $(ATCI_SRC)/at_command/at_command_audio.c \
             $(ATCI_SRC)/at_command/at_command_backlight.c \
             $(ATCI_SRC)/at_command/at_command_rtc.c \
             $(ATCI_SRC)/at_command/at_command_vibrator.c \
             $(ATCI_SRC)/at_command/at_command_wdt.c    \
             $(ATCI_SRC)/at_command/at_command_sleep_manager.c \
             $(ATCI_SRC)/at_command/at_command_gsensor.c \
             $(ATCI_SRC)/at_command/at_command_reg.c \
             $(ATCI_SRC)/at_command/at_command_dvfs.c \
             $(ATCI_SRC)/at_command/at_command_system.c \
             $(ATCI_SRC)/at_command/at_command_clock.c  \
             $(ATCI_SRC)/at_command/at_command_ctp_simulate.c \
             $(ATCI_SRC)/at_command/at_command_serial_port.c \
             $(ATCI_SRC)/at_command/at_command_mem.c \
             $(ATCI_SRC)/at_command/at_command_keypad_simulate.c

ifeq ($(TOOL_APP_MODULE),testframework)
ATCMD_FILES  += $(ATCI_SRC)/at_command/at_command_testframework.c
endif

ifeq ($(MTK_AUDIO_TUNING_ENABLED), y)
ifeq ($(IC_CONFIG),mt2533)
ATCI_FILES += $(ATCI_SRC)/at_command/at_command_audio_tuning_mt2533.c \
              $(ATCI_SRC)/at_command/at_command_audio_tuning_db_to_gain_value_mapping_table.c
else
ATCI_FILES += $(ATCI_SRC)/at_command/at_command_audio_tuning.c \
              $(ATCI_SRC)/at_command/at_command_audio_tuning_db_to_gain_value_mapping_table.c
endif
endif

ifeq ($(IC_CONFIG), $(filter $(IC_CONFIG), mt2523 mt2533))
ATCMD_FILES += $(ATCI_SRC)/at_command/at_command_lcm.c
endif

ifeq ($(MTK_SENSOR_AT_COMMAND_ENABLE), y)
ATCMD_FILES += $(ATCI_SRC)/at_command/at_command_sensor.c 
CFLAGS += -DMTK_SENSOR_AT_COMMAND_ENABLE
endif

ifeq ($(MTK_SENSOR_BIO_USE),MT2511)
# include kiss_fft
include $(SOURCE_DIR)/middleware/third_party/kiss_fft/module.mk
ATCMD_FILES += $(ATCI_SRC)/at_command/at_command_biogui.c 
			  
endif

ifneq ($(MTK_DEBUG_LEVEL), none)
ATCMD_FILES += $(ATCI_SRC)/at_command/at_command_syslog.c
endif

endif

C_FILES += $(ATCI_FILES)
C_FILES += $(ATCMD_FILES)
###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/atci/inc
CFLAGS += -I$(SOURCE_DIR)/driver/board/mt25x3_hdk/backlight
CFLAGS += -I$(SOURCE_DIR)/driver/board/mt25x3_hdk/keypad/inc
CFLAGS += -I$(SOURCE_DIR)/driver/board/component/biosensor/mt2511/inc

