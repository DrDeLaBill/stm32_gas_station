/* Copyright © 2023 Georgy E. All rights reserved. */

#include "system.h"

#include "main.h"
#include "glog.h"
#include "pump.h"
#include "clock.h"
#include "hal_defs.h"


const char SYSTEM_TAG[] = "SYS";


uint16_t SYSTEM_ADC_VOLTAGE = 0;


#ifndef IS_SAME_TIME
#   define IS_SAME_TIME(TIME1, TIME2) (TIME1.Hours   == TIME2.Hours && \
                                       TIME1.Minutes == TIME2.Minutes && \
									   TIME1.Seconds == TIME2.Seconds)
#endif


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
}

void system_rtc_test(void)
{
#ifdef DEBUG
	static const char TEST_TAG[] = "TEST";
#endif
	gprint("\n\n\n");
	printTagLog(TEST_TAG, "RTC testing in progress...");

	RTC_DateTypeDef readDate  ={0};
	RTC_TimeTypeDef readTime = {0};

	printPretty("Get date test: ");
	if (!clock_get_rtc_date(&readDate)) {
		gprint("   error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	gprint("   OK\n");

	printPretty("Get time test: ");
	if (!clock_get_rtc_time(&readTime)) {
		gprint("   error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	gprint("   OK\n");


	printPretty("Save date test: ");
	if (!clock_save_date(&readDate)) {
		gprint("  error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	gprint("  OK\n");

	printPretty("Save time test: ");
	if (!clock_save_time(&readTime)) {
		gprint("  error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	gprint("  OK\n");


	RTC_DateTypeDef checkDate  ={0};
	RTC_TimeTypeDef checkTime = {0};
	printPretty("Check date test: ");
	if (!clock_get_rtc_date(&checkDate)) {
		gprint(" error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	if (memcmp((void*)&readDate, (void*)&checkDate, sizeof(readDate))) {
		gprint(" error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	gprint(" OK\n");

	printPretty("Check time test: ");
	if (!clock_get_rtc_time(&checkTime)) {
		gprint(" error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	if (!IS_SAME_TIME(readTime, checkTime)) {
		gprint(" error\n");
		system_error_handler(RTC_ERROR, NULL);
	}
	gprint(" OK\n");


	printPretty("Weekday test\n");
	const RTC_DateTypeDef dates[] = {
		{RTC_WEEKDAY_SATURDAY,  01, 01, 00},
		{RTC_WEEKDAY_SUNDAY,    01, 02, 00},
		{RTC_WEEKDAY_SATURDAY,  04, 27, 24},
		{RTC_WEEKDAY_SUNDAY,    04, 28, 24},
		{RTC_WEEKDAY_MONDAY,    04, 29, 24},
		{RTC_WEEKDAY_TUESDAY,   04, 30, 24},
		{RTC_WEEKDAY_WEDNESDAY, 05, 01, 24},
		{RTC_WEEKDAY_THURSDAY,  05, 02, 24},
		{RTC_WEEKDAY_FRIDAY,    05, 03, 24},
	};
	const RTC_TimeTypeDef times[] = {
		{00, 00, 00, 0, 0, 0, 0, 0},
		{00, 00, 00, 0, 0, 0, 0, 0},
		{03, 24, 49, 0, 0, 0, 0, 0},
		{04, 14, 24, 0, 0, 0, 0, 0},
		{03, 27, 01, 0, 0, 0, 0, 0},
		{23, 01, 40, 0, 0, 0, 0, 0},
		{03, 01, 40, 0, 0, 0, 0, 0},
		{04, 26, 12, 0, 0, 0, 0, 0},
		{03, 52, 35, 0, 0, 0, 0, 0},
	};
	const uint32_t seconds[] = {
		0,
		86400,
		767503489,
		767592864,
		767676421,
		767833300,
		767847700,
		767939172,
		768023555,
	};

	for (unsigned i = 0; i < __arr_len(seconds); i++) {
		printPretty("[%02u]: ", i);

		RTC_DateTypeDef tmpDate = {0};
		RTC_TimeTypeDef tmpTime = {0};
		clock_seconds_to_datetime(seconds[i], &tmpDate, &tmpTime);
		if (memcmp((void*)&tmpDate, (void*)&dates[i], sizeof(tmpDate))) {
			gprint("            error\n");
			system_error_handler(RTC_ERROR, NULL);
		}
		if (!IS_SAME_TIME(tmpTime, times[i])) {
			gprint("            error\n");
			system_error_handler(RTC_ERROR, NULL);
		}

		uint32_t tmpSeconds = clock_datetime_to_seconds(&dates[i], &times[i]);
		if (tmpSeconds != seconds[i]) {
			gprint("            error\n");
			system_error_handler(RTC_ERROR, NULL);
		}

		gprint("            OK\n");
	}


	printTagLog(TEST_TAG, "RTC testing done");
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

#ifdef STM32F1
	uint32_t backupregister = (uint32_t)BKP_BASE;
	backupregister += (RTC_BKP_DR1 * 4U);
	SOUL_STATUS status = (SOUL_STATUS)((*(__IO uint32_t *)(backupregister)) & BKP_DR1_D);
#elif defined(STM32F4)
	HAL_PWR_EnableBkUpAccess();
	SOUL_STATUS status = (SOUL_STATUS)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1);
	HAL_PWR_DisableBkUpAccess();
#endif

	set_last_error(status);

    if (status == MEMORY_ERROR) {
     	set_error(MEMORY_ERROR);
    } else if (status == STACK_ERROR) {
    	set_error(STACK_ERROR);
    } else if (status == SETTINGS_LOAD_ERROR) {
    	set_error(SETTINGS_LOAD_ERROR);
    }
}

void system_post_load(void)
{
	HAL_PWR_EnableBkUpAccess();
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0);
	HAL_PWR_DisableBkUpAccess();

#ifdef STM32F1
	HAL_ADCEx_Calibration_Start(&hadc1);
#endif
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&SYSTEM_ADC_VOLTAGE, 2);
	uint64_t counter = 0;
	uint64_t count_max = HAL_RCC_GetHCLKFreq() * 10;
	util_old_timer_t timer = {0};
	util_old_timer_start(&timer, 10000);
	while (1) {
		uint32_t voltage = get_system_power();
		if (STM_MIN_VOLTAGEx10 <= voltage && voltage <= STM_MAX_VOLTAGEx10) {
			break;
		}

		if (is_error(RCC_ERROR) && counter > count_max) {
			set_error(POWER_ERROR);
			break;
		} else if (!util_old_timer_wait(&timer)) {
			set_error(POWER_ERROR);
			break;
		}

		counter++;
	}

	if (has_errors()) {
		system_error_handler(
			(get_first_error() == INTERNAL_ERROR) ?
				LOAD_ERROR :
				(SOUL_STATUS)get_first_error(),
			NULL
		);
	}
}

void system_error_handler(SOUL_STATUS error, void (*error_loop) (void))
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

	printTagLog(SYSTEM_TAG, "system_error_handler called error=%u", error);

	/* Custom events begin */
	HAL_GPIO_WritePin(VALVE1_GPIO_Port, VALVE1_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(VALVE2_GPIO_Port, VALVE2_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, GPIO_PIN_RESET);
	/* Custom events end */

	HAL_PWR_EnableBkUpAccess();
	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, error);
	HAL_PWR_DisableBkUpAccess();

	uint64_t counter = 0;
	uint64_t count_max = HAL_RCC_GetHCLKFreq() * 10;
	util_old_timer_t timer = {0};
	util_old_timer_start(&timer, 10000);
	while(1) {
		if (error_loop) {
			error_loop();
		}

		if (is_error(RCC_ERROR) && counter > count_max) {
			set_error(POWER_ERROR);
			break;
		} else if (!util_old_timer_wait(&timer)) {
			set_error(POWER_ERROR);
			break;
		}

		counter++;
	}

#ifdef DEBUG
	printTagLog(SYSTEM_TAG, "system reset");
	counter = 100;
	while(counter--);
#endif

	NVIC_SystemReset();
}

uint32_t get_system_power(void)
{
	if (!SYSTEM_ADC_VOLTAGE) {
		return 0;
	}
	return (STM_ADC_MAX * STM_REF_VOLTAGEx10) / SYSTEM_ADC_VOLTAGE;
}

void system_reset_i2c_errata(void)
{
	printTagLog(SYSTEM_TAG, "RESET I2C (ERRATA)");

	HAL_I2C_DeInit(&EEPROM_I2C);

	GPIO_TypeDef* I2C_PORT = GPIOB;
	uint16_t I2C_SDA_Pin = GPIO_PIN_7;
	uint16_t I2C_SCL_Pin = GPIO_PIN_6;

	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin   = I2C_SCL_Pin | I2C_SCL_Pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStruct);

	hi2c1.Instance->CR1 &= (unsigned)~(0x0001);

	GPIO_InitTypeDef GPIO_InitStructure = {0};
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
	GPIO_InitStructure.Alternate = 0;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;


	GPIO_InitStructure.Pin = I2C_SCL_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(I2C_PORT, (uint16_t)(GPIO_InitStructure.Pin), GPIO_PIN_SET);

	GPIO_InitStructure.Pin = I2C_SDA_Pin;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
	HAL_GPIO_WritePin(I2C_PORT, (uint16_t)(GPIO_InitStructure.Pin), GPIO_PIN_SET);

	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) asm("nop");
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_Pin, GPIO_PIN_RESET);
	while(GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_Pin, GPIO_PIN_RESET);
	while(GPIO_PIN_RESET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SDA_Pin, GPIO_PIN_SET);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SDA_Pin)) asm("nop");

	HAL_GPIO_WritePin(I2C_PORT, I2C_SCL_Pin, GPIO_PIN_SET);
	while(GPIO_PIN_SET != HAL_GPIO_ReadPin(I2C_PORT, I2C_SCL_Pin)) asm("nop");

	GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
	GPIO_InitStructure.Alternate = GPIO_AF4_I2C1;

	GPIO_InitStructure.Pin = I2C_SCL_Pin;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = I2C_SDA_Pin;
	HAL_GPIO_Init(I2C_PORT, &GPIO_InitStructure);

	EEPROM_I2C.Instance->CR1 |= 0x8000;
	asm("nop");
	EEPROM_I2C.Instance->CR1 &= (unsigned)~0x8000;
	asm("nop");

	EEPROM_I2C.Instance->CR1 |= 0x0001;

	HAL_I2C_Init(&EEPROM_I2C);
}
