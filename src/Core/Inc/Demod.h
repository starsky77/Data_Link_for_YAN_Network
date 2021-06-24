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


	int DSPFFTDemod(int32_t *sampleInput) {

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
		return max_index <= 11; // 1200 Hz -> MARK
	}

	//获取到充分的01交替编码时，认为获取到前导码，同步时序
	void Preamble();
};

#endif /* INC_DEMOD_H_ */
