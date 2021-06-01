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
#include <string.h>
#include <stdint.h>
#include <array>
#include <math.h>
#include <stdlib.h>


//this should be the same as dmaBuffer
#define SAMPLE_BUFFER_SIZE 50
#define BIT_BUFFER_SZIE 200

class Demodulator {
private:
	const float FcomponentThreshold;
	uint32_t maxSamplePoint;
	uint32_t sampleBuffer[SAMPLE_BUFFER_SIZE];
	uint32_t sampleBufferCount;
	uint32_t bitBuffer[BIT_BUFFER_SZIE];
	uint32_t bitBuffer_inpure[BIT_BUFFER_SZIE];
	uint32_t bitBufferCount;
	uint32_t sampleFrequency;
public:
	Demodulator(uint32_t samplePointsNumber, uint32_t Frequency,
			float threshold) :
			maxSamplePoint(samplePointsNumber), sampleFrequency(Frequency), FcomponentThreshold(
					threshold) {
		sampleBufferCount = 0;
		bitBufferCount = 0;
		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
			sampleBuffer[i] = 0;
		}
		for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
			bitBuffer[i] = 0;
			bitBuffer_inpure[i] = 0;
		}
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
	//每当采样点达到一定数量，对这一段进行FFT解调



	int FFTDemod(uint32_t *sampleInput) {

//		char info[32] = "Call FFT!\r\n";
//		HAL_UART_Transmit(&huart1, (uint8_t*) info, strlen(info),0xffff);

		//HAL_UART_Transmit_DMA(&huart1, (uint8_t*) info, strlen(info));

		//HAL_UART_Transmit(&huart1, (uint8_t*) info, strlen(info),0xffff);

		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
			sampleBuffer[i] = sampleInput[i];
		}

		complex<float> sample[SAMPLE_BUFFER_SIZE];
		complex<float> result[SAMPLE_BUFFER_SIZE];


		//cannot use new in stm32051
		//	complex<float> *sample = new complex<float> [10];
		//	complex<float> *result = new complex<float> [10];



		for (int i = 0; i < maxSamplePoint; i++) {
			sample[i] = sampleBuffer[i];
		}


		discreteFourierFast(sample, maxSamplePoint, result,
				ftdFunctionToSpectrum);
		int index_1200 = 1200 * maxSamplePoint / sampleFrequency;
		int index_2200 = 2200 * maxSamplePoint / sampleFrequency;

		int index_800 = 800 * maxSamplePoint / sampleFrequency;
		int index_1800 = 1800 * maxSamplePoint / sampleFrequency;
		int index_2800 = 2800 * maxSamplePoint / sampleFrequency;


		uint32_t Component_1200 = abs(result[index_1200]);
		uint32_t Component_2200 = abs(result[index_2200]);
		uint32_t Component_800 = abs(result[index_800]);
		uint32_t Component_1800 = abs(result[index_1800]);

		//Debug
		//Problem 这一段不能在实际使用时出现，因为效率极低
		char c[64];
		sprintf(c, "Result:1200Hz:%d---2200Hz:%d\r\n", Component_1200,
				Component_2200);
		HAL_UART_Transmit(&huart1, (uint8_t*) c, strlen(c), 0xffff);

		sprintf(c, "Result:800Hz:%d---1800Hz:%d\r\n", Component_800,
				Component_1800);
		HAL_UART_Transmit(&huart1, (uint8_t*) c, strlen(c), 0xffff);




		//Preamble尚未完成，如果bitBuffer溢出就强制清空
		if (bitBufferCount > BIT_BUFFER_SZIE) {
			char error[256] = "Bit buffer has overflowed!\r\n";
			HAL_UART_Transmit_DMA(&huart1, (uint8_t*) error, strlen(error));
			bitBufferCount = 0;
			for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
				bitBuffer[i] = 0;
				bitBuffer_inpure[i] = 0;
			}
			return -1;
		}

		//两个频率分量都超阈值，记录为非纯部分，返回2，bitBuffer中按照0算
		if (Component_1200 > FcomponentThreshold
				&& Component_2200 > FcomponentThreshold) {
			bitBuffer_inpure[bitBufferCount] = 1;
			bitBuffer[bitBufferCount] = 0;
			bitBufferCount++;
			return 2;
		}

		if (Component_1200 > FcomponentThreshold) {
			bitBuffer[bitBufferCount] = 0;
			bitBufferCount++;
			return 0;
		} else if (Component_2200 > FcomponentThreshold) {
			bitBuffer[bitBufferCount] = 1;
			bitBufferCount++;
			return 1;
		}
	}
	//获取到充分的01交替编码时，认为获取到前导码，同步时序
	void Preamble();
};

#endif /* INC_DEMOD_H_ */
