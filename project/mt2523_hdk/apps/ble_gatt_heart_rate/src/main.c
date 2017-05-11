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

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
/* device.h includes */
#include "mt2523.h"

/* hal includes */
#include "hal_cache.h"
#include "hal_uart.h"
#include "hal_clock.h"
#include "hal_clock_internal.h"
#include "hal_dvfs.h"
//#include "hal_sleep_manager.h"
#include "bt_init.h"
#include "hal_flash.h"
#include "hal_nvic.h"
#include "nvdm.h"
#include "hal_sleep_manager.h"

#include "bt_debug.h"
#include "memory_map.h"
#include "hal_display_lcd.h"
#include "bsp_gpio_ept_config.h"
#include "bt_gattc_connect.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/


#define VFIFO_SIZE_RX_BT (512)
#define VFIFO_SIZE_TX_BT (1024)
#define VFIFO_ALERT_LENGTH_BT (20)

/* Private variables ---------------------------------------------------------*/
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t   g_bt_cmd_tx_vfifo[VFIFO_SIZE_TX_BT];
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t   g_bt_cmd_rx_vfifo[VFIFO_SIZE_RX_BT];

static hal_uart_port_t      g_atci_bt_port;
static SemaphoreHandle_t    g_bt_io_semaphore = NULL;

/* Private variables ---------------------------------------------------------*/

#ifdef MTK_SYSTEM_CLOCK_26M
static const uint32_t target_freq = 26000;
#else
#ifdef MTK_SYSTEM_CLOCK_104M
static const uint32_t target_freq = 104000;
#else
static const uint32_t target_freq = 208000;
#endif
#endif
extern uint8_t sleep_manager_handle;
/* Private functions ---------------------------------------------------------*/
static void SystemClock_Config(void);
static void prvSetupHardware( void );
extern void heart_rate_task_init(void);
bool bt_hci_log_enabled(void)
{
    return TRUE;
}
void vApplicationTickHook(void)
{

}

int BT_XFile_EncryptionCommand()
{
    return 0;
}

/**
* @brief       This function is low-level libc implementation, which is used for printf family.
* @param[in]   ch: the character that will be put into uart port.
* @return      The character as input.
*/
#ifdef __GNUC__
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif
{

    /* Place your implementation of fputc here */
    /* e.g. write a character to the uart port configured in log_uart_init() */
    log_putchar(ch);
    return ch;

}

static void cache_init(void)
{
    hal_cache_region_t region, region_number;

    /* Max region number is 16 */
    hal_cache_region_config_t region_cfg_tbl[] = {
        /* cacheable address, cacheable size(both MUST be 4k bytes aligned) */
#ifdef CM4_UBIN_LENGTH
        {BL_BASE, BL_LENGTH + CM4_LENGTH + CM4_UBIN_LENGTH},
#else
        {BL_BASE, BL_LENGTH + CM4_LENGTH},
#endif
        /* virtual memory */
        {VRAM_BASE, VRAM_LENGTH}
    };

    region_number = (hal_cache_region_t) (sizeof(region_cfg_tbl) / sizeof(region_cfg_tbl[0]));

    hal_cache_init();
    hal_cache_set_size(HAL_CACHE_SIZE_32KB);
    for (region = HAL_CACHE_REGION_0; region < region_number; region++) {
        hal_cache_region_config(region, &region_cfg_tbl[region]);
        hal_cache_region_enable(region);
    }
    for ( ; region < HAL_CACHE_REGION_MAX; region++) {
        hal_cache_region_disable(region);
    }
    hal_cache_enable();
}


static void ble_at_cmd_trix_space(char *cmd_buf)
{
    char *token = NULL;

    // remove '\n';
    token = strrchr(cmd_buf, '\n');
    while (token != NULL) {
        *token = '\0';
        token = strrchr(cmd_buf, '\n');
    }

    // remove '\r';
    token = strrchr(cmd_buf, '\r');
    while (token != NULL) {
        *token = '\0';
        token = strrchr(cmd_buf, '\r');
    }

}

static void ble_at_cmd_processing(void)
{
    printf("[hr]ble_at_cmd_processing");
    do {
        uint32_t    read_len;

        read_len = hal_uart_get_available_receive_bytes(g_atci_bt_port);
        BT_LOGI("hr", "[BT_CMD]read_len = %d\r\n", read_len);

        if (read_len > 0) {
            hal_uart_receive_dma(g_atci_bt_port, g_bt_cmd_rx_vfifo, read_len);

            ble_at_cmd_trix_space((char *)g_bt_cmd_rx_vfifo);
            read_len = strlen((char *)g_bt_cmd_rx_vfifo);
            BT_LOGI("hr", "[BT_CMD] command len: %d, CMD: %s", read_len, g_bt_cmd_rx_vfifo);
            bt_hr_io_callback(g_bt_cmd_rx_vfifo);
        } else {

        }
    } while (0);

}

static void ble_cmd_task(void *param)
{
    printf("ble_cmd_stask:!!!");
    while (1) {
        xSemaphoreTake(g_bt_io_semaphore, portMAX_DELAY);
        ble_at_cmd_processing();
    }
}

static void ble_io_uart_irq(hal_uart_callback_event_t event, void *parameter)
{

    BaseType_t  x_higher_priority_task_woken = pdFALSE;

    //hal_uart_send_polling(HAL_UART_0, "ble_io_uart_irq\r\n", sizeof("ble_io_uart_irq\r\n"));
    //hal_uart_send_dma(HAL_UART_0, "ble_io_uart_irq\r\n", sizeof("ble_io_uart_irq\r\n"));
    BT_LOGI("HR", "event = %d, semaphore_HD = 0x%x", event, g_bt_io_semaphore);
    if (HAL_UART_EVENT_READY_TO_READ == event) {
        xSemaphoreGiveFromISR(g_bt_io_semaphore, &x_higher_priority_task_woken);
        portYIELD_FROM_ISR( x_higher_priority_task_woken );
    }
}

static hal_uart_status_t ble_io_uart_init(hal_uart_port_t port)
{
    hal_uart_status_t ret;
    /* Configure UART PORT */
    hal_uart_config_t uart_config = {
        .baudrate = HAL_UART_BAUDRATE_115200,
        .word_length = HAL_UART_WORD_LENGTH_8,
        .stop_bit = HAL_UART_STOP_BIT_1,
        .parity = HAL_UART_PARITY_NONE
    };

    g_atci_bt_port = port;



    ret = hal_uart_init(port, &uart_config);
    BT_LOGI("HR", "UART INIT!!!: Ret = %d", ret);

    if (HAL_UART_STATUS_OK == ret) {

        hal_uart_dma_config_t   dma_config = {
            .send_vfifo_buffer              = g_bt_cmd_tx_vfifo,
            .send_vfifo_buffer_size         = VFIFO_SIZE_TX_BT,
            .send_vfifo_threshold_size      = 128,
            .receive_vfifo_buffer           = g_bt_cmd_rx_vfifo,
            .receive_vfifo_buffer_size      = VFIFO_SIZE_RX_BT,
            .receive_vfifo_threshold_size   = 128,
            .receive_vfifo_alert_size       = VFIFO_ALERT_LENGTH_BT
        };
        printf("[test]: start crate semaphone");
        g_bt_io_semaphore = xSemaphoreCreateBinary();
        ret = hal_uart_set_dma(port, &dma_config);

        //printf("[test]: set_dma = %x", ret);
        //  hal_uart_send_polling(HAL_UART_0, "set_dma", sizeof("set_dma"));
        BT_LOGI("HR", "SEND_DMA: Ret = %d", ret);

        ret = hal_uart_register_callback(port, ble_io_uart_irq, NULL);

        BT_LOGI("HR", "SEND_DMA: Ret = %d", ret);

    }
    return ret;
}


uint32_t caculate_mpu_region_size( uint32_t region_size )
{
    uint32_t count;

    if (region_size < 32) {
        return 0;
    }
    for (count = 0; ((region_size  & 0x80000000) == 0); count++, region_size  <<= 1);
    return 30 - count;
}



/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
    /* SystemClock Config */
    SystemClock_Config();

    SystemCoreClockUpdate();
    /* init UART earlier */
    log_uart_init(HAL_UART_2);

    /* Configure the hardware ready to run the test. */
    prvSetupHardware();

    hal_sleep_manager_init();
    hal_sleep_manager_lock_sleep(sleep_manager_handle);
    clock_dump_info();

    nvdm_init();

    log_init(NULL, NULL, NULL);


#if defined(__GNUC__)
    printf("Version:%s\n", "__GNUC__");
#endif
#if defined(__ARMCC_VERSION)
    printf("Version:%s\n", "__ARMCC_VERSION");
#endif
#if defined(__ICCARM__)
    printf("Version:%s\n", "__ARMCC_VERSION");
#endif

    ble_io_uart_init(HAL_UART_1);
    printf("Create cmd task !!!!");
    xTaskCreate(ble_cmd_task, "ble_cmd_task", 360, NULL, 3, NULL);


    ble_init();

    heart_rate_task_init();

    /* Start the scheduler. */
    vTaskStartScheduler();

    /* If all is well, the scheduler will now be running, and the following line
    will never be reached.  If the following line does execute, then there was
    insufficient FreeRTOS heap memory available for the idle and/or timer tasks
    to be created.  See the memory management section on the FreeRTOS web site
    for more details. */
    for ( ;; );
}

static void prvSetupHardware( void )
{
    /* system HW init */
    cache_init();
    /* peripherals init */
    hal_flash_init();

    hal_nvic_init();

    bsp_ept_gpio_setting_init();
}

static void SystemClock_Config(void)
{
    hal_clock_init();

    hal_display_lcd_set_clock(HAL_DISPLAY_LCD_INTERFACE_CLOCK_124MHZ);

    hal_dvfs_init();
    hal_dvfs_target_cpu_frequency(target_freq, HAL_DVFS_FREQ_RELATION_H);
}

