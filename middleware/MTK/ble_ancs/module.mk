			 

#################################################################################
#include path
CFLAGS 	+= -I$(SOURCE_DIR)/middleware/MTK/ble_ancs/inc

ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/ble_ancs/src_protected/),)
include $(SOURCE_DIR)/middleware/MTK/ble_ancs/src_protected/GCC/module.mk
else
LIBS += $(SOURCE_DIR)/middleware/MTK/ble_ancs/lib/libble_ancs.a
endif
#################################################################################
