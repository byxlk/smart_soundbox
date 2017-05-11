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


#include "wifi_private_api.h"
#include "connsys_profile.h"
#include "wifi_init.h"
#include "hal_define.h"
#include "syslog.h"
#include "os.h"
#include "os_util.h"
#include "wifi_freertos_adapter.h"
#include "wifi_os_api.h"
#include "wifi_channel.h"

wifi_country_code_t *country_channel_list = NULL;
os_semaphore_t ch_mutex = NULL;

int32_t wifi_take_channel_list_mutex(void)
{
    if(wifi_os_semphr_take(ch_mutex, NULL) < 0) {
        LOG_E(wifi,"mutex take fail.");
        return -1;
    }
    return 0;
}
int32_t wifi_give_channel_list_mutex(void)
{
    return wifi_os_semphr_give(ch_mutex);
}
/*Note: When use this function, please add the mutex fuction above to protect multiple thread read/write the same variable.*/
wifi_country_code_t *wifi_get_country_channel_list(void)
{
    return country_channel_list;
}

int32_t wifi_build_country_channel_list(wifi_country_code_t *country_channel)
{
    if(wifi_os_semphr_take(ch_mutex, NULL) < 0) {
        LOG_E(wifi,"mutex take fail.");
        return -1;
    }
    if (country_channel_list != NULL){
        os_free(country_channel_list);
    }
    country_channel_list = (wifi_country_code_t *)os_malloc(sizeof(wifi_country_code_t) + (country_channel->num_of_channel_table)*sizeof(wifi_channel_table_t));
    if(country_channel_list == NULL) {
        LOG_E(wifi,"malloc country_channel_list fail.");
        return -1;
    }
    os_memcpy(country_channel_list, country_channel, sizeof(wifi_country_code_t) + (country_channel->num_of_channel_table)*sizeof(wifi_channel_table_t));
    wifi_os_semphr_give(ch_mutex);
    return 0;
}

int32_t wifi_channel_list_init(sys_cfg_t *syscfg)
{
    uint8_t index;
    wifi_country_code_t *country = NULL;
    ch_mutex = wifi_os_semphr_create_mutex();
    if(ch_mutex == NULL) {
        LOG_E(wifi,"create mutex fail.");
        return -1;
    }
    country = (wifi_country_code_t *)os_malloc(sizeof(wifi_country_code_t) + (syscfg->bg_band_entry_num + syscfg->a_band_entry_num)*sizeof(wifi_channel_table_t));
    if(country == NULL) {
        LOG_E(wifi,"malloc  fail.");
        return -1;
    }
    os_memcpy(country->country_code, syscfg->country_code, sizeof(syscfg->country_code));
    country->num_of_channel_table = syscfg->bg_band_entry_num + syscfg->a_band_entry_num;
    for(index=0;index<country->num_of_channel_table;index++){
        if(index < syscfg->bg_band_entry_num){
            country->channel_table[index].first_channel = syscfg->bg_band_triple[index].first_channel;
            country->channel_table[index].num_of_channel = syscfg->bg_band_triple[index].num_of_ch;
            country->channel_table[index].max_tx_power = syscfg->bg_band_triple[index].tx_power;
            country->channel_table[index].dfs_req = syscfg->bg_band_triple[index].channel_prop;
        } else {
            country->channel_table[index].first_channel = syscfg->a_band_triple[index-syscfg->bg_band_entry_num].first_channel;
            country->channel_table[index].num_of_channel = syscfg->a_band_triple[index-syscfg->bg_band_entry_num].num_of_ch;
            country->channel_table[index].max_tx_power = syscfg->a_band_triple[index-syscfg->bg_band_entry_num].tx_power;
            country->channel_table[index].dfs_req = syscfg->a_band_triple[index-syscfg->bg_band_entry_num].channel_prop;
        }
    }
    if(wifi_build_country_channel_list(country) < 0){
        os_free(country);
        return -1;
    }
    os_free(country);
    return 0;

}

uint8_t wifi_parse_bg_band_num(wifi_country_code_t *country_channel)
{
    uint8_t bg_num = 0, index;
    for(index = 0;index < country_channel->num_of_channel_table; index++) {
        if(country_channel->channel_table[index].first_channel <= 14) {
            bg_num++;
        }
    }
    return bg_num;
}

uint8_t wifi_parse_a_band_num(wifi_country_code_t *country_channel)
{
    uint8_t a_num = 0, index;
    for(index = 0;index < country_channel->num_of_channel_table; index++) {
        if(country_channel->channel_table[index].first_channel > 14) {
            a_num++;
        }
    }
    return a_num;
}

