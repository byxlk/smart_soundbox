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
#include "memory_attribute.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define SPIS_CFG_WR_CMD       0x04
#define SPIS_WR_CMD           0x06
#define SPIS_STATUS_CMD       0x0a
#define SPIS_POWERON_CMD      0x0e
#define SPIS_POWEROFF_CMD     0x0c
#define SPI_TEST_DATA_SIZE    1024
#define SPI_TEST_DATA_PATTERN 0xa5
#define SPIS_ADDRESS_ID       0x55aa0000
#define SPIS_CFG_LENGTH       (SPI_TEST_DATA_SIZE - 1)
#define SPI_TEST_FREQUENCY    6000000
#define SPI_TEST_MASTER       HAL_SPI_MASTER_3
#define SPI_TEST_MASTER_SLAVE HAL_SPI_MASTER_SLAVE_0
#define SPIM_PIN_NUMBER_CS    HAL_GPIO_25
#define SPIM_PIN_FUNC_CS      HAL_GPIO_25_MA_SPI3_A_CS
#define SPIM_PIN_NUMBER_CLK   HAL_GPIO_26
#define SPIM_PIN_FUNC_CLK     HAL_GPIO_26_MA_SPI3_A_SCK
#define SPIM_PIN_NUMBER_MOSI  HAL_GPIO_27
#define SPIM_PIN_FUNC_MOSI    HAL_GPIO_27_MA_SPI3_A_MOSI
#define SPIM_PIN_NUMBER_MISO  HAL_GPIO_28
#define SPIM_PIN_FUNC_MISO    HAL_GPIO_28_MA_SPI3_A_MISO

/* Private variables ---------------------------------------------------------*/
static volatile bool g_spi_transaction_finish;
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t wr_buffer[SPI_TEST_DATA_SIZE + 1];

/* Private functions ---------------------------------------------------------*/

#ifdef __GNUC__
int __io_putchar(int ch)
#else
int fputc(int ch, FILE *f)
#endif
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the HAL_UART_2 one at a time */
    hal_uart_put_char(HAL_UART_2, ch);
    return ch;
}

/**
*@brief  Configure and initialize UART hardware initialization for logging.
*@param  None.
*@return None.
*/
static void plain_log_uart_init(void)
{
    hal_uart_config_t uart_config;


    /* gpio config for uart2 */
    hal_gpio_init(HAL_GPIO_0);
    hal_gpio_init(HAL_GPIO_1);
    hal_pinmux_set_function(HAL_GPIO_0, HAL_GPIO_0_U2RXD);
    hal_pinmux_set_function(HAL_GPIO_1, HAL_GPIO_1_U2TXD);


    /* COM port settings */
    uart_config.baudrate = HAL_UART_BAUDRATE_115200;
    uart_config.word_length = HAL_UART_WORD_LENGTH_8;
    uart_config.stop_bit = HAL_UART_STOP_BIT_1;
    uart_config.parity = HAL_UART_PARITY_NONE;
    hal_uart_init(HAL_UART_2, &uart_config);
}

/**
*@brief  Configure and initialize the systerm clock. In this example, we invoke hal_clock_init to initialize clock driver and clock gates.
*@param  None.
*@return None.
*/
static void SystemClock_Config(void)
{
    hal_clock_init();
}

/**
*@brief  Initialize the peripheral driver in this function. In this example, we initialize UART, Flash, and NVIC drivers.
*@param  None.
*@return None.
*/
static void prvSetupHardware(void)
{
    /* Peripherals initialization */
    plain_log_uart_init();
    hal_flash_init();
    hal_nvic_init();
}

/**
*@brief     In this function we query status of spi slaver.
*@param[in] status: Pointer to the result of spi slaver's status.
*@return    None.
*/
static void spi_query_slave_status(uint8_t *status)
{
    uint8_t status_cmd = SPIS_STATUS_CMD;
    uint8_t status_receive[2];
    hal_spi_master_send_and_receive_config_t spi_send_and_receive_config;

    /* Note:
     * The value of receive_length is the valid number of bytes received plus the number of bytes to send.
     * For example, here the valid number of bytes received is 1 byte,
     * and the number of bytes to send also is 1 byte, so the receive_length is 2.
     */
    status_receive[1] = 0;
    spi_send_and_receive_config.receive_length = 2;
    spi_send_and_receive_config.send_length = 1;
    spi_send_and_receive_config.send_data = &status_cmd;
    spi_send_and_receive_config.receive_buffer = status_receive;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_send_and_receive_polling(SPI_TEST_MASTER, &spi_send_and_receive_config)) {
        log_hal_error("SPI master query status of slaver failed\r\n");
        return;
    }

    printf("Status receive: 0x%x\r\n", status_receive[1]);
    *status = status_receive[1];
}

/**
*@brief     In this function we notice spi driver owner that spi transfer has been completed.
*@param[in] event: event of spi driver.
*@param[in] user_data: pointer to the data that registered to spi driver.
*@return    None.
*/
static void spi_master_dma_callback(hal_spi_master_callback_event_t event, void *user_data)
{
    printf(" spi_cb_entered, event = %d\r\n", event);
    g_spi_transaction_finish = true;
}

/**
*@brief  Example of spi send datas. In this function, SPI master send various commond to slave.
*@param  None.
*@return None.
*/
static void spi_master_send_data_two_boards_example(void)
{
    uint32_t i = 0;
    uint8_t status_receive;
    uint8_t poweron_cmd;
    uint8_t poweroff_cmd;
    uint8_t cfg_wr_cmd[9];
    hal_spi_master_config_t spi_config;
    hal_spi_master_advanced_config_t advanced_config;

    printf("---spi_master_send_data_two_boards_example begins---\r\n\r\n");

    /* Step1: GPIO configuration for SPI master */
    hal_gpio_init(SPIM_PIN_NUMBER_CS);
    hal_gpio_init(SPIM_PIN_NUMBER_CLK);
    hal_gpio_init(SPIM_PIN_NUMBER_MOSI);
    hal_gpio_init(SPIM_PIN_NUMBER_MISO);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_CS, SPIM_PIN_FUNC_CS);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_CLK, SPIM_PIN_FUNC_CLK);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_MOSI, SPIM_PIN_FUNC_MOSI);
    hal_pinmux_set_function(SPIM_PIN_NUMBER_MISO, SPIM_PIN_FUNC_MISO);

    /* Step2: Initializes SPI master */
    spi_config.bit_order = HAL_SPI_MASTER_LSB_FIRST;
    spi_config.slave_port = SPI_TEST_MASTER_SLAVE;
    spi_config.clock_frequency = SPI_TEST_FREQUENCY;
    spi_config.phase = HAL_SPI_MASTER_CLOCK_PHASE0;
    spi_config.polarity = HAL_SPI_MASTER_CLOCK_POLARITY0;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_init(SPI_TEST_MASTER, &spi_config)) {
        log_hal_error("SPI master init failed\r\n");
        return;
    }
    advanced_config.byte_order = HAL_SPI_MASTER_LITTLE_ENDIAN;
    advanced_config.chip_polarity = HAL_SPI_MASTER_CHIP_SELECT_LOW;
    advanced_config.sample_select = HAL_SPI_MASTER_SAMPLE_POSITIVE;
    /* User may need to try different get_tick settings for timing tolerance when high SCK frequency is used */
    advanced_config.get_tick = HAL_SPI_MASTER_GET_TICK_DELAY1;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_set_advanced_config(SPI_TEST_MASTER, &advanced_config)) {
        log_hal_error("SPI master advanced configuration failed\r\n");
        return;
    }
    printf("SPI Master configuration successfully\r\n\r\n");

    /* Step3: Master starts to send power_on command to slave */
    printf("SPI Master starts to try send power_on command to slave in a loop\r\n");
    do {
        hal_gpt_delay_ms(500);
        poweron_cmd = SPIS_POWERON_CMD;
        if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_send_polling(SPI_TEST_MASTER, &poweron_cmd, 1)) {
            log_hal_error("SPI master sends power_on command fail\r\n");
            return;
        } else {
            printf("SPI master sends power_on command ok\r\n");
        }
        hal_gpt_delay_ms(10);
        spi_query_slave_status(&status_receive);
    } while ((status_receive & (uint8_t)SPISLV_STATUS_SLV_ON_MASK) != (uint8_t)SPISLV_STATUS_SLV_ON_MASK);
    printf("SPI slaver successfully respond to power_on command\r\n\r\n");

    /* Step4: Master starts to send cfg_wr_cmd command to slave */
    printf("SPI master starts to send cfg_wr_cmd command to slave\r\n");
    cfg_wr_cmd[0] = SPIS_CFG_WR_CMD;
    cfg_wr_cmd[1] = SPIS_ADDRESS_ID & 0xff;
    cfg_wr_cmd[2] = (SPIS_ADDRESS_ID >> 8) & 0xff;
    cfg_wr_cmd[3] = (SPIS_ADDRESS_ID >> 16) & 0xff;
    cfg_wr_cmd[4] = (SPIS_ADDRESS_ID >> 24) & 0xff;
    cfg_wr_cmd[5] = SPIS_CFG_LENGTH & 0xff;
    cfg_wr_cmd[6] = (SPIS_CFG_LENGTH >> 8) & 0xff;
    cfg_wr_cmd[7] = (SPIS_CFG_LENGTH >> 16) & 0xff;
    cfg_wr_cmd[8] = (SPIS_CFG_LENGTH >> 24) & 0xff;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_send_polling(SPI_TEST_MASTER, cfg_wr_cmd, 9)) {
        log_hal_error("SPI master send cfg_wr_cmd command failed\r\n");
        return;
    }
    do {
        spi_query_slave_status(&status_receive);
    } while ((status_receive & (uint8_t)SPISLV_STATUS_TXRX_FIFO_RDY_MASK) != (uint8_t)SPISLV_STATUS_TXRX_FIFO_RDY_MASK);
    printf("SPI master sends cfg_wr_cmd command successfully\r\n\r\n");

    /* Step5: Master starts to send wr_cmd command to slave */
    printf("SPI master starts to send wr_cmd command to slave using DMA mode\r\n");
    printf("Size to sent = %d\r\n", SPI_TEST_DATA_SIZE);
    g_spi_transaction_finish = false;
    hal_spi_master_register_callback(SPI_TEST_MASTER, spi_master_dma_callback, NULL);
    wr_buffer[0] = SPIS_WR_CMD;
    for (i = 1; i < (SPI_TEST_DATA_SIZE + 1); i++) {
        wr_buffer[i] = SPI_TEST_DATA_PATTERN;
    }
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_send_dma(SPI_TEST_MASTER, wr_buffer, (SPI_TEST_DATA_SIZE + 1))) {
        log_hal_error("\r\n SPI master send wr_cmd command failed\r\n");
    }
    while (g_spi_transaction_finish == 0);
    do {
        spi_query_slave_status(&status_receive);
    } while ((status_receive & (uint8_t)SPISLV_STATUS_RDWR_FINISH_MASK) != (uint8_t)SPISLV_STATUS_RDWR_FINISH_MASK);
    printf("SPI master sends wr_cmd command successfully\r\n\r\n");

    /* Step6: Master starts to send power_off command to slave and query whether it's successful. */
    printf("SPI master starts to send power_off command to slave\r\n");
    poweroff_cmd = SPIS_POWEROFF_CMD;
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_send_polling(SPI_TEST_MASTER, &poweroff_cmd, 1)) {
        log_hal_error("SPI master send power_off command failed\r\n");
        return;
    }
    do {
        spi_query_slave_status(&status_receive);
    } while ((status_receive & (uint8_t)SPISLV_STATUS_SLV_ON_MASK) == (uint8_t)SPISLV_STATUS_SLV_ON_MASK);
    printf("SPI master sends power_off command successfully\r\n\r\n");

    /* Step7: De-initialize SPI master and GPIO after data transaction end. */
    if (HAL_SPI_MASTER_STATUS_OK != hal_spi_master_deinit(SPI_TEST_MASTER)) {
        log_hal_error("SPI master deinit failed\r\n");
        return;
    }
    hal_gpio_deinit(SPIM_PIN_NUMBER_CS);
    hal_gpio_deinit(SPIM_PIN_NUMBER_CLK);
    hal_gpio_deinit(SPIM_PIN_NUMBER_MOSI);
    hal_gpio_deinit(SPIM_PIN_NUMBER_MISO);

    printf("---spi_master_send_data_two_boards_example ends---\r\n");
}

int main(void)
{
    /* Configure System clock. */
    SystemClock_Config();

    SystemCoreClockUpdate();

    /* Configure the hardware. */
    prvSetupHardware();

    /* Enable I,F bits. */
    __enable_irq();
    __enable_fault_irq();

    /* The output UART used by printf is configured by plain_log_uart_init(). */
    printf("\r\n\r\n");
    printf("welcome to main()\r\n");
    printf("\r\n\r\n");

    /* Add your application code here. */
    spi_master_send_data_two_boards_example();

    while (1);
}

