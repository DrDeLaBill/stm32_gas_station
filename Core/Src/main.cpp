/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "iwdg.h"
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>
#include <string.h>

#include "glog.h"
#include "soul.h"
#include "gutils.h"
#include "clock.h"
#include "TM1637.h"
#include "system.h"
//#include "wialon.h"
#include "wiegand.h"
#include "settings.h"
#include "at24cm01.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"

#include "UI.h"
#include "pump.h"
#include "Access.h"
#include "Record.h"
#include "SoulGuard.h"
#include "StorageAT.h"
#include "RecordTmp.h"
#include "StorageDriver.h"
#include "ModbusManager.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

#ifdef DEBUG
static const char MAIN_TAG[] = "MAIN";
#endif

uint8_t modbus_uart_byte = 0;

StorageDriver storageDriver;
StorageAT* storage;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void save_new_log(uint32_t mlCount);

void record_check();

void error_loop();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

ModbusManager mbManager(&MODBUS_UART);

SoulGuard<
	RestartWatchdog,
	MemoryWatchdog,
	StackWatchdog,
	SettingsWatchdog,
	PowerWatchdog,
	RTCWatchdog
> soulGuard;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
	system_pre_load();
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  if (is_error(RCC_ERROR)) {
	  system_clock_hsi_config();
  } else {
	  set_error(RCC_ERROR);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  	  reset_error(RCC_ERROR);
  }

#ifdef __STANDART_GAS_STATION__
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  MX_IWDG_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */

#else


#   if IS_DEVICE_WITH_4PIN()

  MX_GPIO_Init();
  MX_DMA_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
#       ifndef DEBUG
  MX_IWDG_Init();
#       endif
  MX_TIM5_Init();

#   else
  DISPLAY_16PIN_GPIO_Init();

  MX_DMA_Init();
  MX_RTC_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
#       ifndef DEBUG
  MX_IWDG_Init();
#       endif
  MX_TIM5_Init();
#   endif

#endif

#ifdef EXTERNAL_LOGGER
    HAL_Delay(5000);
#endif

    set_status(LOADING);

#ifdef DEBUG
    HAL_Delay(300);
#else
    HAL_Delay(100);
#endif

    utl::Timer errTimer(40 * SECOND_MS);

	gprint("\n\n\n");
	printTagLog(MAIN_TAG, "The device is loading");

	SystemInfo();

	set_status(LOADING);

	GPIO_PAIR clk{TM1637_CLK_GPIO_Port, TM1637_CLK_Pin};
	GPIO_PAIR data{TM1637_DATA_GPIO_Port, TM1637_DATA_Pin};
	tm1637_init(&clk, &data);

	// Indicators timer start
	HAL_TIM_Base_Start_IT(&INDICATORS_TIM);

	// UI & pump timer start
	pump_init();
	HAL_TIM_Base_Start_IT(&UI_TIM);

	// Gas sensor encoder
	HAL_TIM_Encoder_Start(&MD212_TIM, TIM_CHANNEL_ALL);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	storage = new StorageAT(
		eeprom_get_size() / STORAGE_PAGE_SIZE,
		&storageDriver,
		EEPROM_PAGE_SIZE
	);

	system_rtc_test();

    errTimer.start();
	while (has_errors() || is_status(LOADING)) {
		soulGuard.defend();

    	if (!errTimer.wait()) {
			system_error_handler((SOUL_STATUS)get_first_error(), error_loop);
		}
	}

//	wln_begin();

    system_post_load();

	Record::showMax();

	if (RecordTmp::exists()) {
		RecordTmp::restore();
	}

	printTagLog(MAIN_TAG, "The device is loaded successfully");

#ifdef DEBUG
	unsigned last_error = get_first_error();

	unsigned kFLOPScounter = 0;
	utl::Timer kFLOPSTimer(10 * SECOND_MS);
	kFLOPSTimer.start();
#endif
	set_status(WORKING);
	errTimer.start();
	while (1)
	{
		soulGuard.defend();

#ifdef DEBUG
		unsigned error = get_first_error();
		if (error && last_error != error) {
			printTagLog(MAIN_TAG, "New error: %u", error);
			last_error = error;
		} else if (last_error != error) {
			printTagLog(MAIN_TAG, "No errors");
			last_error = error;
		}

		kFLOPScounter++;
		if (!kFLOPSTimer.wait()) {
			printTagLog(
				MAIN_TAG,
				"kFLOPS: %lu.%lu",
				kFLOPScounter / (10 * SECOND_MS),
				(kFLOPScounter / SECOND_MS) % 10
			);
			kFLOPScounter = 0;
			kFLOPSTimer.start();
		}
#endif

		if (!errTimer.wait()) {
			system_error_handler((SOUL_STATUS)get_first_error(), error_loop);
		}

		if (has_errors() || is_status(LOADING)) {
			continue;
		}

		HAL_IWDG_Refresh(&DEVICE_IWDG);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		if (!(MODBUS_UART.Instance->CR1 & USART_CR1_RXNEIE)) {
			// MODBUS uart initialization
			HAL_UART_Receive_IT(&MODBUS_UART, (uint8_t*)&modbus_uart_byte, 1);
		}

		settings_check_residues();

		record_check();

//		wln_proccess();

		mbManager.tick();

		errTimer.start();
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
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

/* USER CODE BEGIN 4 */

void record_check()
{
	static uint32_t resultMlBuf = 0;

	uint32_t resultMl = UI::getResultMl();

	if (is_status(NEED_SAVE_FINAL_RECORD)) {
#ifdef DEBUG
		printTagLog(MAIN_TAG, "Saving final record (card=%lu ml=%lu)", UI::getCard(), resultMl);
#endif
		if (RecordTmp::remove() == RECORD_OK) {
			save_new_log(resultMl);
			UI::resetResultMl();
			resultMlBuf = 0;
			resultMl = 0;

			reset_status(NEED_SAVE_FINAL_RECORD);
			reset_status(NEED_INIT_RECORD_TMP);
			reset_status(NEED_SAVE_RECORD_TMP);
		}
	} else if (is_status(NEED_INIT_RECORD_TMP)) {
#ifdef DEBUG
		printTagLog(MAIN_TAG, "Initializing record tmp");
#endif
		if (RecordTmp::init() == RECORD_OK) {
			reset_status(NEED_INIT_RECORD_TMP);
		}
	} else if (__abs_dif(resultMl, resultMlBuf) > RecordTmp::TRIG_LEVEL_ML) {
#ifdef DEBUG
		printTagLog(MAIN_TAG, "Saving record tmp (card=%lu ml=%lu)", UI::getCard(), resultMl);
#endif
    	RecordTmp::save(UI::getCard(), resultMl);
		set_status(NEED_SAVE_RECORD_TMP);
    	resultMlBuf = resultMl;
	}
}

void save_new_log(uint32_t mlCount)
{
    if (mlCount == 0) {
        return;
    }

    set_status(LOADING);

    Record record(0);

    record.record.time     = clock_get_timestamp();
    record.record.used_mls = mlCount;
    record.record.card     = UI::getCard();

    printTagLog(MAIN_TAG, "save new log: begin");
    printTagLog(MAIN_TAG, "save new log: real mls=%lu", pump_count_ml());

    RecordStatus status = record.save();
    if (status != RECORD_OK) {
        printTagLog(MAIN_TAG, "save new logerror=%02x", status);
    } else {
    	printTagLog(MAIN_TAG, "save new log: success");
    }

	printTagLog(MAIN_TAG, "adding %lu used milliliters for %lu card", record.record.used_mls, record.record.card);
    settings_add_used_liters(record.record.used_mls, record.record.card);
    set_status(NEED_SAVE_SETTINGS);

    reset_status(LOADING);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == MODBUS_UART.Instance) {
        mbManager.recieveByte(modbus_uart_byte);
        HAL_UART_Receive_IT(&MODBUS_UART, (uint8_t*)&modbus_uart_byte, 1);
    } else {
    	Error_Handler();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == RFID_D0_Pin) {
        wiegand_set_value(0);
    } else if(GPIO_Pin == RFID_D1_Pin) {
        wiegand_set_value(1);
    }
}

void error_loop()
{
	soulGuard.defend();
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == INDICATORS_TIM.Instance) {
#if IS_DEVICE_WITH_4PIN()
    	tm1637_proccess();
#else
        indicate_proccess();
#endif
    } else if (htim->Instance == UI_TIM.Instance) {
#if IS_DEVICE_WITH_KEYBOARD()
        keyboard4x3_proccess();
#endif

        pump_proccess();

        UI::proccess();

#if IS_DEVICE_WITH_4PIN()
        indicate_proccess();
#endif
    }
}

int _write(int, uint8_t *ptr, int len) {
	(void)ptr;
	(void)len;
#ifdef DEBUG
    HAL_UART_Transmit(&BEDUG_UART, (uint8_t *)ptr, static_cast<uint16_t>(len), GENERAL_TIMEOUT_MS);
    for (int DataIdx = 0; DataIdx < len; DataIdx++) {
        ITM_SendChar(*ptr++);
    }
    return len;
#endif
    return 0;
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
    b_assert(__FILE__, __LINE__, "The error handler has been called");
	SOUL_STATUS err = has_errors() ? (SOUL_STATUS)get_first_error() : ERROR_HANDLER_CALLED;
	system_error_handler(err, error_loop);
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	b_assert((char*)file, line, "Wrong parameters value");
	SOUL_STATUS err = has_errors() ? (SOUL_STATUS)get_first_error() : ASSERT_ERROR;
	system_error_handler(err, error_loop);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
