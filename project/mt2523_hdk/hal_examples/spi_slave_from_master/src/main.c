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
#define SPI_TEST_DATA_SIZE     1024
#define SPI_TEST_DATA_PATTERN  0xa5
#define SPI_TEST_SLAVER        HAL_SPI_SLAVE_0
#define SPIS_PIN_NUMBER_CS     HAL_GPIO_25
#define SPIS_PIN_FUNC_CS       HAL_GPIO_25_SLV_SPI0_CS
#define SPIS_PIN_NUMBER_SLK    HAL_GPIO_26
#define SPIS_PIN_FUNC_SLK      HAL_GPIO_26_SLV_SPI0_SCK
#define SPIS_PIN_NUMBER_MOSI   HAL_GPIO_27
#define SPIS_PIN_FUNC_MOSI     HAL_GPIO_27_SLV_SPI0_MOSI
#define SPIS_PIN_NUMBER_MISO   HAL_GPIO_28
#define SPIS_PIN_FUNC_MISO     HAL_GPIO_28_SLV_SPI0_MISO
#define SPI_TX_BUFFER_ADDR     0x04000000

/* Private variables ---------------------------------------------------------*/
static volatile bool spi_slave_poweron_flag;
static volatile bool spi_slave_cfg_write_flag;
static volatile bool spi_slave_write_data_flag;
static volatile bool spi_slave_timeout_flag;
ATTR_ZIDATA_IN_NONCACHED_RAM_4BYTE_ALIGN static uint8_t wr_buffer[SPI_TEST_DATA_SIZE] = {0};

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
*@brief     In this function user define it's callback.
*@param[in] status:    offers the SPI slave FSM status and interupt status of SPI slave controller.
*@param[in] user_data: pointer to user's data that user will use in this callback.
*@return    None.
*/
static void spi_slave_callback(hal_spi_slave_transaction_status_t status, void *user_data)
{
    uint16_t slave_status;
    const uint8_t *data = (uint8_t *)SPI_TX_BUFFER_ADDR;
    hal_spi_slave_config_t spi_configure;

    if (HAL_SPI_SLAVE_FSM_SUCCESS_OPERATION == (status.fsm_status)) {
        /* Normal fsm behavior */
        slave_status = status.interrupt_status;
        switch (slave_status) {
            case SPISLV_IRQ_POWERON_IRQ_MASK:
                /* PDN is turned on, initializes spi slave controller here */
                spi_configure.bit_order = HAL_SPI_SLAVE_LSB_FIRST;
                spi_configure.phase = HAL_SPI_SLAVE_CLOCK_PHASE0;
                spi_configure.polarity = HAL_SPI_SLAVE_CLOCK_POLARITY0;
                spi_configure.timeout_threshold = 0xFFFFFFFF;
                hal_spi_slave_init(SPI_TEST_SLAVER, &spi_configure);
                spi_slave_poweron_flag = true;
                printf("---Test-POWERON----\n");
                break;
            case SPISLV_IRQ_POWEROFF_IRQ_MASK:
                hal_spi_slave_deinit(SPI_TEST_SLAVER);
                spi_slave_poweron_flag = false;
                printf("---Test-POWEROFF----\n");
                break;
            case SPISLV_IRQ_CRD_FINISH_IRQ_MASK:
                /* Call SPI slave send function here to set data address and size*/
                hal_spi_slave_send(SPI_TEST_SLAVER, data, SPI_TEST_DATA_SIZE);
                printf("---Test-CRD_FINISH----\n");
                break;
            case SPISLV_IRQ_CWR_FINISH_IRQ_MASK:
                /* call spi slave read function here to set data address and size*/
                hal_spi_slave_receive(SPI_TEST_SLAVER, wr_buffer, SPI_TEST_DATA_SIZE);
                spi_slave_cfg_write_flag = true;
                printf("---Test-CWR_FINISH----\n");
                break;
            case SPISLV_IRQ_RD_FINISH_IRQ_MASK:
                printf("---Test-RD_FINISH----\n");
                break;
            case SPISLV_IRQ_WR_FINISH_IRQ_MASK:
                /* User can now get the data from the address set before */
                spi_slave_write_data_flag = true;
                printf("---Test-WR_FINISH----\n");
                break;
            case SPISLV_IRQ_RD_ERR_IRQ_MASK:
                /* Data buffer may be reserved for retransmit depending on user's usage */
                printf("---Test-RD_ERR----\n");
                break;
            case SPISLV_IRQ_WR_ERR_IRQ_MASK:
                /* Data in the address set before isn't correct, user may abandon them */
                printf("---Test-WR_ERR----\n");
                break;
            case SPISLV_IRQ_TIMEOUT_ERR_IRQ_MASK:
                /* timeout happen */
                spi_slave_timeout_flag = true;
                break;
            default:
                break;
        }
    } else if (HAL_SPI_SLAVE_FSM_INVALID_OPERATION != (status.fsm_status)) {
        switch (status.fsm_status) {
            case HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CR:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CR, fsm is poweroff\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CW:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_PWROFF_AFTER_CW, fsm is poweroff\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CR:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CR, fsm is CR\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CR_AFTER_CW:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_CR_AFTER_CW, fsm is CR\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CW:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_CONTINOUS_CW, fsm is CW\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_CW_AFTER_CR:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_CW_AFTER_CR, fsm is CW\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_WRITE_AFTER_CR:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_WRITE_AFTER_CR, fsm is poweron\n");
                break;
            case HAL_SPI_SLAVE_FSM_ERROR_READ_AFTER_CW:
                log_hal_error("HAL_SPI_SLAVE_FSM_ERROR_READ_AFTER_CW, fsm is poweron\n");
                break;
            default:
                break;
        }
    } else {
        log_hal_error("HAL_SPI_SLAVE_FSM_INVALID_OPERATION, fsm is poweron\n");
    }
}

/**
*@brief  Example of spi slaver receive datas. In this function, SPI slaves receive datas from spi master.
*@param  None.
*@return None.
*/
static void spi_slave_receive_data_two_boards_example(void)
{
    uint32_t i = 0;

    printf("---spi_slave_receive_data_two_boards_example begin---\r\n\r\n");

    /* Step1: GPIO configuaration for SPI slaver. */
    hal_gpio_init(SPIS_PIN_NUMBER_CS);
    hal_gpio_init(SPIS_PIN_NUMBER_SLK);
    hal_gpio_init(SPIS_PIN_NUMBER_MOSI);
    hal_gpio_init(SPIS_PIN_NUMBER_MISO);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_CS, SPIS_PIN_FUNC_CS);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_SLK, SPIS_PIN_FUNC_SLK);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_MOSI, SPIS_PIN_FUNC_MOSI);
    hal_pinmux_set_function(SPIS_PIN_NUMBER_MISO, SPIS_PIN_FUNC_MISO);

    /* Step2: Reset SPI slaver status. */
    spi_slave_poweron_flag = false;
    spi_slave_cfg_write_flag = false;
    spi_slave_write_data_flag = false;
    spi_slave_timeout_flag = false;

    /* Step3: Register callback function to SPI slaver driver. */
    if (HAL_SPI_SLAVE_STATUS_OK != hal_spi_slave_register_callback(SPI_TEST_SLAVER, spi_slave_callback, NULL)) {
        log_hal_error("SPI slave register callback failed\r\n");
    }

    /* Step4: wait SPI master to send power on command. */
    printf("Slave is waiting for power_on command\r\n");
    while (spi_slave_poweron_flag == false) {
        if (spi_slave_timeout_flag == true) {
            log_hal_error("Slave detect timeout error\r\n");
            return;
        }
    }
    printf("Slave received power_on happened\r\n\r\n");

    /* Step5: wait SPI master to send config write command. */
    while (spi_slave_cfg_write_flag == false) {
        if (spi_slave_timeout_flag == true) {
            log_hal_error("Slave detect timeout error\r\n");
            return;
        } else {
            printf("Slave is waiting for cfg_write command\r\n");
        }
    }
    printf("Slave received cfg_write command\r\n\r\n");

    /* Step6: wait SPI master to send write data command. */
    while (spi_slave_write_data_flag == false) {
        if (spi_slave_timeout_flag == true) {
            log_hal_error("Slave detect timeout error\r\n");
            return;
        } else {
            printf("Slave is waiting for write data command\r\n");
        }
    }
    printf("Slave received write data command\r\n\r\n");

    /* Step7: wait SPI master to send power off command. */
    while (spi_slave_poweron_flag == true) {
        if (spi_slave_timeout_flag == true) {
            log_hal_error("Slave detect timeout error\r\n");
            return;
        } else {
            printf("Slave is waiting for power_off command\r\n");
        }
    }
    printf("Slave received power_off command\r\n\r\n");

    /* Step8: check whether the data value is correct. */
    printf("Start to check data integrity\r\n");
    for (i = 0; i < SPI_TEST_DATA_SIZE; i++) {
        if (wr_buffer[i] != SPI_TEST_DATA_PATTERN) {
            log_hal_error("Data check failed: address: %x, value: %x\r\n", i, wr_buffer[i]);
            return;
        }
    }
    printf("Data check pass\r\n\r\n");

    /* Step9: De-initialize GPIO after data transaction end. */
    hal_gpio_deinit(SPIS_PIN_NUMBER_CS);
    hal_gpio_deinit(SPIS_PIN_NUMBER_SLK);
    hal_gpio_deinit(SPIS_PIN_NUMBER_MOSI);
    hal_gpio_deinit(SPIS_PIN_NUMBER_MISO);

    printf("---spi_slave_receive_data_two_boards finished---\r\n");
}

int main(void)
{
    /* Configure System clock. */
    SystemClock_Config();

    SystemCoreClockUpdate();

    /* Configure the hardware ready to run the test Configure the hardware. */
    prvSetupHardware();

    /* Enable I,F bits. */
    __enable_irq();
    __enable_fault_irq();

    /* Add your application code here. */
    printf("\r\n\r\n");

    /* The output UART used by printf is set by log_uart_init(). */
    printf("welcome to main()\r\n");
    printf("\r\n\r\n");

    spi_slave_receive_data_two_boards_example();

    while (1);
}

