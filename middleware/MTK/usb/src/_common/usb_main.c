/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. ("MediaTek") and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek ("License Agreement") and been granted explicit permission to do so within
 * the License Agreement ("Permitted User").  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

#ifdef MTK_USB_DEMO_ENABLED

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* device.h includes */
#include "mt2523.h"

/* hal includes */
#include "hal_cache.h"
#include "hal_pmu_wrap_interface.h"
#include "hal_sleep_manager.h"
#include "usbacm_drv.h"
#include "memory_attribute.h"
#include "usbacm_adap.h"
#include "usb.h"
#include "usb_comm.h"
#include "usb_resource.h"

#include "usbms_drv.h"
#include "usbms_adap.h"

#ifdef MTK_PORT_SERVICE_ENABLE
#include "serial_port.h"
extern serial_port_register_callback_t g_serial_port_usb_callback[MAX_USB_PORT_NUM];
#endif

#ifdef  __USB_MASS_STORAGE_ENABLE__
msc_register_callback_t g_msc_usb_callback = NULL;
#endif

#define BOOT_TIMEOUT_CNT 900000
static bool usb_initial = false;

static uint8_t usb_sleep_handler = 0;

bool is_vusb_ready(void)
{
    bool ret;

    ret = (bool)pmu_get_usb_input_status();

    return ret;
}

const USB_DEVICE_PARAM *USB_GetDeviceParam(void);
const USB_ACM_PARAM *USB_GetAcmParam(void);
const USB_MS_PARAM *USB_GetMsParam(void);


bool ap_usb_init(USB_DEVICE_TYPE type)
{
    if (usb_initial == true) {
        return false;
    }

    usb_initial = true;

    /*LOG_I(hal,"++++++++++ usb lcok sleep\n");*/
    hal_sleep_manager_lock_sleep(usb_sleep_handler);
    hal_usb_init();
    ///* Get customize parameters */
    gUsbDevice.device_param = USB_GetDeviceParam();


    switch (type) {
        case USB_CDC_ACM:
            g_UsbACM_Comm.acm_param = USB_GetAcmParam();
            USB_Register_CreateFunc("ACM COMMU1",
                                    USB_Acm1_CommIf_Create,
                                    USB_Acm1_CommIf_Reset,
                                    USB_Acm1_CommIf_Enable,
                                    (usb_speed_if_func_ptr)USB_Acm1_CommIf_Speed_Reset,
                                    NULL);

            USB_Register_CreateFunc("ACM DATA1",
                                    USB_Acm1_DataIf_Create,
                                    USB_Acm1_DataIf_Reset,
                                    USB_Acm1_DataIf_Enable,
                                    (usb_speed_if_func_ptr)USB_Acm1_DataIf_Speed_Reset,
                                    NULL);

            USB_Register_CreateFunc("ACM COMMU2",
                                    USB_Acm2_CommIf_Create,
                                    USB_Acm2_CommIf_Reset,
                                    USB_Acm2_CommIf_Enable,
                                    (usb_speed_if_func_ptr)USB_Acm2_CommIf_Speed_Reset,
                                    NULL);

            USB_Register_CreateFunc("ACM DATA2",
                                    USB_Acm2_DataIf_Create,
                                    USB_Acm2_DataIf_Reset,
                                    USB_Acm2_DataIf_Enable,
                                    (usb_speed_if_func_ptr)USB_Acm2_DataIf_Speed_Reset,
                                    NULL);

            USB_Register_Device_Code(0x00, 0x00, 0x00, 0x0023/*g_UsbACM_Comm.acm_param->desc_product_dual_port*/);

            USB_Init(USB_CDC_ACM);
            break;

#ifdef  __USB_MASS_STORAGE_ENABLE__
        case USB_MASS_STORAGE:
            g_UsbMS.ms_param = USB_GetMsParam();

            //USB_Ms_Register_DiskDriver(&USB_RAM_drv);
            USB_Ms_Register_DiskDriver(&usbms_msdc_driver);


            USB_Register_CreateFunc("MASS STORAGE", USB_Ms_If_Create, USB_Ms_If_Reset,
                                    USB_Ms_If_Enable, (usb_speed_if_func_ptr)USB_Ms_If_Speed_Reset, USB_Ms_If_Resume);
            USB_Register_Device_Code(USB_MS_DEVICE_CODE, USB_MS_SUBCLASS_CODE,
                                     USB_MS_PROTOCOL_CODE, g_UsbMS.ms_param->desc_product);

            /* initialize MASS STORAGE MODE */
            USB_Init(USB_MASS_STORAGE);
            break;
#endif

        default:
            LOG_E(hal, "ASSERT\n");
            break;
    }


    return true;
}

bool ap_usb_deinit(void)
{
    if (usb_initial == false) {
        return false;
    }

    usb_initial = false;

    USB_Release_Type();

    hal_sleep_manager_unlock_sleep(usb_sleep_handler);
    /*LOG_I(hal,"--------- usb unlcok sleep\n");*/

    return true;
}

void usb_cable_detect(void)
{
    //LOG_I(hal, "usb_cable_detect\n");

    if (usb_sleep_handler == 0) {
        usb_sleep_handler = hal_sleep_manager_set_sleep_handle("USBDemo");
    }

    if (is_vusb_ready()) {
        LOG_I(hal, "USB in\n");

#if defined(MTK_USB_BOOT_MSC)
        ap_usb_init(USB_MASS_STORAGE);
#else //defined(MTK_USB_BOOT_ACM)
        ap_usb_init(USB_CDC_ACM);
#endif
		
#ifdef MTK_PORT_SERVICE_ENABLE
        if (g_serial_port_usb_callback[0] != NULL) {
            g_serial_port_usb_callback[0](SERIAL_PORT_DEV_USB_COM1, SERIAL_PORT_EVENT_USB_CONNECTION, NULL);
            LOG_I(hal, "USB1 CONNECTION callback\n");
        }
        if (g_serial_port_usb_callback[1] != NULL) {
            g_serial_port_usb_callback[1](SERIAL_PORT_DEV_USB_COM2, SERIAL_PORT_EVENT_USB_CONNECTION, NULL);
            LOG_I(hal, "USB2 CONNECTION callback\n");
        }
#endif

#ifdef  __USB_MASS_STORAGE_ENABLE__
        if (g_msc_usb_callback != NULL) {
            g_msc_usb_callback(MSC_EVENT_USB_CONNECTION);
            LOG_I(hal, "msc CONNECTION callback\n");
        }
#endif


    } else {
        if (usb_initial == true) {
#ifdef MTK_PORT_SERVICE_ENABLE
        if (g_serial_port_usb_callback[0] != NULL) {
            g_serial_port_usb_callback[0](SERIAL_PORT_DEV_USB_COM1, SERIAL_PORT_EVENT_USB_DISCONNECTION, NULL);
            LOG_I(hal, "USB1 CONNECTION callback\n");
        }
        if (g_serial_port_usb_callback[1] != NULL) {
            g_serial_port_usb_callback[1](SERIAL_PORT_DEV_USB_COM2, SERIAL_PORT_EVENT_USB_DISCONNECTION, NULL);
            LOG_I(hal, "USB2 CONNECTION callback\n");
        }

#endif

#ifdef  __USB_MASS_STORAGE_ENABLE__
            if (g_msc_usb_callback != NULL) {
                g_msc_usb_callback(MSC_EVENT_USB_DISCONNECTION);
                LOG_I(hal, "msc DISCONNECTION callback\n");
            }
#endif
            ap_usb_deinit();
            LOG_I(hal, "USB out\n");
        }
    }
}

extern void usb_hisr(void);

void usb_boot_init(void)
{
    int i = 0;

    if (usb_sleep_handler == 0) {
        usb_sleep_handler = hal_sleep_manager_set_sleep_handle("USBDemo");
		LOG_I(hal, "usb_sleep_handler=%d\n", usb_sleep_handler);
    }

    if (is_vusb_ready()) {

#if defined(MTK_USB_BOOT_MSC)
        ap_usb_init(USB_MASS_STORAGE);
#else //defined(MTK_USB_BOOT_MSC)
        ap_usb_init(USB_CDC_ACM);
#endif

        do {
            usb_hisr();
            if (i++ >= BOOT_TIMEOUT_CNT) {
                break;
            }
        } while ((USB_Get_Device_State() != DEVSTATE_CONFIG));
        LOG_I(hal, "USBCOM init, hisr cnt%d\n\r", i);
    }


}

#ifdef  __USB_MASS_STORAGE_ENABLE__
void ap_usb_register_msc_callback(msc_register_callback_t callback)
{
    g_msc_usb_callback = callback;
	LOG_I(hal, "register MSC connection callback\n");

    if (g_msc_usb_callback != NULL) {
        if (is_vusb_ready()) {
            g_msc_usb_callback(MSC_EVENT_USB_CONNECTION);
            LOG_I(hal, "MSC_CONNECTION callback\n");
        } else {
            g_msc_usb_callback(MSC_EVENT_USB_DISCONNECTION);
            LOG_I(hal, "MSC_DISCONNECTION callback\n");
        }
    }
}
#endif

#endif //MTK_USB_DEMO_ENABLED


