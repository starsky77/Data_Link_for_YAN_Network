/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <string.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "AFSKGenerator.hpp"
#include "KISSReceiver.hpp"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#ifdef __GNUC__
/* With GCC, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
AFSK_Generator gen;
KISS_Receiver rec; // for test

uint16_t ADC2_Value[1];
uint16_t ticks;


uint8_t rData[512];  //  for saving RX Data
uint8_t rDataBuffer[1];  //  RX Data buffer
uint8_t rDataCount = 0;  //  count Data bytes
uint8_t rDataFlag = 0;  //  waitting complete RX date having been send

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


PUTCHAR_PROTOTYPE
{
    HAL_UART_Transmit(&huart1 , (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim14) {
		gen.update();
	}
	if (htim == &htim2){
		// start DMA
		HAL_ADC_Start_DMA(&hadc, (uint32_t*)ADC2_Value, 1);
//		ticks++;
	}

}

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
  MX_DMA_Init();
  MX_DAC1_Init();
  MX_TIM6_Init();
  MX_TIM14_Init();
  MX_ADC_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  // sync ctrl
  constexpr uint16_t PW = 250;
  uint32_t led_tick = HAL_GetTick();
  // AFSK
  gen.init(&hdac1, DAC_CHANNEL_1, &htim6);
  gen.resume_gen();
  // KISS
  HAL_UART_Receive_DMA(&huart1, rDataBuffer, 1);
  // 1200 Hz Time Base
  HAL_TIM_Base_Start_IT(&htim14);
  HAL_TIM_Base_Start_IT(&htim2);
  HAL_ADCEx_Calibration_Start(&hadc);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // low freq pulse gen
    uint32_t t = HAL_GetTick();
    if (t >= led_tick + PW) {
      led_tick = t;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    }

    rec.handle_buffer();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

uint32_t last_adc, now_adc;
uint16_t last_zero, now_zero, interval_zero;

/* USER CODE BEGIN 4 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
	now_adc = ADC2_Value[0];
	if(now_adc>=2048 && last_adc<=2048||now_adc<=2048 && last_adc>=2048){
		now_zero = ticks; //HAL_GetTick();
		if(now_zero > last_zero)
			interval_zero = now_zero - last_zero;
		else
			interval_zero = now_zero - last_zero + 0x10000;
		last_zero = now_zero;

		char c[256];
		// time base is 50 us
		sprintf(c,"%d \r\n", interval_zero);
		HAL_UART_Transmit_DMA(&huart1, (uint8_t *)c, strlen(c));
	}
	last_adc = now_adc;

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  /* NOTE: This function should not be modified, when the callback is needed,
           the HAL_UART_RxCpltCallback could be implemented in the user file
   */
  rec.receive_byte(rDataBuffer[0]);
//  rec.receive_multi_bytes(rDataBuffer, 1);
  HAL_UART_Receive_DMA(&huart1, rDataBuffer, 1);
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
  while (1)
  {
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
