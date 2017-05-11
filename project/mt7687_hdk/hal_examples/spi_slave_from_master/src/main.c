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

/* hal includes */
#include "hal.h"
#include "system_mt7687.h"
#include "top.h"
#include "memory_attribute.h"
#include "mt7687_cm4_hw_memmap.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define SPI_TEST_CONTROL_SIZE  4
#define SPI_TEST_STATUS_SIZE   4
#define SPI_TEST_DATA_SIZE     (20 * 1024)
#define SPI_TEST_TX_DATA_PATTERN  0x01234567
#define SPI_TEST_RX_DATA_PATTERN  0x89abcdef
#define SPI_TEST_MASTER_SLAVE  HAL_SPI_MASTER_SLAVE_0
#define SPI_TEST_SLAVER        HAL_SPI_SLAVE_0
#define SPIS_PIN_NUMBER_CS     HAL_GPIO_32
#define SPIS_PIN_FUNC_CS       HAL_GPIO_32_SPI_CS_0_S_CM4
#define SPIS_PIN_NUMBER_CLK    HAL_GPIO_31
#define SPIS_PIN_FUNC_CLK      HAL_GPIO_31_SPI_SCK_S_CM4
#define SPIS_PIN_NUMBER_MOSI   HAL_GPIO_29
#define SPIS_PIN_FUNC_MOSI     HAL_GPIO_29_SPI_MOSI_S_CM4
#define SPIS_PIN_NUMBER_MISO   HAL_GPIO_30
#define SPIS_PIN_FUNC_MISO     HAL_GPIO_30_SPI_MISO_S_CM4
#define SPI_TEST_READ_DATA_BEGIN     0x0A
#define SPI_TEST_READ_DATA_END       0xA0
#define SPI_TEST_WRITE_DATA_BEGIN    0x55
#define SPI_TEST_WRITE_DATA_END      0xAA
#define SPI_TEST_ACK_FLAG            0x5A

/* Private variables ---------------------------------------------------------*/
static volatile bool transfer_data_finished = false;
/* 1.For this example project, we define slaver_data_buffer layout as below:
 *   The frist word is used to store command request from SPI master.
 *     - SPI_TEST_READ_DATA_BEGIN: when SPI master want to read data from SPI slave,
 *       it send this command to SPI slave to prepare the data.
 *     - SPI_TEST_READ_DATA_END: when SPI master has read all needed data,
 *       it send this command to notice the SPI slave.
 *     - SPI_TEST_WRITE_DATA_BEGIN: when SPI master want write data to SPI slave,
 *       it send this command to SPI slave to prepare the buffer.
 *     - SPI_TEST_WRITE_DATA_END: when SPI master has sent out of data,
 *       it send this command to notice the SPI slave.
 *   The second word is used to store ACK to respond to SPI master's command request.
 *     - define SPI_TEST_ACK_FLAG as the ACK flag.
 *   The rest space is used to store actual data transfer between SPI master and SPI slave.
 * 2.The buffer defined here is fix at specific address according declare both in linkscript
 *   and here.
 * 3.Define of the buffer structure above is just an example for demonstrate,
 *   User should implement it's own buffer structure define based on it's requirement.
 * 4.The buffer defined here should be located at non-cacheable area.
 */
#ifndef __ICCARM__
__attribute__((__section__(".spi_section"))) static uint8_t slaver_data_buffer[SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE] = {0};
#else
_Pragma("location=\".spi_section\"") _Pragma("data_alignment=512") static uint8_t slaver_data_buffer[SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE] = {0};
#endif
static hal_cache_region_config_t region_cfg_tbl[] = {
    /* cache_region_address, cache_region_size */
    { CM4_EXEC_IN_PLACE_BASE, 0x1000000}
    /* add cache regions below if you have any */
};

/* Private functions ---------------------------------------------------------*/

#ifdef __GNUC__
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the HAL_UART_0 one at a time */
    hal_uart_put_char(HAL_UART_0, ch);
    return ch;
}

/**
*@brief  Enable cache.
*@param[in] cache_size: this parameter indicates the size of cache.
*@return Return the state of cache enable succeed of fail.
*/
static int32_t cache_enable(hal_cache_size_t cache_size)
{
    uint8_t region, region_number;

    region_number = sizeof(region_cfg_tbl) / sizeof(region_cfg_tbl[0]);
    if (region_number > HAL_CACHE_REGION_MAX) {
        return -1;
    }
    /* If cache is enabled, flush and invalidate cache */
    hal_cache_init();
    hal_cache_set_size(cache_size);
    for (region = 0; region < region_number; region++) {
        hal_cache_region_config((hal_cache_region_t)region, &region_cfg_tbl[region]);
        hal_cache_region_enable((hal_cache_region_t)region);
    }
    for (; region < HAL_CACHE_REGION_MAX; region++) {
        hal_cache_region_disable((hal_cache_region_t)region);
    }
    hal_cache_enable();
    return 0;
}

/**
*@brief Set pinmux to UART and initialize UART hardware initialization for logging.
*@param None.
*@return None.
*/
static void plain_log_uart_init(void)
{
    hal_uart_config_t uart_config;
    /* Set Pinmux to UART */
    hal_pinmux_set_function(HAL_GPIO_0, HAL_GPIO_0_UART1_RTS_CM4);
    hal_pinmux_set_function(HAL_GPIO_1, HAL_GPIO_1_UART1_CTS_CM4);
    hal_pinmux_set_function(HAL_GPIO_2, HAL_GPIO_2_UART1_RX_CM4);
    hal_pinmux_set_function(HAL_GPIO_3, HAL_GPIO_3_UART1_TX_CM4);

    uart_config.baudrate = HAL_UART_BAUDRATE_115200;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8;
    uart_config.stop_bit = HAL_UART_STOP_BIT_1;
    uart_config.parity = HAL_UART_PARITY_NONE;
    hal_uart_init(HAL_UART_0, &uart_config);
}

/**
*@brief Configure and initialize the systerm clock.
*@param None.
*@return None.
*/
static void SystemClock_Config(void)
{
    top_xtal_init();
}

/**
*@brief  Enable MCU clock and initialize the periperal driver in this function. In this example, we initialize UART drivers.
*@param None.
*@return None.
*/
static void prvSetupHardware(void)
{
    /* System HW initialization */
    if (cache_enable(HAL_CACHE_SIZE_32KB) < 0) {
        printf("cache enable failed");
        return;
    }

    /* Enable MCU clock to 192MHz */
    cmnCpuClkConfigureTo192M();

    /* Peripherals initialization */
    plain_log_uart_init();
}

/**
*@brief SPI slave set the second word to notice SPI master.
*@param None.
*@return None.
*/
static void spis_notice_spim(void)
{
    slaver_data_buffer[4] = SPI_TEST_ACK_FLAG;
}

/**
*@brief user's callback function registered to SPI slave driver.
*@param[in] user_data: user's data used in this callback function.
*@return None.
*/
static void spis_user_callback(void *user_data)
{
    uint32_t *ptr, i;

    switch (slaver_data_buffer[0]) {
        case SPI_TEST_WRITE_DATA_BEGIN:
            /* master want to write data to us, so we prepare data buffer frist. */
            ptr = (uint32_t *)(&slaver_data_buffer[SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE]);
            memset(ptr, 0, SPI_TEST_DATA_SIZE);
            /* then notice SPI master. */
            spis_notice_spim();
            break;
        case SPI_TEST_WRITE_DATA_END:
            /* master has written data completely, let's check the data. */
            ptr = (uint32_t *)(&slaver_data_buffer[SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE]);
            for (i = 0; i < SPI_TEST_DATA_SIZE / 4; i++) {
                if (ptr[i] != SPI_TEST_TX_DATA_PATTERN) {
                    log_hal_error("Test fail: dst_data_buffer[%d]=%x\r\n", i, ptr[i]);
                    return;
                }
            }
            /* then notice SPI master. */
            spis_notice_spim();
            break;
        case SPI_TEST_READ_DATA_BEGIN:
            /* master want to read data, so we prepare data frist. */
            ptr = (uint32_t *)(&slaver_data_buffer[SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE]);
            for (i = 0; i < SPI_TEST_DATA_SIZE / 4; i++) {
                ptr[i] = SPI_TEST_RX_DATA_PATTERN;
            }
            /* then notice SPI master. */
            spis_notice_spim();
            break;
        case SPI_TEST_READ_DATA_END:
            /* master has read data completely, so we end up the example. */
            transfer_data_finished = true;
            break;
        default:
            log_hal_error("spis_user_callback detect error command\r\n");
    }
}

/**
*@brief  Example of SPI master transfer data with SPI slave.
*@param None.
*@return None.
*/
static void spi_slave_transfer_data_two_boards_example(void)
{
    hal_spi_slave_config_t  spis_configure;

    printf("---spi_slave_transfer_data_two_boards_example begin---\r\n\r\n");

    /*Step1: Init GPIO, set SPIS pinmux(if EPT tool hasn't been used to configure the related pinmux).*/
    hal_gpio_init(SPIS_PIN_NUMBER_CS);
    hal_gpio_init(SPIS_PIN_NUMBER_CLK);
    hal_gpio_init(SPIS_PIN_NUMBER_MOSI);
    hal_gpio_init(SPIS_PIN_NUMBER_MISO);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_CS, SPIS_PIN_FUNC_CS);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_CLK, SPIS_PIN_FUNC_CLK);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_MOSI, SPIS_PIN_FUNC_MOSI);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_MISO, SPIS_PIN_FUNC_MISO);

    /* Step2: Init SPI slaver. */
    spis_configure.phase = HAL_SPI_SLAVE_CLOCK_PHASE0;
    spis_configure.polarity = HAL_SPI_SLAVE_CLOCK_POLARITY0;
    if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_init(SPI_TEST_SLAVER, &spis_configure)) {
        log_hal_error("hal_spi_slave_init fail\r\n");
        return;
    } else {
        printf("hal_spi_slave_init pass\r\n");
    }

    /* Step3: Register callback to SPI slaver. */
    if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_register_callback(SPI_TEST_SLAVER, spis_user_callback, NULL)) {
        log_hal_error("hal_spi_slave_register_callback fail\r\n");
        return;
    } else {
        printf("hal_spi_slave_register_callback pass\r\n");
    }

    /* Step4: waiting SPI master to transfer data with us. */
    memset(slaver_data_buffer, 0, SPI_TEST_DATA_SIZE + SPI_TEST_CONTROL_SIZE + SPI_TEST_STATUS_SIZE);
    transfer_data_finished = false;
    printf("SPI slaver is ready and waiting for master to write and read\r\n");
    while (transfer_data_finished == false);

    /* Step5: Deinit spi master and related GPIO. */
    hal_gpio_deinit(SPIS_PIN_NUMBER_CS);
    hal_gpio_deinit(SPIS_PIN_NUMBER_CLK);
    hal_gpio_deinit(SPIS_PIN_NUMBER_MOSI);
    hal_gpio_deinit(SPIS_PIN_NUMBER_MISO);

    printf("\r\n---spi_slave_transfer_data_two_boards_example end---\r\n");
}

int main(void)
{
    /* SystemClock Config */
    SystemClock_Config();

    /* Configure the hardware ready to run the test. */
    prvSetupHardware();

    /* Enable I,F bits */
    __enable_irq();
    __enable_fault_irq();

    /* Add your application code here */
    printf("\r\n\r\n");/* The output UART used by log_hal_info is set by log_uart_init() */
    printf("welcome to main()\r\n");
    printf("pls add your own code from here\r\n");
    printf("\r\n\r\n");

    spi_slave_transfer_data_two_boards_example();

    for (;;);
}

