-include $(SOURCE_DIR)/middleware/MTK/smtcn/src/module.mk

ifneq ($(wildcard $(strip $(SOURCE_DIR))/middleware/MTK/smtcn/src_protected),)
-include $(SOURCE_DIR)/middleware/MTK/smtcn/src_protected/GCC/module.mk
else
LIBS += $(SOURCE_DIR)/middleware/MTK/smtcn/lib/libsmtcn_protected.a
endif

