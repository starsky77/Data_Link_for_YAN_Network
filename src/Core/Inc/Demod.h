/*
 * Demod.h
 *
 *  Created on: 2021年5月20日
 *      Author: wzq
 */

#ifndef INC_DEMOD_H_
#define INC_DEMOD_H_

#include "FastFouier.h"
#include <math.h>
#include <stdlib.h>

#define SAMPLE_BUFFER_SIZE 200
#define BIT_BUFFER_SZIE 200

class Demodulator {
private:
	const double FcomponentThreshold;
	uint32_t maxSamplePoint;
	uint32_t sampleBuffer[SAMPLE_BUFFER_SIZE];
	uint32_t sampleBufferCount;
	uint32_t bitBuffer[BIT_BUFFER_SZIE];
	uint32_t bitBuffer_inpure[BIT_BUFFER_SZIE];
	uint32_t bitBufferCount;
	uint32_t sampleFrequency;
public:
	Demodulator(uint32_t samplePointsNumber, uint32_t Frequency,
			uint32_t threshold) :
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
	~Demodulator();
	//采样点传入sampleBuffer中
	void sampleBufferInput(uint32_t samplePoint) {
		sampleBuffer[sampleBufferCount] = samplePoint;
		sampleBufferCount++;
		if (bitBufferCount == maxSamplePoint) {
			FFTDemod();
			for (int i = 0; i < bitBufferCount; i++) {
				sampleBuffer[i] = 0;
			}
			sampleBufferCount = 0;
		}
	}
	//每当采样点达到一定数量，对这一段进行FFT解调
	int FFTDemod() {
		complex<double> *sample = new complex<double> [SAMPLE_BUFFER_SIZE];
		complex<double> *result = new complex<double> [SAMPLE_BUFFER_SIZE];
		for (int i = 0; i < maxSamplePoint; i++) {
			sample[i] = sampleBuffer[i];
		}
		discreteFourierFast(sample, maxSamplePoint, result,
				ftdFunctionToSpectrum);
		int index_1200 = 1200 * maxSamplePoint / sampleFrequency;
		int index_2200 = 2200 * maxSamplePoint / sampleFrequency;

		uint32_t Component_1200 = result[index_1200].real();
		uint32_t Component_2200 = result[index_2200].real();
		//Debug
		char c[256];
		sprintf(c, "1200Hz:%d \r\n", Component_1200);
		HAL_UART_Transmit_DMA(&huart1, (uint8_t*) c, strlen(c));
		sprintf(c, "2200Hz:%d \r\n", Component_2200);
		HAL_UART_Transmit_DMA(&huart1, (uint8_t*) c, strlen(c));

		//Preamble尚未完成，如果bitBuffer溢出就强制清空
		if (bitBufferCount > BIT_BUFFER_SZIE) {
			c = "Bit buffer has overflowed!\r\n";
			HAL_UART_Transmit_DMA(&huart1, (uint8_t*) c, strlen(c));
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
			bitBuffer_inpure[bitBuffer] = 1;
			bitBuffer[bitBuffCount] = 0;
			bitBufferCount++;
			return 2;
		}

		if (Component_1200 > FcomponentThreshold) {
			bitBuffer[bitBuffCount] = 0;
			bitBufferCount++;
			return 0;
		} else if (Component_2200 > FcomponentThreshold) {
			bitBuffer[bitBuffCount] = 1;
			bitBufferCount++;
			return 1;
		}

	}

	//获取到充分的01交替编码时，认为获取到前导码，同步时序
	void Preamble();
};

#endif /* INC_DEMOD_H_ */
