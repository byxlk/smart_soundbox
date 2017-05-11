/*
    FreeRTOS V8.2.0 - Copyright (C) 2015 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

	***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
	***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
	the FAQ page "My application does not run, what could be wrong?".  Have you
	defined configASSERT()?

	http://www.FreeRTOS.org/support - In return for receiving this top quality
	embedded software for free we request you assist our global community by
	participating in the support forum.

	http://www.FreeRTOS.org/training - Investing in training allows your team to
	be as productive as possible as early as possible.  Now you can receive
	FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
	Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

#include "FreeRTOS.h"
#include "hal_platform.h"

#if configUSE_TICKLESS_IDLE == 2
#include "task.h"
#include "port_tick.h"
#include "hal_gpt.h"
#include "gpt.h"
#include "timer.h"
#include "hal_log.h"
#include "type_def.h"
#include "hal_lp.h"
#include "core_cm4.h"
#include "top.h"
#include "connsys_driver.h"
#include "hal_sleep_driver.h"
#include <string.h>
#include "hal.h"
#include "timers.h"
#include "hal_rtc.h"
#endif /* configUSE_TICKLESS_IDLE == 2 */

#ifndef configSYSTICK_CLOCK_HZ
#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
/* Ensure the SysTick is clocked at the same frequency as the core. */
#define portNVIC_SYSTICK_CLK_BIT    ( 1UL << 2UL )
#else
/* The way the SysTick is clocked is not modified in case it is not the same
as the core. */
#define portNVIC_SYSTICK_CLK_BIT    ( 0 )
#endif

#define TICKLESS_TIME_ACCURACY_DEBUG 0

#if configUSE_TICKLESS_IDLE != 0
static uint32_t ulTimerCountsForOneTick = 0;
static uint32_t ulStoppedTimerCompensation = 0;
static uint32_t xMaximumPossibleSuppressedTicks = 0;
/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR                        ( 45UL )
#endif /* configUSE_TICKLESS_IDLE != 0 */

#if configUSE_TICKLESS_IDLE == 2
#define HWResumeOverhead            (5)
#define PlatformIdleOverhead        (10)  // mt7687 worst case is 5ms (ext 32k/N9 sleep/CM4 sleep)

void tickless_log_timestamp(void);

extern hal_gpt_port_t wakeup_gpt_port;

unsigned char AST_TimeOut_flag = 1;

#define AST_CLOCK 32768 //32.768 kHz
#define xMaximumPossibleASTTicks (0xFFFFFFFF / (AST_CLOCK/configTICK_RATE_HZ))

#define TICKLESS_DEBUG 0
#define RTC_CALIBRATION_ENABLE  0
#define GIVE_OWN_BIT   1

#if TICKLESS_DEBUG == 1
uint32_t bAbort = 0;
uint32_t workaround = 0;
#endif

#if RTC_CALIBRATION_ENABLE
uint32_t rtc_clock;
uint32_t rtc_clock_now;
#endif

ATTR_RWDATA_IN_TCM uint8_t ticklessStatus = 0;

TickType_t gExpectedIdleTime;

extern uint32_t ticklessCount;
extern uint16_t ticklessWFICount;
extern uint32_t ticklessTryCount;
extern void (*ptr_tickless_cb)(void*);

uint32_t ulAST_Reload_ms = 0;
uint32_t ulSleepTickPeriods;
uint32_t ulReloadValue;
uint32_t ulCompletedCountDecrements;
uint32_t ulCompletedTickDecrements;
uint32_t ulPassedSystick;

static uint32_t TimeStampCounter;
static uint32_t TimeStampSystick;

float RTC_Freq = 32.768f;

#if TICKLESS_TIME_ACCURACY_DEBUG
#define TICKLESS_DEBUG_TICKS  10000
extern TickType_t gExpectedIdleTime;
extern unsigned char AST_TimeOut_flag;
extern uint32_t ulAST_Current_ms;
extern uint32_t ulAST_Reload_ms;
extern uint32_t ulReloadValue;
extern uint32_t ulCompleteTickPeriods;
extern uint32_t ulCompletedSysTickDecrements;
extern uint32_t ulAST_CurrentCount;
static TimerHandle_t timer1 = NULL;
static TimerHandle_t timer2 = NULL;

static void tickless_debug_timer_callback(TimerHandle_t expiredTimer)
{
    uint32_t count;
    //hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &count);

    printf("%u, %u, %u, %u\n", (unsigned int)xTaskGetTickCount(), hal_sleep_manager_get_lock_status(), ticklessWFICount, ticklessCount);
#if 0
    printf("\tulCompleteTickPeriods = %d ms\n", ulCompleteTickPeriods);
    printf("\tulCompletedSysTickDecrements = %d\n", ulCompletedSysTickDecrements);
    printf("\tulCompensation = %d\n", ulCompensation);
    printf("\tulCompensation2 = %d\n", ulCompensation2);
    printf("\txExpectedIdleTime = %d ms\n", gExpectedIdleTime);
    printf("\tAST_TimeOut_flag = %d\n", AST_TimeOut_flag);
    printf("\tulAST_CurrentCount = %d\n", ulAST_CurrentCount);
    printf("\tulAST_Current_ms = %u ms\n", ulAST_Current_ms);
    printf("\tulAST_Reload_ms = %d ms\n", ulAST_Reload_ms);
    printf("\tulReloadValue = %u\n", ulReloadValue);
    printf("\tulWakeUpByEvent = %d\n", ulWakeUpByEvent);
    printf("\tulWakeUpByTimer = %d\n", ulWakeUpByTimer);
    printf("\tticklessCount = %d\n\n", ticklessCount);
#endif
}

static void tickless_dummy_debug_timer_callback(TimerHandle_t expiredTimer)
{
}
#endif


void AST_vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
#if RTC_CALIBRATION_ENABLE
    static bool calibration_done = false;
#endif

    TickType_t xModifiableIdleTime;
    uint32_t nowCount;
    uint32_t nowTick;

    ticklessStatus = 4;

#if RTC_CALIBRATION_ENABLE
    if (calibration_done == false) {
        hal_rtc_get_f32k_frequency(&rtc_clock_now);
        if (rtc_clock_now == rtc_clock) {
            calibration_done = true;
            RTC_Freq =  ((float)(rtc_clock)/1000);
            printf("calibration done, %u, %f\n", rtc_clock, RTC_Freq);
        }
        rtc_clock = rtc_clock_now; 
    }
#endif

    gExpectedIdleTime = xExpectedIdleTime;
    AST_TimeOut_flag = 0;

    //need ues AST
    /* Calculate the reload value required to wait xExpectedIdleTime
    tick periods.  -1 is used because this code will execute part way
    through one of the tick periods. */
    ulReloadValue = SysTick->VAL;
    if (ulReloadValue > ulStoppedTimerCompensation) {
        ulReloadValue -= ulStoppedTimerCompensation;
    }

    //Calculate total idle time to ms
    ulAST_Reload_ms = ((xExpectedIdleTime - 1) / (1000 / configTICK_RATE_HZ)) - HWResumeOverhead;

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
    method as that will mask interrupts that should exit sleep mode. */
    __asm volatile("cpsid i");

    /* If a context switch is pending or a task is waiting for the scheduler
    to be unsuspended then abandon the low power entry. */
    if (eTaskConfirmSleepModeStatus() == eAbortSleep) {
#if TICKLESS_DEBUG == 1
        bAbort = 1;
#endif
        /* Restart from whatever is left in the count register to complete
        this tick period. */
        SysTick->LOAD = SysTick->VAL;

        /* Restart SysTick. */
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

        /* Reset the reload register to the value required for normal tick
        periods. */
        SysTick->LOAD = ulTimerCountsForOneTick - 1UL;

        /* Re-enable interrupts - see comments above the cpsid instruction()
        above. */
        __asm volatile("cpsie i");

        return;
    } else {
        hal_sleep_manager_set_sleep_time(ulAST_Reload_ms);

        /* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
        set its parameter to 0 to indicate that its implementation contains
        its own wait for interrupt or wait for event instruction, and so wfi
        should not be executed again.  However, the original expected idle
        time variable must remain unmodified, so a copy is taken. */
        xModifiableIdleTime = xExpectedIdleTime;

        configPRE_SLEEP_PROCESSING(xModifiableIdleTime);
        if (xModifiableIdleTime > 0) {
            ticklessCount++;

#if GIVE_OWN_BIT
            /* Enable FW_OWN_BACK_INT interrupt */
            hal_lp_connsys_get_own_enable_int();
            /* Give connsys ownership to N9 */
            hal_lp_connsys_give_n9_own();
#endif

#ifdef HAL_WDT_PROTECTION_ENABLED
            hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif

            hal_sleep_manager_enter_sleep_mode(sleepdrv_get_sleep_mode());
            ticklessStatus = 5;

#ifdef HAL_WDT_PROTECTION_ENABLED
            hal_wdt_feed(HAL_WDT_FEED_MAGIC);
#endif

#if GIVE_OWN_BIT
            /* re-init connsys for handling inband-cmd response */
            if (FALSE == connsys_get_ownership())
                log_hal_info("connsys_get_ownership fail\n");
#endif
        }
        configPOST_SLEEP_PROCESSING(xExpectedIdleTime);

        hal_gpt_stop_timer(wakeup_gpt_port);

        //calculate time(systick) to jump
        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &nowCount);
        nowTick = (uint32_t)xTaskGetTickCount();
        // get counter distance from last record
        if (nowCount >= TimeStampCounter)
            ulCompletedCountDecrements = nowCount - TimeStampCounter;
        else
            ulCompletedCountDecrements = nowCount + (0xFFFFFFFF - TimeStampCounter);
        // get systick distance from last record
        if (nowTick >= TimeStampSystick)
            ulCompletedTickDecrements = nowTick - TimeStampSystick;
        else
            ulCompletedTickDecrements = nowTick + (0xFFFFFFFF - TimeStampSystick);

        // get systick distance for this sleep
        ulPassedSystick = (uint32_t)(((float)ulCompletedCountDecrements)/RTC_Freq);
        // calculate ticks for jumpping
        ulSleepTickPeriods = ulPassedSystick - ulCompletedTickDecrements;

        //Limit OS Tick Compensation Value
        if (ulSleepTickPeriods > (xExpectedIdleTime - 1)) {
            ulSleepTickPeriods = xExpectedIdleTime - 1;
        }

        SysTick->LOAD = ulReloadValue;
        SysTick->VAL = 0UL;
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
        vTaskStepTick(ulSleepTickPeriods);
        SysTick->LOAD = ulTimerCountsForOneTick - 1UL;

        __asm volatile("cpsie i");
    }

#if TICKLESS_DEBUG == 1
    printf("xExpectedIdleTime = %d ms\n", xExpectedIdleTime);
    printf("bAbort = %d\n", bAbort);
    printf("workaround = %d\n", workaround);
    printf("AST_TimeOut_flag = %d\n", AST_TimeOut_flag);
    printf("ulAST_Reload_ms = %d ms\n", ulAST_Reload_ms);
    printf("ulSleepTickPeriods = %d ms\n", ulSleepTickPeriods);
    printf("ulReloadValue = %u\n\n", ulReloadValue);
    bAbort = 0;
    workaround = 0;
#endif
}

void tickless_GPT_CB(void* data)
{
    AST_TimeOut_flag = 1;
}

static void tickless_log_timestamp_callback(TimerHandle_t expiredTimer)
{
    tickless_log_timestamp();
}

void tickless_init()
{
    TimerHandle_t timer = NULL;
    ptr_tickless_cb = tickless_GPT_CB;

    tickless_log_timestamp();

    timer = xTimerCreate("tickless_log_timestamp_timer",
                         1000*60*60*12,  //12hours
                         true,
                         NULL,
                         tickless_log_timestamp_callback);

    if (timer == NULL) {
        printf("timer create fail\n");
    }
    else {
        if (xTimerStart(timer, 0) != pdPASS)
            printf("xTimerStart fail\n");
    }

#if TICKLESS_TIME_ACCURACY_DEBUG
    timer1 = xTimerCreate("tickless_debug_timer",
                         TICKLESS_DEBUG_TICKS,
                         true,
                         NULL,
                         tickless_debug_timer_callback);

    if (timer1 == NULL) {
        printf("tickless_debug_timer create fail\n");
    }
    else {
        if (xTimerStart(timer1, 0) != pdPASS)
            printf("tickless_debug_timer fail\n");
        else
            printf("tickless_debug_timer start\n");
    }

    timer2 = xTimerCreate("tickless_dummy_timer",
                         TICKLESS_DEBUG_TICKS/40,
                         true,
                         NULL,
                         tickless_dummy_debug_timer_callback);

    if (timer2 == NULL) {
        printf("tickless_dummy_timer create fail\n");
    }
    else {
        if (xTimerStart(timer2, 0) != pdPASS)
            printf("tickless_dummy_timer fail\n");
        else
            printf("tickless_dummy_timer start\n");
    }

#if 1
    uint8_t tickless_test_sleep_handle;
    tickless_test_sleep_handle = hal_sleep_manager_set_sleep_handle("tickless_test");
    printf("tickless_test_sleep_handle %d\n", tickless_test_sleep_handle);
    hal_sleep_manager_lock_sleep(tickless_test_sleep_handle);
#endif
#endif
}

void tickless_handler(TickType_t xExpectedIdleTime)
{
    uint32_t ulReloadValue, ulCompleteTickPeriods;
    TickType_t xModifiableIdleTime;
    static long unsigned int before_sleep_time, after_sleep_time, sleep_time, SystickCompensation;
    static uint32_t StoppedSystickCompensation;

    if (sleepdrv_get_sleep_mode() == HAL_SLEEP_MODE_NONE)
    {
        return;
    }

    ticklessTryCount++;

    ticklessStatus = 1;

    /* Stop the SysTick momentarily.  The time the SysTick is stopped for
    is accounted for as best it can be, but using the tickless mode will
    inevitably result in some tiny drift of the time maintained by the
    kernel with respect to calendar time. */
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, (uint32_t *)&before_sleep_time);
    
    StoppedSystickCompensation = (SysTick->LOAD - SysTick->VAL);

#ifdef HAL_WDT_PROTECTION_ENABLED
    if (xExpectedIdleTime >= ((HAL_WDT_TIMEOUT_VALUE-1)*1000))
    {
        xExpectedIdleTime = (HAL_WDT_TIMEOUT_VALUE-1)*1000;
    }
#endif

    /* Make sure the SysTick reload value does not overflow the counter. */
    if ((xExpectedIdleTime > PlatformIdleOverhead) && (!hal_sleep_manager_is_sleep_locked())) {
        //xExpectedIdleTime = xMaximumPossibleSuppressedTicks;

        /* Make sure the AST reload value does not overflow the counter. */
        if (xExpectedIdleTime > xMaximumPossibleASTTicks) {
            xExpectedIdleTime = xMaximumPossibleASTTicks;
        }

        AST_vPortSuppressTicksAndSleep(xExpectedIdleTime);

        return;
    }

    if (xExpectedIdleTime > xMaximumPossibleSuppressedTicks) {
        xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
    }

    ticklessStatus = 2;

    /* Enter a critical section but don't use the taskENTER_CRITICAL()
    method as that will mask interrupts that should exit sleep mode. */
    __asm volatile("cpsid i");

    /* If a context switch is pending or a task is waiting for the scheduler
    to be unsuspended then abandon the low power entry. */
    if (eTaskConfirmSleepModeStatus() == eAbortSleep) {
        ticklessStatus = 6;
        /* Restart from whatever is left in the count register to complete
        this tick period. */
        SysTick->LOAD = SysTick->VAL;

        /* Restart SysTick. */
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;

        /* Reset the reload register to the value required for normal tick
        periods. */
        SysTick->LOAD = ulTimerCountsForOneTick - 1UL;

        /* Re-enable interrupts - see comments above the cpsid instruction()
        above. */
        __asm volatile("cpsie i");
        //ticklessStatus = 7;
    } else {
        ulReloadValue = ((xExpectedIdleTime - 1UL) * (1000 / configTICK_RATE_HZ));
        hal_sleep_manager_set_sleep_time((uint32_t)ulReloadValue);

        xModifiableIdleTime = xExpectedIdleTime;
        configPRE_SLEEP_PROCESSING(xModifiableIdleTime);
        if (xModifiableIdleTime > 0) {
            __asm volatile("dsb");
            __asm volatile("wfi");
            __asm volatile("isb");
        }
        configPOST_SLEEP_PROCESSING(xExpectedIdleTime);
        ticklessWFICount++;

        hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_1M, (uint32_t *)&after_sleep_time);

        if (after_sleep_time >= before_sleep_time) {
            sleep_time = after_sleep_time - before_sleep_time;
        } else {
            sleep_time = after_sleep_time + (0xFFFFFFFF - before_sleep_time);
        }

        ulCompleteTickPeriods = (sleep_time / 1000) / (1000 / configTICK_RATE_HZ);

        sleep_time -= (ulCompleteTickPeriods * 1000) * (1000 / configTICK_RATE_HZ);
        sleep_time = (1000 * (1000 / configTICK_RATE_HZ)) - sleep_time;

        StoppedSystickCompensation = StoppedSystickCompensation / (configSYSTICK_CLOCK_HZ / 1000000);

        if (sleep_time >= (StoppedSystickCompensation)) {
            sleep_time = sleep_time - (StoppedSystickCompensation);
        } else {
            ulCompleteTickPeriods++;
            sleep_time = (1000 * (1000 / configTICK_RATE_HZ)) - ((StoppedSystickCompensation) - sleep_time);
        }

        if (sleep_time <= 5) {
            SystickCompensation = ulTimerCountsForOneTick - (((sleep_time) * ulTimerCountsForOneTick) / 1000 / (1000 / configTICK_RATE_HZ));
            ulCompleteTickPeriods++;
        } else {
            SystickCompensation = sleep_time * (ulTimerCountsForOneTick / 1000 / (1000 / configTICK_RATE_HZ));
        }

        //Limit OS Tick Compensation Value
        if (ulCompleteTickPeriods >= (xExpectedIdleTime)) {
            ulCompleteTickPeriods = xExpectedIdleTime;
        }        

        SysTick->LOAD = SystickCompensation;
        SysTick->VAL = 0UL;
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;        
        vTaskStepTick(ulCompleteTickPeriods);
        SysTick->LOAD = ulTimerCountsForOneTick - 1UL;
        SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

        /* Re-enable interrupts - see comments above the cpsid instruction() above. */
        __asm volatile("cpsie i");
    }
    ticklessStatus = 3;
}

void tickless_log_timestamp()
{
#if RTC_CALIBRATION_ENABLE
    uint32_t rtc_clock;
#endif
    hal_gpt_get_free_run_count(HAL_GPT_CLOCK_SOURCE_32K, &TimeStampCounter);
    TimeStampSystick = (uint32_t)xTaskGetTickCount() * (1000 / configTICK_RATE_HZ);
#if RTC_CALIBRATION_ENABLE
    hal_rtc_get_f32k_frequency(&rtc_clock);
    RTC_Freq = ((float)(rtc_clock)/1000);
#endif
}
#endif /* configUSE_TICKLESS_IDLE == 2 */

void vPortSetupTimerInterrupt(void)
{
    /* Calculate the constants required to configure the tick interrupt. */
#if configUSE_TICKLESS_IDLE != 0
    {
        ulTimerCountsForOneTick = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ);
        xMaximumPossibleSuppressedTicks = SysTick_LOAD_RELOAD_Msk / ulTimerCountsForOneTick;
        ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / (configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ);
    }
#endif /* configUSE_TICKLESS_IDLE != 0 */

    /* Configure SysTick to interrupt at the requested rate. */
    SysTick->LOAD = (configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}

