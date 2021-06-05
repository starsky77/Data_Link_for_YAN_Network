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
#define BIT_BUFFER_SZIE 64
//#define ZC_BUFFER_SIZE 40

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
	//ZC:
	double timeInterval;
	uint32_t interval;
	int8_t lastCrossingPos;
//	uint32_t ZCBufCount;
//	uint8_t zeroCrossingBuf[ZC_BUFFER_SIZE];
public:
	Demodulator(uint32_t samplePointsNumber, uint32_t Frequency,
			float threshold) :
			maxSamplePoint(samplePointsNumber), sampleFrequency(Frequency), FcomponentThreshold(
					threshold) {
		sampleBufferCount = 0;
		bitBufferCount = 0;
//		ZCBufCount=0;
		interval=0;
		timeInterval=0.0;
		lastCrossingPos=-1;
//		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
//			sampleBuffer[i] = 0;
//		}
		for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
			bitBuffer[i] = 0;
			//bitBuffer_inpure[i] = 0;
		}
//		for (int i = 0; i < ZC_BUFFER_SIZE; i++) {
//			zeroCrossingBuf[i] = 0;
//		}
	}
	~Demodulator(){ };


	//返回在一定时段的采样值中是否出现过零
	//应当按照19200hz(0.52us)为一段来考虑(每次定时器中断发送时使用该函数)
	//过零发生时返回值为1，同时更新interval
//	int ZeroCrossing(uint32_t *sampleInput)
//	{
//		static uint_8 fir=1;
//		if(ZCBufCount>=ZC_BUFFER_SIZE-1)
//			return -1;
//		for(int i=1;i<maxSamplePoint;i++)
//		{
//			if(sampleInput[i-1]<=2048 && sampleInput[i]>2048)
//			{
//				zeroCrossingBuf[ZCBufCount]=1;
//				if(!fir)
//				{
//					interval=ZCBufCount-lastCrossingPos;
//					//us为单位
//					timeInterval=1.0*interval*1e6/(Frequency/samplePointsNumber);
//					//waiting to be tested
//					//过零后清空buff，认为上次过零发生在-1位置
//					lastCrossingPos=-1;
//					for (int i = 0; i < ZC_BUFFER_SIZE; i++) {
//						zeroCrossingBuf[i] = 0;
//					}
//					ZCBufCount=0;
//					return 1;
//				}
//				else
//				{
//					fir=0;
//				}
//				lastCrossingPos=ZCBufCount;
//				ZCBufCount++;
//				return 1;
//			}
//		}
//		zeroCrossingBuf[ZCBufCount]=0;
//		ZCBufCount++;
//		return 0;
//	}

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
	int FFTDemod(uint16_t *sampleInput) {

//		char info[32] = "Call FFT!\r\n";
//		HAL_UART_Transmit(&huart1, (uint8_t*) info, strlen(info),0xffff);

		//HAL_UART_Transmit_DMA(&huart1, (uint8_t*) info, strlen(info));

		//HAL_UART_Transmit(&huart1, (uint8_t*) info, strlen(info),0xffff);

//		for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++) {
//			sampleBuffer[i] = sampleInput[i];
//		}

		complex<uint16_t> sample[SAMPLE_BUFFER_SIZE];
		complex<uint16_t> result[SAMPLE_BUFFER_SIZE];


		//cannot use new in stm32051
		//	complex<float> *sample = new complex<float> [10];
		//	complex<float> *result = new complex<float> [10];



		for (int i = 0; i < maxSamplePoint; i++) {
			sample[i] = sampleInput[i];
		}


		int realSamplePoint=maxSamplePoint*16;

		discreteFourierFast(sample, maxSamplePoint, result,
				ftdFunctionToSpectrum);



		int index_1200 = 1200 * realSamplePoint / sampleFrequency;
		int index_2200 = 2200 * realSamplePoint / sampleFrequency;

		int index_800 = 800 * realSamplePoint / sampleFrequency;
		int index_1800 = 1800 * realSamplePoint / sampleFrequency;
		int index_2800 = 2800 * realSamplePoint / sampleFrequency;


		uint32_t Component_1200 = abs(result[1]);
		uint32_t Component_2200 = abs(result[2]);
		uint32_t Component_800 = abs(result[0]);
		uint32_t Component_1800 = abs(result[3]);

		//Debug
		//Problem 这一段不能在实际使用时出现，因为效率极低
		char c[40];
		sprintf(c, "Result:1200Hz:%d--2200Hz:%d\r\n", Component_1200,
				Component_2200);
		HAL_UART_Transmit(&huart1, (uint8_t*) c, strlen(c), 0xffff);

		sprintf(c, "Result:800Hz:%d--1800Hz:%d\r\n", Component_800,
				Component_1800);
		HAL_UART_Transmit(&huart1, (uint8_t*) c, strlen(c), 0xffff);




		//Preamble尚未完成，如果bitBuffer溢出就强制清空
		if (bitBufferCount > BIT_BUFFER_SZIE) {
			char error[32] = "Bit buffer has overflowed!\r\n";
			HAL_UART_Transmit_DMA(&huart1, (uint8_t*) error, strlen(error));
			bitBufferCount = 0;
			for (int i = 0; i < BIT_BUFFER_SZIE; i++) {
				bitBuffer[i] = 0;
				//bitBuffer_inpure[i] = 0;
			}
			return -1;
		}

		//两个频率分量都超阈值，记录为非纯部分，返回2，bitBuffer中按照0算
		if (Component_1200 > FcomponentThreshold
				&& Component_2200 > FcomponentThreshold) {
			//bitBuffer_inpure[bitBufferCount] = 1;
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
