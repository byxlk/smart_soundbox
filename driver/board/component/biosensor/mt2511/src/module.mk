ifeq ($(MTK_SENSOR_BIO_USE),MT2511)
###################################################

VSM_DRIVER_SRC = driver/board/component/biosensor/mt2511
VSM_DRIVER_FILES = $(VSM_DRIVER_SRC)/src/vsm_driver.c \
                   $(VSM_DRIVER_SRC)/src/vsm_sensor_subsys_adaptor.c \
                   $(VSM_DRIVER_SRC)/src/vsm_platform_function.c

C_FILES += $(VSM_DRIVER_FILES)

ifeq ($(MT2511_INTERFACE),SPI)
VSM_DRIVER_FILES += $(VSM_DRIVER_SRC)/src/vsm_spi_operation.c
CFLAGS	+= -DMT2511_USE_SPI
else
VSM_DRIVER_FILES += $(VSM_DRIVER_SRC)/src/vsm_i2c_operation.c
CFLAGS	+= -DMT2511_USE_I2C
endif

###################################################
# include path
CFLAGS += -I$(SOURCE_DIR)/$(VSM_DRIVER_SRC)/inc
CFLAGS += -I$(SOURCE_DIR)/middleware/MTK/sensor_subsys/inc

#include module setting
ifeq ($(MTK_SENSOR_BIO_MODULE),EVERLIGHT)
    CFLAGS += -DCUSTOM_MODULE_EVERLIGHT -I$(SOURCE_DIR)/$(VSM_DRIVER_SRC)/module/everlight
else
    ifeq ($(MTK_SENSOR_BIO_MODULE),SOLTEAMOPTO)
        CFLAGS += -DCUSTOM_MODULE_SOLTEAMOPTO -I$(SOURCE_DIR)/$(VSM_DRIVER_SRC)/module/solteamopto
    else
        ifeq ($(MTK_SENSOR_BIO_MODULE),OSRAM)
            CFLAGS += -DCUSTOM_MODULE_OSRAM -I$(SOURCE_DIR)/$(VSM_DRIVER_SRC)/module/osram
        else
            CFLAGS += -DCUSTOM_MODULE_DEFAULT -I$(SOURCE_DIR)/$(VSM_DRIVER_SRC)/module/default
        endif
    endif
endif

ifeq ($(PLATFORM_DEVICE),HDK)
CFLAGS	+= -DBIO_TUNNING_TOOL_USE
endif

ifeq ($(MT2511_E1),y)
CFLAGS	+= -DUSE_EXTERNAL_BOOST
endif

ifeq ($(MT2511_USE_IRQ),y)
CFLAGS	+= -DMT2511_USE_IRQ
endif

endif