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
    @file       wifi_ex_connect.c
    @brief      WiFi - Connect API Examples

    @history    2015/08/07  Initial for 1st draft  (Michael Rong)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "wifi_scan.h"
#include "wifi_inband.h"
#include "inband_queue.h"
#include "os.h"
#include "os_util.h"
#include "cli.h"
#include "type_def.h"
#include "wifi_api.h"
#include "wifi_ex_connect.h"
#include "get_profile_string.h"
#include "misc.h"
#include "wifi_cli.h"
#include "wifi_private_cli.h"


#define WIFI_CLI_RETURN_STRING(ret) ((ret>=0)?"Success":"Error")

#if defined(MTK_HOMEKIT_ENABLE)
/**
* @brief Example of Get the AP information list of the last scan operation
* wifi connect get scanlist
* @param [OUT]scan_list
*  Number of AP scaned and a SCAN_LIST data structure will be got
*
* @return  =0 means success, >0 means fail
*
* @note If no wifi_conn_start_scan() has been performed, an empty data structure is returned
*/
static uint8_t wifi_connect_get_scan_list_ex(uint8_t len, char *param[])
{
    wifi_scan_list_t scan_list[MAX_SCAN_ARRAY_ITEM];
    wifi_scan_list_t *ptr = scan_list;
    int i;
    uint8_t status = 0, number = 0;
    int32_t ret = 0;

    for (i = 0; i < MAX_SCAN_ARRAY_ITEM - 1; i++) {
        os_memset(scan_list[i].ap_data.ssid, 0, sizeof(scan_list[i].ap_data.ssid));
        scan_list[i].next = (struct _wifi_scan_list_t *)&scan_list[i + 1];
    }
    scan_list[MAX_SCAN_ARRAY_ITEM - 1].next = NULL;

    ret = wifi_connection_get_scan_list(&number, scan_list);
    if (ret < 0) {
        status = 1;
    }

    printf("wifi_connect_get_scan_list, ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);
    i = 0;
    while (ptr) {
        printf("%d\n", i++);
        printf("    rssi:%d\n", ptr->ap_data.rssi);
        printf("    ssid=%s, ssid_len=%d\n", ptr->ap_data.ssid, ptr->ap_data.ssid_length);
        printf("    channel=%d\n", ptr->ap_data.channel);
        printf("    central_channel=%d\n", ptr->ap_data.central_channel);
        printf("    bssid=%02X:%02X:%02X:%02X:%02X:%02X\n", ptr->ap_data.bssid[0],
               ptr->ap_data.bssid[1],
               ptr->ap_data.bssid[2],
               ptr->ap_data.bssid[3],
               ptr->ap_data.bssid[4],
               ptr->ap_data.bssid[5]);
        printf("    wps=%d\n", ptr->ap_data.is_wps_supported);
        printf("    auth_mode=%d\n", ptr->ap_data.auth_mode);
        printf("    encrypt_type=%d\n", ptr->ap_data.encrypt_type);

        ptr = (wifi_scan_list_t *)(ptr->next);
    }
    return status;
}
#endif


#if defined(MTK_HOMEKIT_ENABLE)
static uint8_t wifi_connect_set_scan_prefer(uint8_t len, char *param[])
{

    if (len > 0 && param != NULL) {
        wifi_scan_set_prefer_ssid(param[0], os_strlen(param[0]));
    } else {
        wifi_scan_set_prefer_ssid(NULL, 0);
    }

    return 0;
}
#endif

#ifdef MTK_MINICLI_ENABLE
cmd_t   wifi_connect_set_cli[] = {
    { "scan",       "start/stop scan",                      wifi_connect_scan_ex},
    { "connection", "connect/disconnect to AP",             wifi_connect_set_connection_ex},
    { "deauth",     "de-authenticate specific STA",         wifi_connect_deauth_station_ex},
    { "eventcb",    "register/un-register event callback",  wifi_connect_set_event_callback_ex},
#if defined(MTK_HOMEKIT_ENABLE)
    { "prefer_scan", "scan preferred SSID",                 wifi_connect_set_scan_prefer},
#endif
    { NULL }
};

cmd_t   wifi_connect_get_cli[] = {
    { "stalist",     "get STA list",                        wifi_connect_get_station_list_ex},
    { "linkstatus",  "get link status",                     wifi_connect_get_link_status_ex},
    { "max_num_sta", "get max number of supported STA",     wifi_connect_get_max_station_number_ex},
    { "rssi",        "get rssi",                            wifi_connect_get_rssi_ex},
#if defined(MTK_HOMEKIT_ENABLE)
    { "scanlist",   "get scan list",        wifi_connect_get_scan_list_ex},
#endif
    { NULL }
};
#endif
