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

#include "log.h"
#include "utils.h"
#include "clock.h"
#include "wiegand.h"
#include "settings.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"
#include "eeprom_at24cm01_storage.h"

#include "UI.h"
#include "Pump.h"
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

static constexpr char MAIN_TAG[] = "MAIN";

uint8_t modbus_uart_byte = 0;

extern settings_t settings;

StorageDriver storageDriver;
StorageAT storage(
	eeprom_get_size() / Page::PAGE_SIZE,
	&storageDriver
);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void save_new_log(uint32_t mlCount);

void record_check();

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
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
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

  set_status(WAIT_LOAD);

  HAL_Delay(100);

  gprint("\n\n\n");
  printTagLog(MAIN_TAG, "The device is loading");

  // Indicators timer start
  HAL_TIM_Base_Start_IT(&INDICATORS_TIM);

  // UI timer start
  HAL_TIM_Base_Start_IT(&UI_TIM);

  // Gas sensor encoder
  HAL_TIM_Encoder_Start(&MD212_TIM, TIM_CHANNEL_ALL);

  // MODBUS slave initialization
  HAL_UART_Receive_IT(&MODBUS_UART, (uint8_t*)&modbus_uart_byte, 1);

  printTagLog(MAIN_TAG, "The device is loaded successfully");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

	while (is_status(WAIT_LOAD)) soulGuard.defend();

	Record::showMax();

	if (RecordTmp::exists()) {
		RecordTmp::restore();
	}

	while (1)
	{

		soulGuard.defend();

		Pump::measure();

		if (has_errors()) {
			continue;
		}

		HAL_IWDG_Refresh(&DEVICE_IWDG);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		settings_check_residues();

		record_check();

		mbManager.tick();
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
		save_new_log(UI::getResultMl());
		RecordTmp::remove();
		UI::resetResultMl();
		resultMlBuf = 0;
		reset_status(NEED_SAVE_FINAL_RECORD);
	}

	if (is_status(NEED_INIT_RECORD_TMP)) {
		reset_status(NEED_INIT_RECORD_TMP);
		RecordTmp::init();
	}

	if (__abs_dif(resultMl, resultMlBuf) > RecordTmp::TRIG_LEVEL_ML) {
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

    set_status(WAIT_LOAD);

    Record record(0);

    record.record.time     = clock_get_timestamp();
    record.record.used_mls = mlCount;
    record.record.card     = UI::getCard();

    printTagLog(MAIN_TAG, "save new log: begin");
    printTagLog(MAIN_TAG, "save new log: real mls=%lu", Pump::getCurrentMl());

    RecordStatus status = record.save();
    if (status != RECORD_OK) {
        printTagLog(MAIN_TAG, "save new log: error=%02x", status);
    } else {
    	printTagLog(MAIN_TAG, "save new log: success");
    }

	printTagLog(MAIN_TAG, "adding %lu used milliliters for %lu card", record.record.used_mls, record.record.card);
    settings_add_used_liters(record.record.used_mls, record.record.card);
    set_settings_update_status(true);

    reset_status(WAIT_LOAD);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == MODBUS_UART.Instance) {
        mbManager.recieveByte(modbus_uart_byte);
        HAL_UART_Receive_IT(&MODBUS_UART, (uint8_t*)&modbus_uart_byte, 1);
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

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if(htim->Instance == INDICATORS_TIM.Instance)
    {
        indicate_proccess();
    } else if (htim->Instance == UI_TIM.Instance) {
        keyboard4x3_proccess();

        Pump::tick();

        UI::proccess();
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
	set_error(INTERNAL_ERROR);
	while (1);
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
	set_error(INTERNAL_ERROR);
	while (1);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
