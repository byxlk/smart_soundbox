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
/**
    @file       wifi_private_cli.h
    @brief      WiFi - Configure API Examples, these CLI in the wifi_private_api.c
                is only for internal use, it will be removed or modified in future.

    @history    2016/07/27  Initial for 1st draft  (Pengfei Qiu)
 */

#ifndef __WIFI_PRIVATE_CLI_H__
#define __WIFI_PRIVATE_CLI_H__

#ifdef __cplusplus
extern "C" {
#endif
#include "cli.h"


/**
* @brief
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_rssi_threshold_ex(uint8_t len, char *param[]);
/**
* @brief
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_rssi_threshold_ex(uint8_t len, char *param[]);

int32_t wifi_frame_type_event_handler(wifi_event_t event, uint8_t *payload, uint32_t length);

/**
* @brief Example of set frame filter for packets format wanted to be received
* wifi config set frame_filter 1 8 221 0 15 172
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_frame_filter_ex(uint8_t len, char *param[]);
/**
* @brief Set N9 Debug Level
* wifi config set n9dbg <dbg_level>
* @param [IN]dbg_level
*   0: None
*   1: ERROR
*   2. WARNING
*   3. TRACE
*   4. INFO
*   5. LAUD
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_n9_dbg_level(uint8_t len, char *param[]);

/**
* @brief Get N9 Debug Level
* wifi config get n9dbg
* @param [OUT]dbg_level
*   0: None
*   1: ERROR
*   2. WARNING
*   3. TRACE
*   4. INFO
*   5. LAUD
*
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_n9_dbg_level(uint8_t len, char *param[]);

/**
* @brief Example of Start/Stop WiFi Scanning
* wifi connect set scan <start/stop> <mode> <operation> [ssid] [bssid]
* @param [IN]ssid SSID, "NULL" means not specified
* @param [IN]bssid BSSID, "NULL" means not specified
* @param [IN]flag
* @param 0 Active Scan
* @param 1 Passive Scan
*
* @return  =0 means success, >0 means fail
*
* @note When SSID/BSSID specified (not NULL)
* @note 1. ProbeReq carries SSID (for Hidden AP)
* @note 2. Scan Tab always keeps the APs of this specified SSID/BSSID
*/
uint8_t wifi_connect_scan_ex(uint8_t len, char *param[]);

/**
* @brief Example of Get WiFi TX Power
* <br><b>
* wifi config get tx_power
* </b></br>
* @param  [OUT]power: 64~190, instead of value range get from N9 is -315 to +315
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_get_tx_power_ex(uint8_t len, char *param[]);

/**
* @brief Example of Set WiFi Tx Power
* <br><b>
* wifi config set tx_power <power>
* </b></br>
* @param [IN]interval 64 ~ 190, instead of value range set to N9 is -315 to +315
* @return  =0 means success, >0 means fail
*/
uint8_t wifi_config_set_tx_power_ex(uint8_t len, char *param[]);

uint8_t wifi_config_set_n9log_state_ex(uint8_t len, char *param[]);

/**
* @brief Example of Set Country Code
* wifi config set country_code <code>
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_country_code_ex(uint8_t len, char *param[]);

/**
* @brief Example of Get Country Code
* wifi config get country_code
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_country_code_ex(uint8_t len, char *param[]);

#ifdef __cplusplus
    }
#endif
#endif /*  __WIFI_PRIVATE_CLI_H__ */

