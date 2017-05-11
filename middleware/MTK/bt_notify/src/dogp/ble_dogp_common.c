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
//#ifdef MTK_BLE_DOGP_ENABLE

 
#include "bt_gap_le.h"
#include "ble_dogp_adp_service.h"
#include "bt_gattc.h"
#include "bt_debug.h"
#include "bt_gatts.h"
#include "ble_dogp_common.h"
#include "bt_debug.h"
#include "bt_callback_manager.h"
#include "bt_hci.h"
#include "syslog.h"

log_create_module(DOGP_CM, PRINT_LEVEL_INFO);
static ble_gap_conn_param_priority_t g_ble_gap_curr_conn_param = BLE_GAP_CONN_PARAM_PRIORITY_DEFAULT;

ble_dogp_adp_low_powe_cntx_t g_dogp_adp_low_power_cntx = {0};

#if 0
static bt_gap_le_smp_pairing_config_t g_dogp_pairing_config_req = {
    .auth_req = BT_GAP_LE_SMP_AUTH_REQ_BONDING,
    .io_capability = BT_GAP_LE_SMP_NO_INPUT_NO_OUTPUT,
    .maximum_encryption_key_size = 16,
};

static bt_gap_le_local_key_t g_dogp_local_key_req = {
    .encryption_info = {{0}},
    .master_id = {0},
    .identity_info = {{0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x19, 0x28, 0x55, 0x33, 0x68, 0x33, 0x56, 0xde}},
    .signing_info = {{0}}
};

static bool g_dogp_sc_only = false;
static bt_gap_le_bonding_info_t g_dogp_bonding_info = {{{0}}};
static bt_gap_le_local_config_req_ind_t g_dogp_local_config;
#endif

static TimerHandle_t xTimer_low_power = NULL;
#define BLE_DOGP_TIMER_PERIOD       15000



#if 0
bt_gap_le_local_config_req_ind_t *ble_dogp_gap_le_get_local_config(void)
{
    LOG_I(DOGP_CM, "bt_gap_le_get_local_config \r\n");

    g_dogp_local_config.local_key_req = &g_dogp_local_key_req;
    g_dogp_local_config.sc_only_mode_req = g_dogp_sc_only;

    return &g_dogp_local_config;
}

bt_gap_le_bonding_info_t *ble_dogp_gap_le_get_bonding_info(const bt_addr_t remote_addr)
{
    LOG_I(DOGP_CM, "bt_gap_le_get_bonding_info \r\n");

    return &g_dogp_bonding_info;
}

bt_status_t ble_dogp_gap_le_get_pairing_config(bt_gap_le_bonding_start_ind_t *ind)
{
    LOG_I(DOGP_CM, "bt_gap_le_get_pairing_config \r\n");

    ind->pairing_config_req = g_dogp_pairing_config_req;
    return BT_STATUS_SUCCESS;
}
#endif
bt_status_t bt_dogp_event_callback(bt_msg_type_t msg, bt_status_t status, void *buff)
{
    bt_status_t sta;
    LOG_I(DOGP_CM, "%s: status(0x%04x)", __FUNCTION__, status);
    
    if(status != BT_STATUS_SUCCESS) {
        return status;
    }
    
    sta = ble_dogp_adp_on_ble_evt(msg, buff);
    if (sta != BT_STATUS_SUCCESS)
        return sta;
    
    sta = bt_dogp_app_event_callback(msg, buff);
    if (sta != BT_STATUS_SUCCESS)
        return sta;
    
    return BT_STATUS_SUCCESS;
}

bt_status_t ble_dogp_event_callback_init(void)
{
    bt_status_t result;
    #if 0
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_local_cofig, 0, (void *)ble_dogp_gap_le_get_local_config);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_pairing_config, 0, (void *)ble_dogp_gap_le_get_pairing_config);
    bt_callback_manager_register_callback(bt_callback_type_gap_le_get_bonding_info, 0, (void *)ble_dogp_gap_le_get_bonding_info);
    #endif
    result = bt_callback_manager_register_callback(bt_callback_type_app_event, MODULE_MASK_SYSTEM | MODULE_MASK_GAP, (void*)bt_dogp_event_callback);
    if (result != BT_STATUS_SUCCESS) {
        LOG_I(DOGP_CM, "ble_dogp_event_callback_init fail! \r\n");
    }
    return result;
}

/**
 * @brief Function for updating connection params to client.
 *
 * @param[in]  conn_id	 connection id.
 * @param[in]  app_name	 application name, who trigger the request.
 *
 * @param[out] status    result of updating connection params request.
 */
bt_status_t ble_dogp_update_connection_interval(uint16_t conn_handle, ble_gap_conn_param_priority_t conn_priority)
{
    bt_status_t status;
    bt_hci_cmd_le_connection_update_t conn_params;
    if (g_ble_gap_curr_conn_param == conn_priority) {
        return BT_STATUS_FAIL;
    }
    g_ble_gap_curr_conn_param = conn_priority;
    conn_params.supervision_timeout = 0x0258;            /** TBC: 6000ms : 600 * 10 ms. */
    conn_params.connection_handle = conn_handle;
    LOG_I(DOGP_CM, "ble_dogp_update_connection_interval, conn_priority %d\r\n",conn_priority);

    switch (conn_priority) {
        case BLE_GAP_CONN_PARAM_HIGH_SPEED_ANDROID: {
            conn_params.conn_interval_min = 0x0006;/** TBC: 7.5ms : 6 * 1.25 ms. */
            conn_params.conn_interval_max = 0x0006;/** TBC: 7.5ms : 6 * 1.25 ms. */
            conn_params.conn_latency = 0;
        }
        break;
        case BLE_GAP_CONN_PARAM_HIGH_SPEED_IOS: {
            conn_params.conn_interval_min = 0x0010;/** TBC: 20ms : 16 * 1.25 ms. */
            conn_params.conn_interval_max = 0x0020;/** TBC: 40ms : 32 * 1.25 ms. */
            conn_params.conn_latency = 0;
        }
        break;
        case BLE_GAP_CONN_PARAM_LOW_POWER:
        case BLE_GAP_CONN_PARAM_PRIORITY_DEFAULT: {
            conn_params.conn_interval_min = 0x0120;/** TBC: 360ms : 288 * 1.25 ms. */
            conn_params.conn_interval_max = 0x0130;/** TBC: 380ms : 304 * 1.25 ms. */
            conn_params.conn_latency = 4;
        }
        break;
        default: {
            conn_params.conn_interval_min = 0x0120;/*TBC: 360ms : 288 * 1.25 ms*/
            conn_params.conn_interval_max = 0x0130;/*TBC: 380ms : 304 * 1.25 ms*/
            conn_params.conn_latency = 4;
        }
        break;
    }

    status = bt_gap_le_update_connection_parameter(&conn_params);
    return status;
}


/**
 * @brief Function for getting the current connection interval.
 *
 * @param[in]  conn_updated	 the information of BLE_GAP_CONNECTION_PARAM_UPDATED_IND event.
 *
 * @param[out] priority      the current connection interval.
 */
ble_gap_conn_param_priority_t ble_gap_get_current_connection_interval(uint16_t conn_interval)
{
    LOG_I(DOGP_CM, "ble_gap_get_current_connection_interval, conn_interval %d\r\n",conn_interval);

    if (conn_interval == 0x0010) {
        return BLE_GAP_CONN_PARAM_HIGH_SPEED_ANDROID;
    } else if ((conn_interval >= 0x0010) && (conn_interval <= 0x0020)) {
        return BLE_GAP_CONN_PARAM_HIGH_SPEED_IOS;
    } else if ((conn_interval >= 0x0120) && (conn_interval <= 0x0130)) {
        return BLE_GAP_CONN_PARAM_LOW_POWER;
    }
    return BLE_GAP_CONN_PARAM_PRIORITY_DEFAULT;
}

static void ble_dogp_adp_update_connection_interval_timerout(TimerHandle_t xTimer)
{
    LOG_I(DOGP_CM, "ble_dogp_adp_update_connection_interval_timerout \r\n");

    int32_t status = BT_STATUS_FAIL;
    ble_gap_conn_param_priority_t interval = BLE_GAP_CONN_PARAM_PRIORITY_DEFAULT;

    if (xTimerIsTimerActive(xTimer_low_power)) {
        xTimerStop(xTimer_low_power, 0);
    }

    interval = BLE_GAP_CONN_PARAM_LOW_POWER;
    status = ble_dogp_update_connection_interval(g_dogp_adp_low_power_cntx.conn_handle, interval);
    if (status == BT_STATUS_SUCCESS) {
        g_dogp_adp_low_power_cntx.conn_priority = interval;
    }
    LOG_I(DOGP_CM, "[BLE_DOGP] update_connection_interval_timerout, status = %d, interval = %d \r\n", status, interval);
}

int32_t ble_dogp_adp_update_connection_interval(uint16_t conn_handle)
{
    LOG_I(DOGP_CM, "ble_dogp_adp_update_connection_interval conn_handle %d\r\n", conn_handle);

    int32_t status = BT_STATUS_FAIL;
    ble_gap_conn_param_priority_t interval = BLE_GAP_CONN_PARAM_PRIORITY_DEFAULT;

    if ((conn_handle != 0) && (conn_handle == g_dogp_adp_low_power_cntx.conn_handle)) {
        if (BLE_DOGP_ADP_REMOTE_DEVICE_IOS == g_dogp_adp_low_power_cntx.remote_device_type) {
            interval = BLE_GAP_CONN_PARAM_HIGH_SPEED_IOS;
        } else {
            interval = BLE_GAP_CONN_PARAM_HIGH_SPEED_ANDROID;
        }
        status = ble_dogp_update_connection_interval(conn_handle, interval);
        if (status == BT_STATUS_SUCCESS) {
            g_dogp_adp_low_power_cntx.conn_priority = interval;
            xTimer_low_power = xTimerCreate("DOGP_LOW_POWER_TIMER",
                                          BLE_DOGP_TIMER_PERIOD / portTICK_PERIOD_MS, pdFALSE,
                                          ( void *)0,
                                          ble_dogp_adp_update_connection_interval_timerout);

            LOG_I(DOGP_CM,  "[BLE_DOGP]xTimer_low_power:%d\r\n", xTimer_low_power);

            if (xTimer_low_power == NULL) {
                LOG_I(DOGP_CM,  "[BLE_DOGP]init create LOW_POWER_TIMER fail\r\n");
            }
        }
        LOG_I(DOGP_CM, "[BLE_DOGP] ble_dogp_adp_update_connection_interval, status = %d, interval = %d \r\n", status, interval);
    }
    return status;

}
#if 0
char *device_name = "DOGP_DEVICE";

void ble_dogp_adp_advertise_data_init(void)
{
    LOG_I(DOGP_CM, "ble_dogp_adp_advertise_data_init\r\n");

    bt_hci_cmd_le_set_advertising_parameters_t adv_param = {
        .advertising_interval_min = 0xC0,
        .advertising_interval_max = 0xC0,
        .advertising_type = BT_HCI_ADV_TYPE_CONNECTABLE_UNDIRECTED,
        .own_address_type = BT_ADDR_RANDOM,
        .advertising_channel_map = 7,
        .advertising_filter_policy = 0
    };

    bt_hci_cmd_le_set_advertising_enable_t enable;
    bt_hci_cmd_le_set_advertising_data_t adv_data = {
        .advertising_data_length = 31,
    };

    memcpy(&adv_data.advertising_data[5], device_name, strlen(device_name));
    adv_data.advertising_data[0] = 2; 
    adv_data.advertising_data[1] = BT_GAP_LE_AD_TYPE_FLAG;
    adv_data.advertising_data[2] = BT_GAP_LE_AD_FLAG_BR_EDR_NOT_SUPPORTED | BT_GAP_LE_AD_FLAG_GENERAL_DISCOVERABLE;
    adv_data.advertising_data[3] = strlen(device_name)+1;
    adv_data.advertising_data[4] = BT_GAP_LE_AD_TYPE_NAME_COMPLETE;
    adv_data.advertising_data[5 + strlen(device_name)] = 0x0;
    
    enable.advertising_enable = BT_HCI_ENABLE;

    bt_gap_le_set_advertising(&enable, &adv_param, &adv_data, NULL);
    
    bt_gatts_set_max_mtu(512);
}
#endif

bt_status_t bt_dogp_app_event_callback(bt_msg_type_t msg, void *buff)
{
    bt_status_t status = BT_STATUS_SUCCESS;
    LOG_I(DOGP_CM, "bt_dogp_app_event_callback msg = 0x%04x \r\n", msg);

     switch (msg)
     {
        #if 0
         case BT_POWER_ON_CNF:
             ble_dogp_adp_advertise_data_init();
             break;
        #endif
         case BT_GAP_LE_CONNECT_IND:
         {
             bt_gap_le_connection_ind_t *connection_ind = (bt_gap_le_connection_ind_t *)buff;
             LOG_I(DOGP_CM, "connection handle=0x%04x", connection_ind->connection_handle);

             /**< only support single link. */
             if (0 == g_dogp_adp_low_power_cntx.conn_handle) {
                 g_dogp_adp_low_power_cntx.conn_interval= connection_ind->conn_interval;
                 g_dogp_adp_low_power_cntx.conn_handle = connection_ind->connection_handle;
                 //ble_dogp_adp_update_connection_interval(connection_ind->connection_handle);
            }  
             //bt_gap_le_bond(connection_ind->connection_handle, &pairing_config);

         }
             break;      
             
         case BT_GAP_LE_BONDING_REPLY_REQ_IND:
             break;

         case BT_GAP_LE_CONNECTION_UPDATE_CNF:
         case BT_GAP_LE_CONNECTION_UPDATE_IND:
         {
             bt_gap_le_connection_update_ind_t *ind = (bt_gap_le_connection_update_ind_t *)buff;

             if ((ind->conn_handle != 0) && (ind->conn_handle == g_dogp_adp_low_power_cntx.conn_handle)) {
                 LOG_I(DOGP_CM, "CONNECTION UPDATE: event_id = %x, interval = %d\n", msg, ind->conn_interval);
                 g_dogp_adp_low_power_cntx.conn_interval = ind->conn_interval;
                 g_dogp_adp_low_power_cntx.conn_priority = ble_gap_get_current_connection_interval(ind->conn_interval);
             }
         }
            break;
            
         case BT_GAP_LE_DISCONNECT_IND:  
         {
             bt_hci_evt_disconnect_complete_t *connection_ind = (bt_hci_evt_disconnect_complete_t *)buff;
             uint16_t conn_handle = connection_ind->connection_handle;
             if ((conn_handle != 0) && (conn_handle == g_dogp_adp_low_power_cntx.conn_handle)) {
                 memset(&g_dogp_adp_low_power_cntx, 0, sizeof(ble_dogp_adp_low_powe_cntx_t));
                 
                 #if 0
                 ble_dogp_adp_advertise_data_init();
                 #endif
             }
         }
            break;
            
         default:
             break;
     }
    return status;
}

/**
 * @brief Function for application to set remote device's type, android or ios device.
 */
void ble_dogp_adp_set_remote_device_type(uint16_t conn_handle, ble_dogp_adp_remote_device_type_t type)
{
    if ((conn_handle != 0) && (conn_handle == g_dogp_adp_low_power_cntx.conn_handle)) {
        g_dogp_adp_low_power_cntx.remote_device_type = (uint8_t)type;
    }
    LOG_I(DOGP_CM, "[BLE_DOGP] ble_dogp_adp_set_remote_device_type: conn_handle is [%d], master_type is [%d]\r\n", conn_handle, type);
}




//#endif /*MTK_BLE_DOGP_ENABLE*/
