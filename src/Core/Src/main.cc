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
#include "dma.h"
#include "tim.h"
#include "gpio.h"

#include "Demod.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
#define ADC_BUFF_SIZE 32
#define FFT_SAMPLE_SIZE 256

Demodulator demod(ADC_BUFF_SIZE,FFT_SAMPLE_SIZE,40800);

int ADC2_Value[1];
int ADC_buffer[ADC_BUFF_SIZE];
int ADC_bufferCount=0;
//uint32_t ADC_Buffer[FFT_SAMPLE_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void simpleLEDtest()
{
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	HAL_Delay(10);
}

void UartTestOutput()
{
	char c[32];
	sprintf(c,"%s\r\n", "Test run111");
	HAL_UART_Transmit(&huart2, (uint8_t *)c, strlen(c),0xffff);
}


int timeCount=0;

int ADC_store_count=0;
int FFT_flag=0;
int FFT_count=0;
int FFT_waitting_count=0;


int timeRecord1=0;
int timeRecord2=0;

//demod 10次fft一输出
//adc 积攒10次一次传输(10次采样-1次fft)
//adc每次fft时会停止采样500轮次
//定时器中断频率19200hz

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim == &htim2) {
//		if(demod.bufferFullFlag==0)
//		{
//			HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC2_Value, 1);
//		}
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
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start_IT(&htim4);
  HAL_TIM_Base_Start(&htim4);
//  HAL_ADCEx_Calibration_Start(&hadc1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int endFlag=0;
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)ADC2_Value, 1);
  while (1)
  {
    /* USER CODE END WHILE */
	 simpleLEDtest();
	 if(endFlag)
		 continue;
	 if(demod.bufferFullFlag==1)
	 {
		char c[40];
		sprintf(c, "FFT result:\r\n");
		HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
		for(int i=0;i<20;i++)
		{
			sprintf(c, "Result%d:Maxvalue:%d,MaxIndex:%d\r\n", i,(int)demod.resultBuffer[i*2],demod.resultBuffer[i*2+1]);
			HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);

//			sprintf(c, "Result%d:index0:%d--index1:%d\r\n",i, demod.resultBuffer[i*6],
//					demod.resultBuffer[i*6+1]);
//			HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
//			sprintf(c, "Result%d:index2:%d--index3:%d\r\n",i, demod.resultBuffer[i*6+2],
//					demod.resultBuffer[i*6+3]);
//			HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
//			sprintf(c, "Result%d:index4:%d--index5:%d\r\n\r\n",i, demod.resultBuffer[i*6+4],
//					demod.resultBuffer[i*6+5]);
//			HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
		}
		endFlag=1;
	 }
//	 UartTestOutput();

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 192;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_PLLCLK, RCC_MCODIV_1);
  HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_PLLCLK, RCC_MCODIV_1);
}
/* USER CODE BEGIN 4 */



void TestADC() {
	for (int i = 0; i < ADC_BUFF_SIZE; i++) {
		char c[32];
		sprintf(c, "value%d:%d\r\n", i, ADC2_Value[i]);
		HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
		//高频率下用IT以及DMA在显示上会出现问题
		//HAL_UART_Transmit_IT(&huart2, (uint8_t *)c, strlen(c));
	}

	for (int i = 0; i < ADC_BUFF_SIZE; i++) {
		ADC2_Value[i] = 0;
	}

}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{

//	UartTestOutput();
//	demod.FFTDemod(ADC2_Value);
//
//	for (int i = 0; i < ADC_BUFF_SIZE; i++) {
//		ADC2_Value[i] = 0;
//	}

//	TestADC();


//	timeRecord1=SystemTimer();
	//减少采样点的数量：32个有效采样点
//	for(int i=0;i<64;i++)
//	{
//		ADC2_Value[i]=(ADC2_Value[i*8]-699)*5;
//	}

	ADC_buffer[ADC_bufferCount]=(ADC2_Value[0]-699)*5;
	ADC_bufferCount++;

	if(ADC_bufferCount>=32)
	{
		demod.DSPFFTDemod(ADC_buffer);
		ADC_bufferCount=0;
	}

//	for(int i=0;i<64;i++)
//	{
//		ADC2_Value[i]=0;
//	}

//	timeRecord2=SystemTimer();
//	int timepass=timeRecord2-timeRecord1;
//
//	char c[32];
//	sprintf(c, "time cost %d us\r\n", timepass);
//	HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);



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
