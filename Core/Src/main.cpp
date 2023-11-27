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

#include "soul.h"
#include "utils.h"
#include "clock.h"
#include "wiegand.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"
#include "eeprom_at24cm01_storage.h"

#include "UI.h"
#include "Pump.h"
#include "Access.h"
#include "RecordDB.h"
#include "StorageAT.h"
#include "SettingsDB.h"
#include "ModbusManager.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
class StorageDriver: public IStorageDriver
{
public:
    StorageDriver() {}
    StorageStatus read(uint32_t address, uint8_t* data, uint32_t len) override;
    StorageStatus write(uint32_t address, uint8_t* data, uint32_t len) override;
};
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

const char* MAIN_TAG = "MAIN";

uint8_t umka200_uart_byte = 0;

uint8_t modbus_uart_byte = 0;

SettingsDB settings;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

void save_new_log(uint32_t mlCount);

void reset_eeprom_i2c();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
StorageAT storage(
    eeprom_get_size() / Page::PAGE_SIZE,
    (new StorageDriver())
);

ModbusManager mbManager(&MODBUS_UART);
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
  reset_eeprom_i2c();
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
//  MX_IWDG_Init();
  MX_TIM5_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(100);

  PRINT_MESSAGE(MAIN_TAG, "The device is loading\n");

  // Indicators timer start
  HAL_TIM_Base_Start_IT(&INDICATORS_TIM);

  // UI timer start
  HAL_TIM_Base_Start_IT(&UI_TIM);

  // IWDG check reboot
  if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
      LOG_TAG_BEDUG(MAIN_TAG, "IWDG just went off");
      LOG_TAG_BEDUG(MAIN_TAG, "REBOOT DEVICE");
      UI::setReboot();
      HAL_Delay(2500);
  }
  UI::resetReboot();

  // Gas sensor encoder
  HAL_TIM_Encoder_Start(&MD212_TIM, TIM_CHANNEL_ALL);

  // MODBUS slave initialization
  HAL_UART_Receive_IT(&MODBUS_UART, (uint8_t*)&modbus_uart_byte, 1);

  // Settings
  while (settings.load() != SettingsDB::SETTINGS_OK) {
      settings.reset();
  }

  PRINT_MESSAGE(MAIN_TAG, "The device is loaded successfully\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
//    HAL_IWDG_Refresh(&DEVICE_IWDG);

    soul_proccess();

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    Pump::measure();

    if (general_check_errors()) {
        continue;
    }

    settings.checkResidues();

    if (Pump::getLastMl() > 0) {
        save_new_log(Pump::getLastMl());
        Pump::setLastMl(0);
    }

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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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

void save_new_log(uint32_t mlCount)
{
    if (mlCount == 0) {
        return;
    }

    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    if (!clock_get_rtc_date(&date)) {
        memset(reinterpret_cast<void*>(&date), 0, sizeof(date));
    }
    if (!clock_get_rtc_time(&time)) {
        memset(reinterpret_cast<void*>(&time), 0, sizeof(time));
    }
    uint32_t datetimeSeconds = datetime_to_seconds(&date, &time);


    UI::setLoad();

    RecordDB record(0);

    record.record.time = datetimeSeconds;
    record.record.used_liters = mlCount;
    record.record.card = UI::getCard();

    LOG_TAG_BEDUG(MAIN_TAG, "save new log: begin");

    RecordDB::RecordStatus status = record.save();
    if (status != RecordDB::RECORD_OK) {
        LOG_TAG_BEDUG(MAIN_TAG, "save new log: error=%02x", status);
    } else {
    	LOG_TAG_BEDUG(MAIN_TAG, "save new log: success");
    }

	LOG_TAG_BEDUG(MAIN_TAG, "adding %lu used milliliters for %lu card", record.record.used_liters, record.record.card);
    settings.add_used_liters(record.record.used_liters, record.record.card);
    settings.save();

    UI::resetLoad();
}

void reset_eeprom_i2c()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin   = EEPROM_SDA_Pin | EEPROM_SCL_Pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(EEPROM_SDA_GPIO_Port, EEPROM_SDA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EEPROM_SCL_GPIO_Port, EEPROM_SCL_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);

    HAL_GPIO_WritePin(EEPROM_SDA_GPIO_Port, EEPROM_SDA_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(EEPROM_SCL_GPIO_Port, EEPROM_SCL_Pin, GPIO_PIN_SET);
    HAL_Delay(100);

    HAL_GPIO_WritePin(EEPROM_SDA_GPIO_Port, EEPROM_SDA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(EEPROM_SCL_GPIO_Port, EEPROM_SCL_Pin, GPIO_PIN_RESET);
    HAL_Delay(100);
}

bool general_check_errors()
{
    if (Pump::hasError()) {
        return true;
    }
    return false;
}

StorageStatus StorageDriver::read(uint32_t address, uint8_t* data, uint32_t len)
{
    eeprom_status_t status = eeprom_read(address, data, len);
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
};

StorageStatus StorageDriver::write(uint32_t address, uint8_t* data, uint32_t len)
{
    eeprom_status_t status = eeprom_write(address, data, len);
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }
    return STORAGE_OK;
};

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

        indicate_display();

    } else if (htim->Instance == UI_TIM.Instance) {
        keyboard4x3_proccess();

        Pump::tick();

        UI::UIProccess();
    }
}

int _write(int file, uint8_t *ptr, int len) {
    HAL_UART_Transmit(&BEDUG_UART, (uint8_t *)ptr, len, GENERAL_BUS_TIMEOUT_MS);
#ifdef DEBUG
    for (int DataIdx = 0; DataIdx < len; DataIdx++) {
        ITM_SendChar(*ptr++);
    }
#endif
    return len;
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
