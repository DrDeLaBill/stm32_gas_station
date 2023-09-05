/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdint.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DIGITS_A_Pin GPIO_PIN_0
#define DIGITS_A_GPIO_Port GPIOC
#define DIGITS_B_Pin GPIO_PIN_1
#define DIGITS_B_GPIO_Port GPIOC
#define DIGITS_C_Pin GPIO_PIN_2
#define DIGITS_C_GPIO_Port GPIOC
#define DIGITS_D_Pin GPIO_PIN_3
#define DIGITS_D_GPIO_Port GPIOC
#define MD212_ENC_A_Pin GPIO_PIN_0
#define MD212_ENC_A_GPIO_Port GPIOA
#define MD212_ENC_B_Pin GPIO_PIN_1
#define MD212_ENC_B_GPIO_Port GPIOA
#define MODBUS_TX_Pin GPIO_PIN_2
#define MODBUS_TX_GPIO_Port GPIOA
#define MODBUS_RX_Pin GPIO_PIN_3
#define MODBUS_RX_GPIO_Port GPIOA
#define VALVE1_Pin GPIO_PIN_4
#define VALVE1_GPIO_Port GPIOA
#define VALVE2_Pin GPIO_PIN_5
#define VALVE2_GPIO_Port GPIOA
#define PUMP_Pin GPIO_PIN_6
#define PUMP_GPIO_Port GPIOA
#define DIGITS_E_Pin GPIO_PIN_4
#define DIGITS_E_GPIO_Port GPIOC
#define DIGITS_F_Pin GPIO_PIN_5
#define DIGITS_F_GPIO_Port GPIOC
#define PUMP_CURRENT_Pin GPIO_PIN_0
#define PUMP_CURRENT_GPIO_Port GPIOB
#define VALVE_CURRENT_Pin GPIO_PIN_1
#define VALVE_CURRENT_GPIO_Port GPIOB
#define KBD_ROW3_Pin GPIO_PIN_10
#define KBD_ROW3_GPIO_Port GPIOB
#define KBD_ROW4_Pin GPIO_PIN_12
#define KBD_ROW4_GPIO_Port GPIOB
#define KBD_COL1_Pin GPIO_PIN_13
#define KBD_COL1_GPIO_Port GPIOB
#define KBD_COL2_Pin GPIO_PIN_14
#define KBD_COL2_GPIO_Port GPIOB
#define KBD_COL3_Pin GPIO_PIN_15
#define KBD_COL3_GPIO_Port GPIOB
#define DIGITS_G_Pin GPIO_PIN_6
#define DIGITS_G_GPIO_Port GPIOC
#define DIGITS_DP_Pin GPIO_PIN_7
#define DIGITS_DP_GPIO_Port GPIOC
#define DIGITS_1_Pin GPIO_PIN_8
#define DIGITS_1_GPIO_Port GPIOC
#define DIGITS_2_Pin GPIO_PIN_9
#define DIGITS_2_GPIO_Port GPIOC
#define RFID_TX_Pin GPIO_PIN_9
#define RFID_TX_GPIO_Port GPIOA
#define RFID_RX_Pin GPIO_PIN_10
#define RFID_RX_GPIO_Port GPIOA
#define BEDUG_TX_Pin GPIO_PIN_11
#define BEDUG_TX_GPIO_Port GPIOA
#define BEDUG_RX_Pin GPIO_PIN_12
#define BEDUG_RX_GPIO_Port GPIOA
#define DIGITS_3_Pin GPIO_PIN_10
#define DIGITS_3_GPIO_Port GPIOC
#define DIGITS_4_Pin GPIO_PIN_11
#define DIGITS_4_GPIO_Port GPIOC
#define DIGITS_5_Pin GPIO_PIN_12
#define DIGITS_5_GPIO_Port GPIOC
#define EEPROM_SCL_Pin GPIO_PIN_6
#define EEPROM_SCL_GPIO_Port GPIOB
#define EEPROM_SDA_Pin GPIO_PIN_7
#define EEPROM_SDA_GPIO_Port GPIOB
#define KBD_ROW1_Pin GPIO_PIN_8
#define KBD_ROW1_GPIO_Port GPIOB
#define KBD_ROW2_Pin GPIO_PIN_9
#define KBD_ROW2_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

// General settings
#define GENERAL_RFID_CARDS_COUNT ((uint8_t)10)
#define GENERAL_BUS_TIMEOUT_MS   ((uint32_t)1000)
#define GENERAL_SESSION_ML_MAX   ((uint32_t)50000)

// EEPROM
extern I2C_HandleTypeDef         hi2c1;
#define EEPROM_I2C               (hi2c1)

// UMKA200 (RFID)
extern UART_HandleTypeDef        huart1;
#define UMKA200_UART             (huart1)

// PUPMP
extern TIM_HandleTypeDef         htim2;
#define MD212_TIM                (htim2)
extern ADC_HandleTypeDef         hadc1;
#define PUMP_ADC                 (hadc1)
#define PUMP_ADC_CHANNEL         ((uint32_t)8)
extern ADC_HandleTypeDef         hadc1;
#define VALVE_ADC                (hadc1)
#define VALVE_ADC_CHANNEL        ((uint32_t)9)

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
