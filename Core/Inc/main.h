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
#include <stdbool.h>

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */


#define DEVICE_MODE_4PIN_KEYBOARD  (1)
#define DEVICE_MODE_4PIN           (DEVICE_MODE_4PIN_KEYBOARD + 1)
#define DEVICE_MODE_16PIN_KEYBOARD (DEVICE_MODE_4PIN + 1)
#define DEVICE_MODE_16PIN          (DEVICE_MODE_16PIN_KEYBOARD + 1)

#if defined(MODE4PIN) && !defined(MODEKEYBOARD)
#   warning "Selected 4-pin device mode without keyboard"
#   define DEVICE_MODE                (DEVICE_MODE_4PIN)
#elif defined(MODE4PIN) && defined(MODEKEYBOARD)
#   warning "Selected 4-pin device mode with keyboard"
#   define DEVICE_MODE                (DEVICE_MODE_4PIN_KEYBOARD)
#elif defined(MODE16PIN) && !defined(MODEKEYBOARD)
#   warning "Selected 16-pin device mode without keyboard"
#   define DEVICE_MODE                (DEVICE_MODE_16PIN)
#elif defined(MODE16PIN) && defined(MODEKEYBOARD)
#   warning "Selected 16-pin device mode with keyboard"
#   define DEVICE_MODE                (DEVICE_MODE_16PIN_KEYBOARD)
#else
#   warning "Selected default device mode"
#   define DEVICE_MODE                (DEVICE_MODE_4PIN_KEYBOARD)
#endif

#ifndef IS_DEVICE_WITH_4PIN
#   define IS_DEVICE_WITH_4PIN()      (DEVICE_MODE == DEVICE_MODE_4PIN_KEYBOARD || DEVICE_MODE == DEVICE_MODE_4PIN)
#endif

#ifndef IS_DEVICE_WITH_16PIN
#   define IS_DEVICE_WITH_16PIN()     (DEVICE_MODE == DEVICE_MODE_16PIN_KEYBOARD || DEVICE_MODE == DEVICE_MODE_16PIN)
#endif

#ifndef IS_DEVICE_WITH_KEYBOARD
#   define IS_DEVICE_WITH_KEYBOARD()  (DEVICE_MODE == DEVICE_MODE_4PIN_KEYBOARD || DEVICE_MODE == DEVICE_MODE_16PIN_KEYBOARD)
#endif


#if !(IS_DEVICE_WITH_4PIN() || IS_DEVICE_WITH_16PIN())
#   error "unknown device mode"
#endif

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

int _write(int file, uint8_t *ptr, int len);

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define VALVE_SENS_Pin GPIO_PIN_0
#define VALVE_SENS_GPIO_Port GPIOC
#define PUMP_SENS_Pin GPIO_PIN_1
#define PUMP_SENS_GPIO_Port GPIOC
#define VALVE1_Pin GPIO_PIN_2
#define VALVE1_GPIO_Port GPIOC
#define VALVE2_Pin GPIO_PIN_3
#define VALVE2_GPIO_Port GPIOC
#define PUMP_Pin GPIO_PIN_0
#define PUMP_GPIO_Port GPIOA
#define BEDUG_TX_Pin GPIO_PIN_2
#define BEDUG_TX_GPIO_Port GPIOA
#define BEDUG_RX_Pin GPIO_PIN_3
#define BEDUG_RX_GPIO_Port GPIOA
#define TM1637_DATA_Pin GPIO_PIN_4
#define TM1637_DATA_GPIO_Port GPIOA
#define TM1637_CLK_Pin GPIO_PIN_5
#define TM1637_CLK_GPIO_Port GPIOA
#define ENC_B_Pin GPIO_PIN_6
#define ENC_B_GPIO_Port GPIOC
#define ENC_A_Pin GPIO_PIN_7
#define ENC_A_GPIO_Port GPIOC
#define PUMP_START_Pin GPIO_PIN_8
#define PUMP_START_GPIO_Port GPIOC
#define PUMP_STOP_Pin GPIO_PIN_9
#define PUMP_STOP_GPIO_Port GPIOC
#define GUN_SWITCH_Pin GPIO_PIN_8
#define GUN_SWITCH_GPIO_Port GPIOA
#define MODBUS_TX_Pin GPIO_PIN_9
#define MODBUS_TX_GPIO_Port GPIOA
#define MODBUS_RX_Pin GPIO_PIN_10
#define MODBUS_RX_GPIO_Port GPIOA
#define RFID_D1_Pin GPIO_PIN_11
#define RFID_D1_GPIO_Port GPIOA
#define RFID_D1_EXTI_IRQn EXTI15_10_IRQn
#define RFID_D0_Pin GPIO_PIN_12
#define RFID_D0_GPIO_Port GPIOA
#define RFID_D0_EXTI_IRQn EXTI15_10_IRQn
#define RFID_LED_Pin GPIO_PIN_15
#define RFID_LED_GPIO_Port GPIOA
#define KBD_ROW1_Pin GPIO_PIN_10
#define KBD_ROW1_GPIO_Port GPIOC
#define KBD_ROW2_Pin GPIO_PIN_11
#define KBD_ROW2_GPIO_Port GPIOC
#define KBD_ROW3_Pin GPIO_PIN_12
#define KBD_ROW3_GPIO_Port GPIOC
#define KBD_ROW4_Pin GPIO_PIN_2
#define KBD_ROW4_GPIO_Port GPIOD
#define KBD_COL1_Pin GPIO_PIN_4
#define KBD_COL1_GPIO_Port GPIOB
#define KBD_COL2_Pin GPIO_PIN_5
#define KBD_COL2_GPIO_Port GPIOB
#define EEPROM_SCL_Pin GPIO_PIN_6
#define EEPROM_SCL_GPIO_Port GPIOB
#define EEPROM_SDA_Pin GPIO_PIN_7
#define EEPROM_SDA_GPIO_Port GPIOB
#define KBD_COL3_Pin GPIO_PIN_8
#define KBD_COL3_GPIO_Port GPIOB
#define KBD_BL_Pin GPIO_PIN_9
#define KBD_BL_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#if IS_DEVICE_WITH_16PIN()
#   define DIGITS_1_Pin GPIO_PIN_4
#   define DIGITS_1_GPIO_Port GPIOA
#   define DIGITS_2_Pin GPIO_PIN_5
#   define DIGITS_2_GPIO_Port GPIOA
#   define DIGITS_4_Pin GPIO_PIN_6
#   define DIGITS_4_GPIO_Port GPIOA
#   define DIGITS_3_Pin GPIO_PIN_7
#   define DIGITS_3_GPIO_Port GPIOA
#   define DIGITS_5_Pin GPIO_PIN_4
#   define DIGITS_5_GPIO_Port GPIOC
#   define DIGITS_6_Pin GPIO_PIN_5
#   define DIGITS_6_GPIO_Port GPIOC
#   define DIGITS_D_Pin GPIO_PIN_0
#   define DIGITS_D_GPIO_Port GPIOB
#   define DIGITS_E_Pin GPIO_PIN_1
#   define DIGITS_E_GPIO_Port GPIOB
#   define DIGITS_C_Pin GPIO_PIN_2
#   define DIGITS_C_GPIO_Port GPIOB
#   define DIGITS_G_Pin GPIO_PIN_10
#   define DIGITS_G_GPIO_Port GPIOB
#   define DIGITS_DP_Pin GPIO_PIN_12
#   define DIGITS_DP_GPIO_Port GPIOB
#   define DIGITS_F_Pin GPIO_PIN_13
#   define DIGITS_F_GPIO_Port GPIOB
#   define DIGITS_A_Pin GPIO_PIN_14
#   define DIGITS_A_GPIO_Port GPIOB
#   define DIGITS_B_Pin GPIO_PIN_15
#   define DIGITS_B_GPIO_Port GPIOB
#endif

// General settings
#define GENERAL_TIMEOUT_MS       ((uint32_t)100)
#define GENERAL_SESSION_ML_MIN   ((uint32_t)2000)
#define GENERAL_MODBUS_SLAVE_ID  ((uint8_t)0x01)

// Defines
#define ML_IN_LTR                ((uint32_t)1000)

// MODBUS slave
extern UART_HandleTypeDef        huart1;
#define MODBUS_UART              (huart1)

// EEPROM
extern I2C_HandleTypeDef         hi2c1;
#define EEPROM_I2C               (hi2c1)

// PUMP
extern TIM_HandleTypeDef         htim3;
#define MD212_TIM                (htim3)
extern ADC_HandleTypeDef         hadc1;
#define PUMP_ADC                 (hadc1)
#define PUMP_ADC_CHANNEL         ((uint32_t)11)
extern ADC_HandleTypeDef         hadc1;
#define VALVE_ADC                (hadc1)
#define VALVE_ADC_CHANNEL        ((uint32_t)10)

// Power
#define POWER_ADC                (hadc1)

// Clock
extern RTC_HandleTypeDef         hrtc;
#define CLOCK_RTC                (hrtc)

// BEDUG UART
extern UART_HandleTypeDef        huart2;
#define BEDUG_UART               (huart2)

// Indicators
extern TIM_HandleTypeDef         htim4;
#define INDICATORS_TIM           (htim4)

// Watchdog
extern IWDG_HandleTypeDef        hiwdg;
#define DEVICE_IWDG              (hiwdg)

// UI
extern TIM_HandleTypeDef         htim5;
#define UI_TIM                   (htim5)


void save_new_log(uint32_t mlCount);


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
