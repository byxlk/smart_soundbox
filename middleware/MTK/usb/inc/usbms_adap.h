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


/*****************************************************************************
 *
 * Filename:
 * ---------
 *    usbms_adap.h
 *
 * Project:
 * --------
 *   Maui_Software
 *
 * Description:
 * ------------
 *   This file intends for usb adaption layer for mass storage definitions
 *
 * Author:
 * -------
 *   Jensen Hu
 *   CJung Chen  
 *
 *****************************************************************************/
 
#ifndef USBMS_ADAP_H
#define USBMS_ADAP_H

#include "kal_data_types.h"
#include "usb_msdisk.h"
#include "usbms_state.h"

/***********************************************
	function and global variable
************************************************/

extern USB_DiskDriver_STRUCT usbms_msdc_driver;


/* translate fd type to usb dev type*/
extern kal_uint8 USB_Ms_Get_Max_LUN(void);

extern void USB_Ms_DeRegister_DiskDriver(kal_uint8 disk_index);
extern void USB_Ms_Change_Register_DiskDriver(kal_uint8 disk_index, USB_DiskDriver_STRUCT *disk_api);

extern USB_STORAGE_DEV_STATUS USB_Ms_Checkmedia_Exist(kal_uint8 LUN);
extern kal_bool USB_Ms_Format(kal_uint8 LUN);
extern kal_bool USB_Ms_Read_Capacity(kal_uint8 LUN, kal_uint32 *max_lba, kal_uint32 *sec_len);
extern kal_bool USB_Ms_Read_FormatCapacity(kal_uint8 LUN, kal_uint32 *max_lba, kal_uint32 *sec_len);
extern kal_bool USB_Ms_Read_Write(USB_MS_DIRECTION direct, kal_uint8 LUN, void *data, kal_uint32 LBA, kal_uint16 sec_len);
extern kal_bool USB_Ms_Prevmedia_Removal(kal_uint8 LUN, kal_bool enable);
extern USB_STORAGE_DEV_TYPE USB_Ms_Dev_Type(kal_uint8 LUN);

#endif //USBMS_ADAP_H

