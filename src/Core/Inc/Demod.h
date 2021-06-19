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
#define SAMPLE_BUFFER_SIZE 512
#define BIT_BUFFER_SZIE 65


extern TIM_HandleTypeDef htim4;
extern int timeCount;
uint32_t SystemTimer(void)//获取系统时间的函数
{
	return (htim4.Instance->CNT + timeCount * 65535)/1000;
	//系统时间=定时器当前计数值+65535*定时器中断次数
}

class Demodulator {
private:
	const float FcomponentThreshold;
	uint32_t maxSamplePoint;
//	uint32_t sampleBuffer[SAMPLE_BUFFER_SIZE];
	uint32_t sampleBufferCount;
	uint8_t bitBuffer[BIT_BUFFER_SZIE];
	//uint8_t bitBuffer_inpure[BIT_BUFFER_SZIE];
	uint32_t bitBufferCount;
	uint32_t sampleFrequency;



//	uint32_t ZCBufCount;
//	uint8_t zeroCrossingBuf[ZC_BUFFER_SIZE];
public:
	//DEBUG
	uint16_t resultBuffer[600];
	uint16_t debugBufferCount;
	int bufferFullFlag;

	Demodulator(uint32_t samplePointsNumber, uint32_t Frequency,
			float threshold) :
			maxSamplePoint(samplePointsNumber), sampleFrequency(Frequency), FcomponentThreshold(
					threshold) {
		sampleBufferCount = 0;
		bitBufferCount = 0;

		bufferFullFlag=0;
//		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
//			sampleBuffer[i] = 0;
//		}
		for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
			bitBuffer[i] = 0;
			//bitBuffer_inpure[i] = 0;
		}


		//DEBUG
		debugBufferCount=0;
	}
	~Demodulator(){ };



	//采样点传入sampleBuffer中
	//由于现在是一次性传入，不需要该函数了
//	void sampleBufferInput(uint32_t samplePoint) {
//		sampleBuffer[sampleBufferCount] = samplePoint;
//		sampleBufferCount++;
//		if (bitBufferCount == maxSamplePoint) {
//			FFTDemod();
//			for (int i = 0; i < bitBufferCount; i++) {
//				sampleBuffer[i] = 0;
//			}
//			sampleBufferCount = 0;
//		}
//	}


	//调用DSP库的FFT
	int DSPFFTDemod(uint16_t *sampleInput)
	{
		float_t fftData[SAMPLE_BUFFER_SIZE * 2];
		float_t fftOut[SAMPLE_BUFFER_SIZE];

		for (uint32_t i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
			fftData[2 * i] = sampleInput[i];
			fftData[2 * i + 1] = 0.0;
		}

		arm_cfft_f32(&arm_cfft_sR_f32_len512, fftData, 0, 1);
		arm_cmplx_mag_f32(fftData, fftOut, SAMPLE_BUFFER_SIZE);

		//注意fftOut的每个频率分量需要除以N/2才能得到真正结果，N为采样点个数
		uint16_t Component_0 = fftOut[0] / SAMPLE_BUFFER_SIZE;
		uint16_t Component_1 = fftOut[1] * 2 / SAMPLE_BUFFER_SIZE;
		uint16_t Component_2 = fftOut[2] * 2 / SAMPLE_BUFFER_SIZE;
		uint16_t Component_3 = fftOut[3] * 2 / SAMPLE_BUFFER_SIZE;
		uint16_t Component_4 = fftOut[4] * 2 / SAMPLE_BUFFER_SIZE;
		uint16_t Component_5 = fftOut[5] * 2 / SAMPLE_BUFFER_SIZE;


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
