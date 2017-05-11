/* Copyright Statement:
 *
 * (C) 2005-2016  MediaTek Inc. All rights reserved.
 *
 * This software/firmware and related documentation (MediaTek Software) are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. (MediaTek) and/or its licensors.
 * Without the prior written permission of MediaTek and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 * You may only use, reproduce, modify, or distribute (as applicable) MediaTek Software
 * if you have agreed to and been bound by the applicable license agreement with
 * MediaTek (License Agreement) and been granted explicit permission to do so within
 * the License Agreement (Permitted User).  If you are not a Permitted User,
 * please cease any access or use of MediaTek Software immediately.
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT MEDIATEK SOFTWARE RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES
 * ARE PROVIDED TO RECEIVER ON AN AS-IS BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL
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


#include <string.h>
#include <stdlib.h>
#include "stdio.h"

#include "gnss_app.h"

// This option is used to enable connect powerGPS tool via UART
// This feature is only for debug usage, and will impact the power consumption.
// You can disable it when you don't need it.
#ifdef GNSS_SUPPORT_TOOL_BRIDGE

#include "hal_uart.h"
//#include "pinmux.h"
#include "gnss_bridge.h"
#include "memory_attribute.h"

#define GNSS_BRIDGE_GPIO_PIN1 eIOT_GPIO_IDX_0
#define GNSS_BRIDGE_GPIO_PIN2 eIOT_GPIO_IDX_1
#define GNSS_BRIDGE_GPIO_PIN3 eIOT_GPIO_IDX_2
#define GNSS_BRIDGE_GPIO_PIN4 eIOT_GPIO_IDX_3
#define GNSS_BRIDGE_BAUDRATE_RATE HAL_UART_BAUDRATE_115200
#define GNSS_BRIDGE_WORDLENGTH HAL_UART_WORD_LENGTH_8
#define GNSS_BRIDGE_STOPBIT HAL_UART_STOP_BIT_1
#define GNSS_BRIDGE_PARITY HAL_UART_PARITY_NONE


#define GNSS_BRIDGE_DMA_RX_BUF_SIZE 768
#define GNSS_BRIDGE_DMA_RX_ALERT_LEN 32
#define GNSS_BRIDGE_DMA_RX_THRESHOLD 256
#define GNSS_BRIDGE_DMA_TX_BUF_SIZE 2560
#define GNSS_BRIDGE_DMA_TX_THRESHOLD 128
static hal_uart_port_t gnss_bridge_port;
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN uint8_t g_bridge_dma_rx_buffer[GNSS_BRIDGE_DMA_RX_BUF_SIZE];
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN uint8_t g_bridge_dma_tx_buffer[GNSS_BRIDGE_DMA_TX_BUF_SIZE];
static gnss_bridge_callback_f gnss_bridge_callback;


/**
* @brief       interrupt callback function, used to notify user UART event
* @param[in]   status: UART event
* @param[in]   parameter: parameter
* @return      None
*/
static void gnss_uart_bridge_irq(hal_uart_callback_event_t status, void *parameter)
{
    if (HAL_UART_EVENT_READY_TO_READ == status) {
        gnss_bridge_callback(GNSS_BRIDGE_CALLBACK_TYPE_CAN_READ);        
    } else if (HAL_UART_EVENT_READY_TO_WRITE == status) {
        gnss_bridge_callback(GNSS_BRIDGE_CALLBACK_TYPE_CAN_WRITE);        
    }
}

/**
* @brief       This function is used to init UART
* @param[in]   port: UART port will be init
* @param[in]   callback: This callback function will be called when UART event comes. e.g. HAL_UART_EVENT_READY_TO_WRITE message when UART buffer is availbe
* @return      None
*/
void gnss_uart_bridge_init(hal_uart_port_t port, gnss_bridge_callback_f callback)
{    
    hal_uart_config_t uart_config;
    hal_uart_dma_config_t dma_config;
    
    gnss_bridge_port = port;
    uart_config.baudrate = GNSS_BRIDGE_BAUDRATE_RATE;
    uart_config.word_length = GNSS_BRIDGE_WORDLENGTH;
    uart_config.stop_bit = GNSS_BRIDGE_STOPBIT;
    uart_config.parity = GNSS_BRIDGE_PARITY;
    
    dma_config.receive_vfifo_buffer = g_bridge_dma_rx_buffer;
    dma_config.receive_vfifo_buffer_size = GNSS_BRIDGE_DMA_RX_BUF_SIZE;
    dma_config.receive_vfifo_alert_size = GNSS_BRIDGE_DMA_RX_ALERT_LEN;
    dma_config.receive_vfifo_threshold_size = GNSS_BRIDGE_DMA_RX_THRESHOLD;
    dma_config.send_vfifo_buffer = g_bridge_dma_tx_buffer;
    dma_config.send_vfifo_buffer_size = GNSS_BRIDGE_DMA_TX_BUF_SIZE;
    dma_config.send_vfifo_threshold_size = GNSS_BRIDGE_DMA_TX_THRESHOLD;  
    gnss_bridge_callback = callback;
    
    if (HAL_UART_STATUS_OK == hal_uart_init(gnss_bridge_port, &uart_config)) {
        GNSSLOGD("uart init success 1\n");
        hal_uart_set_dma(gnss_bridge_port, &dma_config);
        hal_uart_register_callback(gnss_bridge_port, gnss_uart_bridge_irq, NULL);
    } else {
        if (HAL_UART_STATUS_OK == hal_uart_deinit(gnss_bridge_port) ) {
            if (HAL_UART_STATUS_OK == hal_uart_init(gnss_bridge_port, &uart_config)) {
                GNSSLOGD("uart init success 2\n");
                hal_uart_set_dma(gnss_bridge_port, &dma_config);
                hal_uart_register_callback(gnss_bridge_port, gnss_uart_bridge_irq, NULL);
            } else {
                GNSSLOGD("uart init fail 1\n");
            }
        } else {
            GNSSLOGD("uart init fail 2\n");
        }
    }
    GNSSLOGD("GNSS bridge UART Init");
    
}

/**
* @brief       Deinit the registered uart
* @return      None
*/
void gnss_uart_bridge_deinit()
{
    if (HAL_UART_STATUS_OK == hal_uart_deinit(gnss_bridge_port) ) {
        GNSSLOGD("uart deinit succeed\r\n");
    } else {
        GNSSLOGD("uart deinit fail\r\n");
    }
}


/**
* @brief       read data from UART
* @param[in]   buf: pointer to buffer
* @param[in]   buf_len: buffer size
* @return      None
*/
uint32_t gnss_bridge_read_command(uint8_t* buf, int32_t buf_len)
{
    int ret_len;
    
    ret_len = hal_uart_receive_dma(gnss_bridge_port, (uint8_t *)buf, (uint32_t) buf_len);
    //GNSSLOGD("gnss_bridge_read_command, ret_len:%d\r\n", ret_len);
    buf[ret_len] = 0;
    return ret_len;
}

/**
* @brief       Send data to UART
* @param[in]   buf: pointer to buffer
* @param[in]   buf_len: buffer size
* @return      None
*/
uint32_t gnss_bridge_send_debug_data(uint8_t* buf, int32_t buf_len)
{
    int ret_len;
    
    ret_len = hal_uart_send_dma(gnss_bridge_port, buf, (uint32_t) buf_len);
    
    //GNSSLOGD("hal_uart_send_dma, buf_len:%d,ret_len:%d\r\n", buf_len, ret_len);
    return ret_len;
}
#endif


