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
    @file       wifi_private_cli.c
    @brief      WiFi - Configure API Examples, these CLI in the wifi_private_api.c
                is only for internal use, it will be removed or modified in future.

    @history    2016/07/27  Initial for 1st draft  (Pengfei Qiu)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "os.h"
#include "os_util.h"
#include "FreeRTOS.h"
#include "wifi_api.h"
#include "wifi_private_api.h"
#include "wifi_scan.h"
#include "ethernet_filter.h"
#include "inband_queue.h"
#include "get_profile_string.h"
#include "queue.h"
#include "task.h"
#include "task_def.h"

#if defined(MTK_MINICLI_ENABLE)
#include "cli_def.h"
#endif


#define WIFI_CLI_RETURN_STRING(ret) ((ret>=0)?"Success":"Error")

/**
* @brief
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_rssi_threshold_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t enable = 0;
    int8_t rssi = 0;

    if((ret = wifi_inband_get_rssi_threshold(&enable, &rssi)) >= 0){
        printf("rssi threthold: [enabled: %d], [value: %d]\n", enable, rssi);
    } else {
        status = 1;
    }
    printf("wifi_inband_get_rssi_threshold: ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);

    return status;
}


/**
* @brief
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_rssi_threshold_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t enable = 0;
    int8_t rssi = 0;

    enable = (uint8_t)atoi(param[0]);
    if((len == 1) && (enable != 0)){
        printf("usage: wifi config set rssi_threshold <enabled> <rssi value>\n");
        return 1;
    }
    rssi = (int8_t)atoi(param[1]);

    ret = wifi_inband_set_rssi_threshold(enable, rssi);
    if (ret < 0) {
        status = 1;
    }
    printf("wifi_inband_set_rssi_threshold: ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);
    return status;
}

int32_t wifi_frame_type_event_handler(wifi_event_t event,
                                                    uint8_t *payload,
                                                    uint32_t length)
{
    uint8_t type = 0;
    if(payload == NULL) {
        LOG_E(wifi, "payload is empty!");
        return -1;
    }
    LOG_HEXDUMP_E(wifi, "event id: %d", payload, length, event);
    type = payload[0]>>4;
    switch (type) {
        case 0: printf("frame type is assoc req\r\n");break;
        case 1: printf("frame type is assoc resp\r\n");break;
        case 2: printf("frame type is reassoc req\r\n");break;
        case 3: printf("frame type is reassoc resp\r\n");break;
        case 4: printf("frame type is probe req\r\n");break;
        case 5: printf("frame type is probe resp\r\n");break;
        case 8: printf("frame type is beacon\r\n");break;
        case 9: printf("frame type is atim\r\n");break;
        case 10: printf("frame type is disassoc\r\n");break;
        case 11: printf("frame type is auth\r\n");break;
        case 12: printf("frame type is deauth\r\n");break;
        default:
            printf("wrong management frame\r\n");
            break;
    }
    return 1;
}

/**
* @brief Example of set frame filter for packets format wanted to be received
* wifi config set frame_filter 1 8 221 0 15 172
* @parameter
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_frame_filter_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t enable = 0;
    uint16_t frame_type = 0;
    extra_vender_ie_t vender_ie = {0};
    wifi_connection_register_event_handler(WIFI_EVENT_IOT_REPORT_FILTERED_FRAME, wifi_frame_type_event_handler);
    enable = (uint8_t)atoi(param[0]);
    frame_type = (uint16_t)atoi(param[1]);
    if (2 == len) {
        ret = wifi_config_set_frame_filter(enable, frame_type, NULL);
    }
    else {
        vender_ie.element_id = (uint8_t)atoi(param[2]);
        vender_ie.OUI[0] = (uint8_t)atoi(param[3]);
        vender_ie.OUI[1] = (uint8_t)atoi(param[4]);
        vender_ie.OUI[2] = (uint8_t)atoi(param[5]);
        ret = wifi_config_set_frame_filter(enable, frame_type, &vender_ie);
    }
    if (ret < 0) {
        status = 1;
    }
    printf("wifi_config_set_frame_filter: ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);
    return status;
}


#define EXT_SEC_CMD_SET_AUTH_MODE    0x00
#define EXT_SEC_CMD_SET_ENCR_TYPE    0x01
#define EXT_SEC_CMD_SET_WPAPSK       0x02
#define EXT_SEC_CMD_SET_WEP_KEY      0x03
#define EXT_SEC_CMD_MAX              0x04

typedef struct _EXT_SECURITY_CMD_T {
    UINT16     u2ExtCmdID;
    UINT16     u2DataLen;
    UINT      aucDataBuffer[1];
} EXT_SECURITY_CMD_T, *P_EXT_SECURITY_CMD_T;

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
uint8_t wifi_config_set_n9_dbg_level(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t dbg_level = atoi(param[0]);

    ret = wifi_inband_set_debug_level(dbg_level);
    if (ret < 0) {
        status = 1;
    }
    printf("wifi_config_set_n9_dbg_level, level = %d, ret:%s, Code=%ld\n", dbg_level, WIFI_CLI_RETURN_STRING(ret), ret);

    return status;
}


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
uint8_t wifi_config_get_n9_dbg_level(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t dbg_level = 0;

    ret = wifi_inband_get_debug_level(&dbg_level);
    if (ret < 0) {
        status = 1;
    }
    printf("wifi_config_get_n9_dbg_level, level = %d, ret:%s, Code=%ld\n",
           dbg_level, WIFI_CLI_RETURN_STRING(ret), ret);

    return status;
}


extern wifi_scan_list_item_t *g_scan_list;
extern uint8_t g_scan_list_size;


static wifi_scan_list_item_t g_ap_list[8] = {{0}};
void parse_channel_list_from_string(const char *channel_list, ch_list_t *channel_item, uint8_t band)
{
    char string_data[4] = {0};
    uint32_t index = 0;
    uint32_t save_index = 0;
    uint32_t count = 0;
    uint32_t channel_section_count = 0;

    while ('\0' != channel_list[index]) {
        if ('|' == channel_list[index]) {
            channel_item->triplet[channel_section_count + channel_item->num_bg_band_triplet].channel_prop = (char)atoi((char *)string_data);
            channel_section_count++;
            os_memset(string_data, 0, sizeof(string_data));
            save_index = 0;
            count = 0;
        } else if (',' == channel_list[index]) {
            switch(count) {
            case 0: {
                channel_item->triplet[channel_section_count + channel_item->num_bg_band_triplet].first_channel = (char)atoi((char *)string_data);
                break;
            }
            case 1: {
                channel_item->triplet[channel_section_count + channel_item->num_bg_band_triplet].num_of_ch = (char)atoi((char *)string_data);
                break;
            }
            default:
                return;
            }
            os_memset(string_data, 0, sizeof(string_data));
            save_index = 0;
            count++;
        } else {
            string_data[save_index] = channel_list[index];
            save_index++;
        }
        index++;
    }
    channel_item->triplet[channel_section_count].channel_prop = (char)atoi((char *)string_data);
    if (WIFI_BAND_2_4_G == band) {
        channel_item->num_bg_band_triplet = channel_section_count + 1;
    } else if (WIFI_BAND_5_G == band) {
        channel_item->num_a_band_triplet = channel_section_count + 1;
    }
}


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
uint8_t wifi_connect_scan_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    ch_list_t *scan_channel_list = NULL;
    uint8_t start = atoi(param[0]);

    int32_t len_param = 0;
    P_CMD_IOT_START_SCAN_T scan_param = NULL;

    scan_channel_list=(ch_list_t *)os_malloc(sizeof(ch_list_t)+6*sizeof(ch_desc_t));
    os_memset(scan_channel_list, 0, sizeof(ch_list_t)+6*sizeof(ch_desc_t));

    if (!start) {
        ret = wifi_connection_stop_scan();
    } else {
        uint8_t scan_mode = atoi(param[1]);
        uint8_t scan_option = atoi(param[2]);

        char *chanel_string_2g = NULL;
        char *chanel_string_5g = NULL;
        char *ssid = NULL;
        uint8_t ssid_len = 0;
        uint8_t bssid_val[WIFI_MAX_NUMBER_OF_STA] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
        uint8_t *bssid = (uint8_t *)bssid_val;

        if (len >= 4) {
            chanel_string_2g = param[3];
            if (len == 5) {
                chanel_string_5g = param[4];
                ssid = NULL;
                ssid_len = 0;
                bssid = (uint8_t *)(NULL);
            } else if (len == 6) {
                chanel_string_5g = param[4];
                ssid = param[5];
                ssid_len = os_strlen(ssid);
                bssid = (uint8_t *)(NULL);
            } else if (len >6) {
                chanel_string_5g = param[4];
                ssid = param[5];
                ssid_len = os_strlen(ssid);
                wifi_conf_get_mac_from_str((char *)bssid_val, param[6]);
            }
        } else {
            chanel_string_2g = NULL;
            chanel_string_5g = NULL;
            ssid = NULL;
            ssid_len = 0;
            bssid = (uint8_t *)(NULL);
        }

        if (chanel_string_2g != NULL && strlen(chanel_string_2g) > 4) {
            parse_channel_list_from_string(chanel_string_2g, scan_channel_list, WIFI_BAND_2_4_G);
        }

        if (chanel_string_5g != NULL && strlen(chanel_string_5g) > 4) {
            parse_channel_list_from_string(chanel_string_5g, scan_channel_list, WIFI_BAND_5_G);
        }
        //hex_dump("scan_channel_list",scan_channel_list, sizeof(CH_LIST_T)+6*sizeof(CH_DESC_T));

        len_param = (scan_channel_list->num_a_band_triplet + scan_channel_list->num_bg_band_triplet) * sizeof(ch_desc_t) + sizeof(CMD_IOT_START_SCAN_T);
        scan_param = (P_CMD_IOT_START_SCAN_T)os_malloc(len_param);
        os_memset((unsigned char *)(scan_param), 0x0, len_param);

        /******************reset scan list*******************************/
        wifi_connection_scan_init(g_ap_list, 8);
        //printf("size = %d", sizeof(wifi_scan_list_item_t));
        /******************reset scan list*******************************/


        if (g_scanning && g_scan_by_supplicant) {
            wifi_connection_stop_scan();
            g_scanning = 0;
        }
        g_scan_by_supplicant = 0;
        g_scan_by_app = 1;


        if (ssid && (ssid_len > 0)) {
            os_memcpy(scan_param->SSID, ssid, ssid_len);
            scan_param->enable_ssid = (0x80 | ssid_len);
        }

        if (bssid) {
            os_memcpy(scan_param->BSSID, bssid, WIFI_MAC_ADDRESS_LENGTH);
            scan_param->enable_bssid = 1;
        }

        if (scan_mode > 0) {
            scan_param->partial = 1;
            scan_param->partial_bcn_cnt = 1;
        }
        scan_param->scan_mode = scan_option;
        os_memcpy(&scan_param->channel_list, scan_channel_list, (scan_channel_list->num_a_band_triplet + scan_channel_list->num_bg_band_triplet)* sizeof(ch_desc_t)+2);

        ret = inband_queue_exec(WIFI_COMMAND_ID_IOT_START_SCAN, (unsigned char *)(scan_param), len_param, TRUE, FALSE);
        os_free(scan_param);
        if (ret == 0) {
            g_scanning = 1;
        }

    }

    if (ret < 0) {
        status = 1;
    }
    printf("wifi_connect_scan_ex(), ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);

    return status;
}

/**
* @brief Example of Get WiFi TX Power
* wifi config get tx_power
* @get power: 64~190
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_tx_power_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t tx_power = 0;

    ret = wifi_config_get_tx_power(&tx_power);
    if (ret < 0) {
        status = 1;
    }
    printf("wifi_config_get_tx_power_ex tx_power:%d, ret:%s, Code=%ld\n", tx_power, WIFI_CLI_RETURN_STRING(ret), ret);

    return status;
}

/**
* @brief Example of Set WiFi TX Power
* wifi config set tx_power <tx power>
* @param [IN]interval 64 ~ 190
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_tx_power_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    uint8_t tx_power = atoi(param[0]);

    ret = wifi_config_set_tx_power(tx_power);
    if (ret < 0) {
        status = 1;
    }
    printf("wifi_config_set_tx_power_ex tx_power:%d, ret:%s, Code=%ld\n", tx_power, WIFI_CLI_RETURN_STRING(ret), ret);

    return status;
}

static xQueueHandle n9log_queue = NULL;
static xTaskHandle n9log_task = NULL;

log_create_module(n9log, PRINT_LEVEL_INFO);

int wifi_n9log_handler(unsigned char *body, int len)
{
    if (len >= 1024) {
        LOG_HEXDUMP_E(n9log, "n9log packet length is longer than 1024", body, len);
        return 1;
    }
    xQueueSend(n9log_queue, body, 0);
    return 0;
}
void n9log_task_entry(void *args)
{
    uint8_t n9log_buffer[MAX_N9LOG_SIZE] = {0};
    for (;;) {
        xQueueReceive(n9log_queue, n9log_buffer, portMAX_DELAY);
        LOG_I(n9log, "%s", n9log_buffer);
    }
}
void n9log_queue_init(void)
{
    if (n9log_queue == NULL) {
        n9log_queue = xQueueCreate(3, MAX_N9LOG_SIZE+1);
        if (NULL == n9log_queue) {
            LOG_E(n9log, "create n9log queue failed.");
        }
    } else {
        LOG_I(n9log, "n9log_queue is not null, please deinit first.");
    }
    if (n9log_task == NULL ) {
        if (pdPASS != xTaskCreate(n9log_task_entry,
                                  N9LOG_TASK_NAME,
                                  N9LOG_TASK_STACKSIZE,
                                  NULL,
                                  N9LOG_TASK_PRIO,
                                  &n9log_task)) {
            LOG_I(n9log, "create n9log task failed.");
       }
    } else {
        LOG_I(n9log, "n9log_task is still running.");
    }
    wifi_config_register_n9log_handler((N9LOG_HANDLER)wifi_n9log_handler);
}
void n9log_queue_deinit(void)
{
    if (n9log_task != NULL) {
        vTaskDelete(n9log_task);
        n9log_task = NULL;
    }
    if (n9log_queue != NULL) {
        vQueueDelete(n9log_queue);
        n9log_queue = NULL;
    }
}
uint8_t wifi_config_set_n9log_state_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    if (len < 2) {
        printf("Usage: \n");
        printf("n9log set off  -> Disable n9 logs\n");
        printf("n9log set n9   -> output n9 logs to uart\n");
        printf("n9log set host -> output n9 logs to host\n");
        return 1;
    }
    if (!os_strcmp(param[0], "set")){
        if (!os_strcmp(param[1], "off")){
            if ((ret = wifi_inband_set_n9_consol_log_state(0)) < 0) {
                status = 1;
            } else {
                 n9log_queue_deinit();
            }
        } else if (!os_strcmp(param[1], "n9")){
            if ((ret = wifi_inband_set_n9_consol_log_state(1)) < 0) {
                status = 1;
            } else {
                n9log_queue_deinit();
            }
        } else if (!os_strcmp(param[1], "host")){
            if ((ret = wifi_inband_set_n9_consol_log_state(2)) < 0) {
                  status = 1;
            } else {
                n9log_queue_init();
            }
        } else {
            printf("Not Supported Cmd\n");
            return 1;
        }
    } else {
        printf("Not Supported Cmd\n");
        return 1;
    }
    printf("wifi_config_set_n9log_state_ex, ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);
    return status;
}

/**
* @brief Example of Set Country Code
* wifi config set country_code <code>
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_set_country_code_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    wifi_country_code_t *country_code;

    printf("Set country code: %s.\n", param[0]);

    if(strlen(param[0])> 2) {
        printf("Invalid country code length: %d.\n", strlen(param[0]));
        return 1;
    }

    if(os_memcmp(param[0],"CN", 2) == 0) {
        country_code = (wifi_country_code_t *)os_malloc(sizeof(wifi_country_code_t) + 2*sizeof(wifi_channel_table_t));
        os_memset(country_code, 0, sizeof(wifi_country_code_t) + 2*sizeof(wifi_channel_table_t));
        os_memcpy(country_code, param[0], 2);
        country_code->num_of_channel_table = 2;
        country_code->channel_table[0].first_channel = 1;
        country_code->channel_table[0].num_of_channel = 13;
        country_code->channel_table[0].dfs_req = 0;
        country_code->channel_table[0].max_tx_power = 20;
        country_code->channel_table[1].first_channel = 149;
        country_code->channel_table[1].num_of_channel = 4;
        country_code->channel_table[1].dfs_req = 0;
        country_code->channel_table[1].max_tx_power = 27;
    }else if(os_memcmp(param[0],"US", 2) == 0) {
        country_code = (wifi_country_code_t *)os_malloc(sizeof(wifi_country_code_t) + 5*sizeof(wifi_channel_table_t));
        os_memset(country_code, 0, sizeof(wifi_country_code_t) + 5*sizeof(wifi_channel_table_t));
        os_memcpy(country_code, param[0], 2);
        country_code->num_of_channel_table = 5;
        country_code->channel_table[0].first_channel = 1;
        country_code->channel_table[0].num_of_channel = 11;
        country_code->channel_table[0].dfs_req = 0;
        country_code->channel_table[0].max_tx_power = 30;
        country_code->channel_table[1].first_channel = 36;
        country_code->channel_table[1].num_of_channel = 4;
        country_code->channel_table[1].dfs_req = 0;
        country_code->channel_table[1].max_tx_power = 17;
        country_code->channel_table[2].first_channel = 52;
        country_code->channel_table[2].num_of_channel = 4;
        country_code->channel_table[2].dfs_req = 1;
        country_code->channel_table[2].max_tx_power = 24;
        country_code->channel_table[3].first_channel = 100;
        country_code->channel_table[3].num_of_channel = 11;
        country_code->channel_table[3].dfs_req = 1;
        country_code->channel_table[3].max_tx_power = 30;
        country_code->channel_table[4].first_channel = 149;
        country_code->channel_table[4].num_of_channel = 5;
        country_code->channel_table[4].dfs_req = 0;
        country_code->channel_table[4].max_tx_power = 30;
    }else if(os_memcmp(param[0],"UK", 2) == 0) {
        country_code = (wifi_country_code_t *)os_malloc(sizeof(wifi_country_code_t) + 4*sizeof(wifi_channel_table_t));
        os_memset(country_code, 0, sizeof(wifi_country_code_t) + 4*sizeof(wifi_channel_table_t));
        os_memcpy(country_code, param[0], 2);
        country_code->num_of_channel_table = 4;
        country_code->channel_table[0].first_channel = 1;
        country_code->channel_table[0].num_of_channel = 13;
        country_code->channel_table[0].dfs_req = 0;
        country_code->channel_table[0].max_tx_power = 20;
        country_code->channel_table[1].first_channel = 36;
        country_code->channel_table[1].num_of_channel = 4;
        country_code->channel_table[1].dfs_req = 0;
        country_code->channel_table[1].max_tx_power = 23;
        country_code->channel_table[2].first_channel = 52;
        country_code->channel_table[2].num_of_channel = 4;
        country_code->channel_table[2].dfs_req = 1;
        country_code->channel_table[2].max_tx_power = 23;
        country_code->channel_table[3].first_channel = 100;
        country_code->channel_table[3].num_of_channel = 11;
        country_code->channel_table[3].dfs_req = 1;
        country_code->channel_table[3].max_tx_power = 30;
    } else {
        printf("Only support country code: CN, US, UK for reference design.\n");
        return 1;
    }
    ret = wifi_config_set_country_code(country_code);
    os_free(country_code);
    if (ret < 0) {
        status = 1;
    }

    printf("wifi_config_set_country_code_ex, ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);
    return status;
}

/**
* @brief Example of Get Country Code
* wifi config get country_code
*
* @return =0 means success, >0 means fail
*/
uint8_t wifi_config_get_country_code_ex(uint8_t len, char *param[])
{
    uint8_t status = 0;
    int32_t ret = 0;
    wifi_country_code_t country_code = {{0}};

    ret = wifi_config_get_country_code(&country_code);
    if (ret < 0) {
        status = 1;
    }
    printf("country code :%s\n", (char *)country_code.country_code);

    printf("wifi_config_get_country_code_ex, ret:%s, Code=%ld\n", WIFI_CLI_RETURN_STRING(ret), ret);
    return status;
}

