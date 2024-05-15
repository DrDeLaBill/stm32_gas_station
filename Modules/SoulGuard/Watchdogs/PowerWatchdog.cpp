/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "log.h"
#include "main.h"
#include "soul.h"
#include "clock.h"
#include "hal_defs.h"
#include "settings.h"

#include "UI.h"
#include "Pump.h"
#include "defines.h"
#include "RecordTmp.h"
#include "SettingsDB.h"
#include "CodeStopwatch.h"


uint32_t PowerWatchdog::getPower()
{
#ifdef POWER_Pin
    ADC_ChannelConfTypeDef conf = {};
    conf.Channel      = POWER_ADC_CHANNEL;
    conf.Rank         = 1;
    conf.SamplingTime = ADC_SAMPLETIME_28CYCLES;
    if (HAL_ADC_ConfigChannel(&POWER_ADC, &conf) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&POWER_ADC);
    HAL_ADC_PollForConversion(&POWER_ADC, GENERAL_TIMEOUT_MS);
    uint32_t value = HAL_ADC_GetValue(&POWER_ADC);
    HAL_ADC_Stop(&POWER_ADC);

    return value;
#endif
    return 0;
}

void PowerWatchdog::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

#ifdef POWER_Pin
	if (getPower() < TRIG_LEVEL) {
		reset_error(POWER_ERROR);
		return;
	}

#if POWER_WATCHDOG_BEDUG
	printTagLog(TAG, "POWER = %lu", getPower());
#endif
	set_error(POWER_ERROR);


	// Interrupts

    HAL_NVIC_DisableIRQ(static_cast<IRQn_Type>(static_cast<int>(
		USART1_IRQn |
#if !POWER_WATCHDOG_BEDUG
		USART2_IRQn |
#endif
		TIM3_IRQn |
		TIM4_IRQn |
		TIM5_IRQn |
    	EXTI15_10_IRQn
    )));

	// GPIO
	HAL_GPIO_WritePin(GPIOC, VALVE1_Pin|VALVE2_Pin|DIGITS_5_Pin|DIGITS_6_Pin
						  |KBD_ROW1_Pin|KBD_ROW2_Pin|KBD_ROW3_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOA, PUMP_Pin|DIGITS_1_Pin|DIGITS_2_Pin|DIGITS_4_Pin
						  |DIGITS_3_Pin|RFID_LED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, DIGITS_D_Pin|DIGITS_E_Pin|DIGITS_C_Pin|DIGITS_G_Pin
						  |DIGITS_DP_Pin|DIGITS_F_Pin|DIGITS_A_Pin|DIGITS_B_Pin
						  |KBD_BL_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(KBD_ROW4_GPIO_Port, KBD_ROW4_Pin, GPIO_PIN_RESET);
	__HAL_RCC_GPIOC_CLK_DISABLE();
	__HAL_RCC_GPIOH_CLK_DISABLE();
	__HAL_RCC_GPIOA_CLK_DISABLE();
	__HAL_RCC_GPIOD_CLK_DISABLE();

	// ADC
    __HAL_RCC_ADC1_CLK_DISABLE();

    // TIM
    __HAL_RCC_TIM3_CLK_DISABLE();
    __HAL_RCC_TIM4_CLK_DISABLE();
    __HAL_RCC_TIM5_CLK_DISABLE();

    // UART
    __HAL_RCC_USART1_CLK_DISABLE();
#if !POWER_WATCHDOG_BEDUG
    // Bedug UART
    __HAL_RCC_USART2_CLK_DISABLE();
#endif

    uint32_t curr_count = 0;
    uint32_t curr_card = UI::getCard();
    if (UI::isPumpWorking()) {
    	curr_count = Pump::getCurrentMl();
    }

    if (!curr_count || !curr_card) {
#if POWER_WATCHDOG_BEDUG
    	const uint8_t msg[] = "PUMP IS NOT WORKING. POWER ERROR. REBOOT DEVICE.\n";
    	HAL_UART_Transmit(&BEDUG_UART, msg, strlen((char*)msg), GENERAL_TIMEOUT_MS);
#endif
    	NVIC_SystemReset();
    	return;
    }

    RecordTmp::save(curr_card, curr_count);

#if POWER_WATCHDOG_BEDUG
	printTagLog(TAG, "POWER ERROR. REBOOT DEVICE.");
#endif

	while (1) {}
#endif
}
