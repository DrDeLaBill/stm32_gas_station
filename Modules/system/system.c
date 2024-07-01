/* Copyright © 2023 Georgy E. All rights reserved. */

#include "system.h"

#include "main.h"
#include "pump.h"
#include "hal_defs.h"


uint16_t SYSTEM_ADC_VOLTAGE = 0;


void system_clock_hsi_config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

	/** Configure the main internal regulator output voltage
	*/
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI
										|RCC_OSCILLATORTYPE_LSE;
	RCC_OscInitStruct.LSEState = RCC_LSE_ON;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.LSIState = RCC_LSI_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 84;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
									|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
	{
		Error_Handler();
	}

//	reset_error(RCC_ERROR);
}

void system_pre_load(void)
{
	if (!MCUcheck()) {
		set_error(MCU_ERROR);
		while (1) {}
	}

	SET_BIT(RCC->CR, RCC_CR_HSEON_Pos);

	unsigned counter = 0;
	while (1) {
		if (READ_BIT(RCC->CR, RCC_CR_HSERDY_Pos)) {
			CLEAR_BIT(RCC->CR, RCC_CR_HSEON_Pos);
			break;
		}

		if (counter > 0x100) {
			set_status(RCC_FAULT);
			set_error(RCC_ERROR);
			break;
		}

		counter++;
	}

	HAL_PWR_EnableBkUpAccess();
	SOUL_STATUS status = (SOUL_STATUS)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
	HAL_PWR_DisableBkUpAccess();

	set_last_error(status);
	switch (status) {
	case RCC_ERROR:
        break;
    case MEMORY_ERROR:
    	set_error(MEMORY_ERROR);
        break;
    case POWER_ERROR:
        break;
    case STACK_ERROR:
    	set_error(STACK_ERROR);
        break;
    case LOAD_ERROR:
        break;
    case RAM_ERROR:
        break;
    case USB_ERROR:
        break;
    case SETTINGS_LOAD_ERROR:
    	set_error(SETTINGS_LOAD_ERROR);
        break;
    case APP_MODE_ERROR:
        break;
    case VALVE_ERROR:
        break;
    case NON_MASKABLE_INTERRUPT:
        break;
    case HARD_FAULT:
        break;
    case MEM_MANAGE:
        break;
    case BUS_FAULT:
        break;
    case USAGE_FAULT:
        break;
    case ASSERT_ERROR:
        break;
    case ERROR_HANDLER_CALLED:
    	break;
    case INTERNAL_ERROR:
        break;
    default:
		break;
	}
}

void system_post_load(void)
{
	HAL_PWR_EnableBkUpAccess();
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0);
	HAL_PWR_DisableBkUpAccess();

//	HAL_ADCEx_Calibration_Start(&hadc1);
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&SYSTEM_ADC_VOLTAGE, 1);
	unsigned counter = 0;
	while (1) {
		uint16_t voltage = 0;
		if (SYSTEM_ADC_VOLTAGE) {
			voltage = STM_ADC_MAX * STM_REF_VOLTAGEx10 / SYSTEM_ADC_VOLTAGE;
		}

		if (STM_MIN_VOLTAGEx10 <= voltage && voltage <= STM_MAX_VOLTAGEx10) {
			break;
		}

		if (counter > 0x1000) {
			set_error(POWER_ERROR);
			break;
		}

		counter++;
	}

	if (has_errors()) {
		system_error_handler(
			(get_first_error() == INTERNAL_ERROR) ?
				LOAD_ERROR :
				(SOUL_STATUS)get_first_error()
		);
	}
}

void system_error_handler(SOUL_STATUS error)
{
	static bool called = false;
	if (called) {
		return;
	}
	called = true;

	set_error(error);

	if (!has_errors()) {
		error = INTERNAL_ERROR;
	}

	HAL_GPIO_WritePin(VALVE1_GPIO_Port, VALVE1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(VALVE2_GPIO_Port, VALVE2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_RESET);

	HAL_PWR_EnableBkUpAccess();
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, error);
	HAL_PWR_DisableBkUpAccess();

	uint32_t counter = 0x100;
	while(--counter) {
		pump_stop();
	}
	NVIC_SystemReset();
}
