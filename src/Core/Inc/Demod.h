/*
 * Demod.h
 *
 *  Created on: 2021年5月20日
 *      Author: wzq
 */

#ifndef INC_DEMOD_H_
#define INC_DEMOD_H_

#include "FastFouier.h"
#include "usart.h"
#include "main.h"
#include "arm_math.h"
#include "arm_const_structs.h"
#include <string.h>
#include <stdint.h>
#include <array>
#include <math.h>
#include <stdlib.h>

//this should be the same as dmaBuffer
#define SAMPLE_BUFFER_SIZE 32
#define BIT_BUFFER_SZIE 65
#define PI2 6.28318530717959


extern TIM_HandleTypeDef htim4;
extern int timeCount;
uint32_t SystemTimer(void)//获取系统时间的函数
{
	return (htim4.Instance->CNT + timeCount * 65535);
	//系统时间=定时器当前计数值+65535*定时器中断次数
}

#define ADC_DEBUG

class Demodulator {
private:
	uint32_t maxSamplePoint;
	uint32_t validSamplePoint;
	uint32_t sampleBufferCount;
	uint8_t bitBuffer[BIT_BUFFER_SZIE];
	uint32_t bitBufferCount;
	uint32_t sampleFrequency;

	float_t fftData[512];
	float_t fftOut[512];
	float_t fftResult[512];
	arm_rfft_fast_instance_f32 S;


public:
	//DEBUG
#ifdef ADC_DEBUG
	uint16_t resultBuffer[600];
	uint16_t debugBufferCount;
	int bufferFullFlag;
#endif

	Demodulator(uint32_t validSamplePointNum, uint32_t samplePointsNumber,
			uint32_t Frequency) :
			validSamplePoint(validSamplePointNum), maxSamplePoint(
					samplePointsNumber), sampleFrequency(Frequency) {
		sampleBufferCount = 0;
		bitBufferCount = 0;
#ifdef ADC_DEBUG
		bufferFullFlag = 0;
		debugBufferCount = 0;
#endif
		for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
			bitBuffer[i] = 0;
		}

		arm_rfft_fast_init_f32(&S, samplePointsNumber);

		//DEBUG
	}
	~Demodulator() {
	}
	;


	int DSPFFTDemod(int *sampleInput) {

		//cff
//		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
////			fftData[2 * i] = sampleInput[i];
//			//test 2200Hz
//			fftData[2 * i] = 2000*arm_sin_f32(PI2*i*2200.0/sampleFrequency);
//			fftData[2 * i + 1] = 0.0;
//		}
//		for(int i=SAMPLE_BUFFER_SIZE;i<realDataSize;i++)
//		{
//			fftData[2 * i] = 0.0;
//			fftData[2 * i + 1] = 0.0;
//		}

//rff
		uint32_t i;
		for (i = 0; i < validSamplePoint; i++) {
			fftData[i] = sampleInput[i];
			//test input
//			fftData[i] = 2000*arm_sin_f32(PI2*i*1200.0/sampleFrequency);
		}
		for (; i < maxSamplePoint; i++) {
			fftData[i] = 0.0;
		}

#ifdef TIMECOUNT
		uint32_t timeRecord1=SystemTimer();
#endif

		float32_t max_value = 0;
		uint32_t max_index = 0;

		arm_rfft_fast_f32(&S, fftData, fftOut, 0);
//		arm_cfft_f32(&arm_cfft_sR_f32_len512, fftData, 0, 1);
		arm_cmplx_mag_f32(fftOut, fftResult, maxSamplePoint);
		arm_max_f32(&fftResult[1], maxSamplePoint - 1, &max_value, &max_index);
		max_index++;

#ifdef TIMECOUNT
		uint32_t timeRecord2=SystemTimer();
		uint32_t timePass=timeRecord2-timeRecord1;

		char c[32];
		sprintf(c, "Time cost:%dus\r\n", timePass);
		HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
#endif

#ifdef ADC_DEBUG
		//debug
		if (debugBufferCount < 20) {

			resultBuffer[debugBufferCount * 2] = max_value;
			resultBuffer[debugBufferCount * 2 + 1] = max_index;
			debugBufferCount++;
		} else {
			bufferFullFlag = 1;
		}
#endif
		//通过测试：1200Hz的max_index为5-9，2200Hz的max_index为12-13
		if(max_index<=10&&max_index>4)
		{
			return 1;
		}
		else if(max_index>=11&&max_index<15)
		{
			return 0;
		}
	}



#ifdef MYFFT

	//每当采样点达到一定数量，对这一段进行FFT解调
	int FFTDemod(uint16_t *sampleInput) {

//		char info[32] = "Call FFT!\r\n";
//		HAL_UART_Transmit(&huart2, (uint8_t*) info, strlen(info),0xffff);

		//HAL_UART_Transmit_DMA(&huart2, (uint8_t*) info, strlen(info));

		//HAL_UART_Transmit(&huart2, (uint8_t*) info, strlen(info),0xffff);

//		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
//			sampleBuffer[i] = sampleInput[i];
//		}

		//SAMPLE_BUFFER_SIZE应当与有效采样点数量一致
		complex<uint16_t> sample[SAMPLE_BUFFER_SIZE];
		complex<uint16_t> result[SAMPLE_BUFFER_SIZE];


		//cannot use new in stm32051
		//	complex<float> *sample = new complex<float> [10];
		//	complex<float> *result = new complex<float> [10];



		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
			sample[i] = sampleInput[i];
		}

//		uint32_t timeRecord1=SystemTimer();

		discreteFourierFast(sample, SAMPLE_BUFFER_SIZE, result,
				ftdFunctionToSpectrum);





//		uint32_t timeRecord2=SystemTimer();
//		uint32_t timePass=timeRecord2-timeRecord1;
//		char c[32];
//		sprintf(c, "Time cost:%dms\r\n", timePass);
//		HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);

//		int index_1200 = 1200 * realSamplePoint / sampleFrequency;
//		int index_2200 = 2200 * realSamplePoint / sampleFrequency;
//
//		int index_800 = 800 * realSamplePoint / sampleFrequency;
//		int index_1800 = 1800 * realSamplePoint / sampleFrequency;
//		int index_2800 = 2800 * realSamplePoint / sampleFrequency;


		uint16_t Component_1 = abs(result[1]);
		uint16_t Component_2 = abs(result[2]);
		uint16_t Component_0 = abs(result[0]);
		uint16_t Component_3 = abs(result[3]);
		uint16_t Component_4 = abs(result[4]);
		uint16_t Component_5 = abs(result[5]);

		//Debug
		//Problem 这一段不能在实际使用时出现，因为效率极低

		if(debugBufferCount<20)
		{
			resultBuffer[debugBufferCount*6+0]=Component_0;
			resultBuffer[debugBufferCount*6+1]=Component_1;
			resultBuffer[debugBufferCount*6+2]=Component_2;
			resultBuffer[debugBufferCount*6+3]=Component_3;
			resultBuffer[debugBufferCount*6+4]=Component_4;
			resultBuffer[debugBufferCount*6+5]=Component_5;
			debugBufferCount++;
//			char c[32];
//			sprintf(c, "Test：%d\r\n",debugBufferCount);
//			HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
		}
		else
		{
			bufferFullFlag=1;
		}
//		else
//		{
//			char c[40];
//
//			for(int i=0;i<10;i++)
//			{
//				sprintf(c, "Result:index0:%d--index1:%d\r\n", resultBuffer[i*6],
//						resultBuffer[i*6+1]);
//				HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
//				sprintf(c, "Result:index2:%d--index3:%d\r\n", resultBuffer[i*6+2],
//						resultBuffer[i*6+3]);
//				HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
//				sprintf(c, "Result:index4:%d--index5:%d\r\n\r\n", resultBuffer[i*6+4],
//						resultBuffer[i*6+5]);
//				HAL_UART_Transmit(&huart2, (uint8_t*) c, strlen(c), 0xffff);
//			}
//
//
//			debugBufferCount=0;
//
//		}


		//Preamble尚未完成，如果bitBuffer溢出就强制清空
//		if (bitBufferCount > BIT_BUFFER_SZIE) {
//			char error[32] = "Bit buffer has overflowed!\r\n";
//			HAL_UART_Transmit_DMA(&huart2, (uint8_t*) error, strlen(error));
//			bitBufferCount = 0;
//			for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
//				bitBuffer[i] = 0;
//				//bitBuffer_inpure[i] = 0;
//			}
//			return -1;
//		}

		//两个频率分量都超阈值，记录为非纯部分，返回2，bitBuffer中按照0算
//		if (Component_1200 > FcomponentThreshold
//				&& Component_2200 > FcomponentThreshold) {
//			//bitBuffer_inpure[bitBufferCount] = 1;
//			bitBuffer[bitBufferCount] = 0;
//			bitBufferCount++;
//			return 2;
//		}
//
//		if (Component_1200 > FcomponentThreshold) {
//			bitBuffer[bitBufferCount] = 0;
//			bitBufferCount++;
//			return 0;
//		} else if (Component_2200 > FcomponentThreshold) {
//			bitBuffer[bitBufferCount] = 1;
//			bitBufferCount++;
//			return 1;
//		}
	}

#endif
	//获取到充分的01交替编码时，认为获取到前导码，同步时序
	void Preamble();
};

#endif /* INC_DEMOD_H_ */
