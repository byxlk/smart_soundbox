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

#include "mt25x3_hdk_backlight.h"

#include "hal_gpio.h"
#ifdef HAL_ISINK_MODULE_ENABLED
#include "hal_isink.h"
#endif
#ifdef HAL_DISPLAY_LCD_MODULE_ENABLED
#include "mt25x3_hdk_lcd.h"
extern lcm_config_para_t lcm_setting;
#endif
#ifdef HAL_DISPLAY_DSI_MODULE_ENABLED
#include "hal_display_dsi.h"
#endif

#include "hal_gpt.h"
#include "hal_log.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool isUpdate;
uint8_t prev_brightness;

void BSP_Backlight_init(void)
{
#ifdef HAL_DISPLAY_LCD_MODULE_ENABLED
#ifdef MTK_EXTERNAL_PMIC
    if (BACKLIGHT_TYPE_ISINK == lcm_setting.backlight_type) {
        BSP_Backlight_init_external_isink();
    }
#else
#ifdef HAL_ISINK_MODULE_ENABLED
    if (BACKLIGHT_TYPE_ISINK == lcm_setting.backlight_type) {
        BSP_Backlight_init_isink(HAL_ISINK_CHANNEL_0, 1, HAL_ISINK_OUTPUT_CURRENT_24_MA);
    }
#endif
#endif

#ifdef HAL_DISPLAY_PWM_MODULE_ENABLED
    if (BACKLIGHT_TYPE_DISPLAY_PWM == lcm_setting.backlight_type) {
        BSP_Backlight_init_display_pwm();
    }
#endif

#ifdef HAL_DISPLAY_DSI_MODULE_ENABLED
    if (BACKLIGHT_TYPE_LCM_BRIGHTNESS == lcm_setting.backlight_type) {
        BSP_Backlight_init_lcm_brightness();
    }
#endif
#endif
}

void BSP_Backlight_deinit(void)
{
#ifdef HAL_DISPLAY_LCD_MODULE_ENABLED
#ifdef MTK_EXTERNAL_PMIC
    if (BACKLIGHT_TYPE_ISINK == lcm_setting.backlight_type) {
        BSP_Backlight_deinit_external_isink();
    }
#else
#ifdef HAL_ISINK_MODULE_ENABLED
    if (BACKLIGHT_TYPE_ISINK == lcm_setting.backlight_type) {
        hal_isink_deinit(HAL_ISINK_CHANNEL_0);
        hal_isink_deinit(HAL_ISINK_CHANNEL_1);
    }
#endif
#endif

#ifdef HAL_DISPLAY_PWM_MODULE_ENABLED
    if (BACKLIGHT_TYPE_DISPLAY_PWM == lcm_setting.backlight_type) {
        hal_display_pwm_deinit();
    }
#endif

#ifdef HAL_DISPLAY_DSI_MODULE_ENABLED
    if (BACKLIGHT_TYPE_LCM_BRIGHTNESS == lcm_setting.backlight_type) {
        BSP_Backlight_deinit_lcm_brightness();
    }
#endif
#endif
}

void BSP_Backlight_enable(bool enable)
{
    if (enable) {
        BSP_Backlight_init();
    } else {
        BSP_Backlight_deinit();
    }
}

#ifdef HAL_ISINK_MODULE_ENABLED

bool isDobuleEn = false;

void BSP_Backlight_init_isink(hal_isink_channel_t channel, bool isDouble, hal_isink_current_t current)
{
    hal_isink_init(channel);
    hal_isink_set_mode(channel, HAL_ISINK_MODE_REGISTER);
    hal_isink_set_clock_source(channel, HAL_ISINK_CLOCK_SOURCE_2MHZ);
    if (isDouble) {
        hal_isink_set_double_current(channel, current);
    } else {
        hal_isink_set_step_current(channel, current);
    }
    isDobuleEn = isDouble;
}

void BSP_Backlight_set_clock_source_isink(hal_isink_channel_t channel, hal_isink_clock_source_t source_clock)
{
    hal_isink_set_clock_source(channel, source_clock);
}

void BSP_Backlight_set_step_current(hal_isink_channel_t channel, hal_isink_current_t current)
{
    if (isDobuleEn) {
        BSP_Backlight_deinit();
        BSP_Backlight_init_isink(channel, false, current);
    } else {
        hal_isink_set_step_current(channel, current);
    }
}

void BSP_Backlight_set_double_current_isink(hal_isink_channel_t channel, hal_isink_current_t current)
{
    if (!isDobuleEn) {
        BSP_Backlight_deinit();
        BSP_Backlight_init_isink(channel, true, current);
    } else {
        hal_isink_set_double_current(channel, current);
    }
}
#endif

#ifdef HAL_DISPLAY_PWM_MODULE_ENABLED
void BSP_Backlight_init_display_pwm(void)
{
    hal_display_pwm_init(HAL_DISPLAY_PWM_CLOCK_26MHZ);
    hal_display_pwm_set_duty(80);
}

void BSP_Backlight_set_clock_source_display_pwm(hal_display_pwm_clock_t source_clock)
{
    hal_display_pwm_set_clock(source_clock);
}

void BSP_Backlight_set_duty_display_pwm(uint8_t percent)
{
    hal_display_pwm_set_duty(percent);
}

void BSP_Backlight_set_width_display_pwm(uint32_t width)
{
    hal_display_pwm_set_width(width);
}

#endif

void BSP_Backlight_init_lcm_brightness(void)
{
#ifdef HAL_DISPLAY_DSI_MODULE_ENABLED
    BSP_Backlight_set_step_lcm_brightness(255);
#endif
}

void BSP_Backlight_deinit_lcm_brightness(void)
{
#ifdef HAL_DISPLAY_DSI_MODULE_ENABLED
    BSP_Backlight_set_step_lcm_brightness(30);
#endif
}

void BSP_Backlight_set_step_lcm_brightness(uint8_t level)
{
#ifdef HAL_DISPLAY_DSI_MODULE_ENABLED
    uint32_t data_array[16];
    uint32_t gpt_start_count, gpt_current_count, gpt_duration_count;

    if(level == prev_brightness)
        return;

    prev_brightness = level;

    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &gpt_start_count);
    while(isUpdate) {
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, &gpt_current_count);
        hal_gpt_get_duration_count(gpt_start_count, gpt_current_count, &gpt_duration_count);
        if ((gpt_duration_count > 10000) && isUpdate) {
            log_hal_info("Previous frame isn't complete\n");
        }
    }

    if (LCM_INTERFACE_TYPE_DSI == lcm_setting.type) {
        hal_display_lcd_turn_on_mtcmos();
        hal_display_dsi_restore_callback();
        data_array[0] = 0x00023902;
        data_array[1] = 0x51 | (level << 8);
        hal_display_dsi_set_command_queue(data_array, 2, true);
        hal_display_dsi_deinit();
        hal_display_lcd_turn_off_mtcmos();
    }
#endif
}

#ifdef MTK_EXTERNAL_PMIC
void BSP_Backlight_init_external_isink(void)
{
    hal_pinmux_set_function(HAL_GPIO_44, 0); // ISINK EN
    hal_gpio_set_direction(HAL_GPIO_44, HAL_GPIO_DIRECTION_OUTPUT);

    hal_gpio_set_output(HAL_GPIO_44, HAL_GPIO_DATA_LOW);
    hal_gpio_set_output(HAL_GPIO_44, HAL_GPIO_DATA_HIGH);

}
void BSP_Backlight_deinit_external_isink(void)
{
    hal_pinmux_set_function(HAL_GPIO_44, 0); // ISINK EN
    hal_gpio_set_direction(HAL_GPIO_44, HAL_GPIO_DIRECTION_OUTPUT);

    hal_gpio_set_output(HAL_GPIO_44, HAL_GPIO_DATA_LOW);
}
#endif

#ifdef __cplusplus
}
#endif

