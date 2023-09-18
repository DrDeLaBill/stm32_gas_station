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
#include "rtc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>
#include <string.h>

#include "soul.h"
#include "utils.h"
#include "clock.h"
#include "wiegand.h"
#include "ui_manager.h"
#include "pump_manager.h"
#include "record_manager.h"
#include "modbus_manager.h"
#include "settings_manager.h"

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

const char* MAIN_TAG = "MAIN";

uint8_t umka200_uart_byte = 0;

uint8_t modbus_uart_byte = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void umka200_data_sender(uint8_t* data, uint16_t len);
void pump_stop_handler();

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
  /* USER CODE BEGIN 2 */

  // PUMP initialization
  pump_set_pump_stop_handler(&pump_stop_handler);

  // MODBUS slave initialization
  HAL_UART_Receive_IT(&MODBUS_UART, (uint8_t*)&modbus_uart_byte, 1);


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    soul_proccess();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	ui_proccess();

    if (!settings_loaded()) {
    	settings_load();
    	continue;
    }

	pump_proccess();

    if (general_check_errors()) {
    	continue;
    }

	modbus_manager_proccess();

	record_cache_records_proccess();

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void pump_stop_handler()
{
	log_record_t record = {
		.cf_id = settings.cf_id,
		.time  = {
			clock_get_year(),
			clock_get_month(),
			clock_get_date(),
			clock_get_hour(),
			clock_get_minute(),
			clock_get_second()
		},
		.used_liters = pump_get_fuel_count_ml(),
		.card        = device_info.user_card,
		.id          = 0
	};

	LOG_TAG_BEDUG(MAIN_TAG, "save new log: begin");
	LOG_TAG_BEDUG(MAIN_TAG, "save new log: time=20%02u-%02u-%02u %02u:%02u:%02u", record.time[0], record.time[1], record.time[2], record.time[3], record.time[4], record.time[5]);
	LOG_TAG_BEDUG(MAIN_TAG, "save new log: cf_id=%lu", record.cf_id);
	LOG_TAG_BEDUG(MAIN_TAG, "save new log: card=%lu", record.card);
	LOG_TAG_BEDUG(MAIN_TAG, "save new log: used_liters=%lu", record.used_liters);

	uint32_t new_id = 0;
	record_status_t status = record_get_new_id(&new_id);
	if (status != RECORD_OK) {
		LOG_TAG_BEDUG(MAIN_TAG, "save new log: find new log id error=%02x", status);
		return;
	}

	record.id = new_id;

	LOG_TAG_BEDUG(MAIN_TAG, "save new log: id=%lu", record.id);

	memcpy((uint8_t*)&log_record, (uint8_t*)&record, sizeof(log_record));

	status = record_save();
	if (status != RECORD_OK) {
		LOG_TAG_BEDUG(MAIN_TAG, "save new log: error=%02x", status);
		return;
	}

	LOG_TAG_BEDUG(MAIN_TAG, "save new log: success");
}

bool general_check_errors()
{
	if (pump_has_error()) {
		return true;
	}
	return false;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == MODBUS_UART.Instance) {
    	modbus_manager_recieve_data_byte(modbus_uart_byte);
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

int _write(int file, uint8_t *ptr, int len) {
#ifdef DEBUG
    HAL_UART_Transmit(&BEDUG_UART, (uint8_t *)ptr, len, GENERAL_BUS_TIMEOUT_MS);
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
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  NVIC_SystemReset();
  while (1)
  {
  }
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
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
