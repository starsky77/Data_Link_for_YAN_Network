/*
 * AFSKGenerator.hpp
 *
 *  Created on: Apr 3, 2021
 *      Author: Guan
 */

#ifndef SRC_AFSKGENERATOR_
#define SRC_AFSKGENERATOR_

#include "main.h"
#include "CircularQueue.hpp"
#include <stdint.h>
#include <array>


class AFSK_Generator {
public:
	AFSK_Generator(): hdac(nullptr), Channel(0), dac_tim(nullptr),
		state(READY), req(NA), byte(0), mask(0x80) { }
	AFSK_Generator(const AFSK_Generator&) = delete;
	AFSK_Generator& operator=(const AFSK_Generator&) = delete;

	int init(DAC_HandleTypeDef *hdac_, uint32_t Channel_, TIM_HandleTypeDef *htim_);

	using Request_t = enum { NA, SEIZE, RELEASE };
	void request(Request_t req_) {
		req = req_;
	}
	auto requestTx(const uint8_t* buf, size_t len) {
		return q.fifo_put(buf, len);
	}


	bool dac_enabled() {
		return hdac->Instance->CR & (DAC_CR_EN1 << Channel) != 0;
	}
	uint32_t cur_val() { return HAL_DAC_GetValue(hdac, Channel); }

	// invoked in the middle of every slot
	void update();

	~AFSK_Generator() {
		if (HAL_DAC_Stop_DMA(hdac, Channel) != HAL_OK) Error_Handler();
	}

private:
	// HW handle operated on
	DAC_HandleTypeDef *hdac;
	uint32_t Channel;
	TIM_HandleTypeDef *dac_tim;
	// sine code table w/ val in range [0, 4096)
	static constexpr std::array<uint16_t, 256> sine12bit {
		2045, 2090, 2135, 2180, 2224, 2269, 2313, 2357, 2401, 2445,
		2488, 2531, 2574, 2617, 2659, 2701, 2743, 2784, 2825, 2865,
		2904, 2944, 2982, 3020, 3058, 3095, 3131, 3166, 3201, 3236,
		3269, 3302, 3334, 3365, 3396, 3425, 3454, 3482, 3509, 3535,
		3560, 3585, 3608, 3631, 3652, 3673, 3693, 3711, 3729, 3745,
		3761, 3776, 3789, 3801, 3813, 3823, 3832, 3841, 3848, 3854,
		3859, 3863, 3865, 3867, 3867, 3867, 3865, 3863, 3859, 3854,
		3848, 3841, 3832, 3823, 3813, 3801, 3789, 3776, 3761, 3745,
		3729, 3711, 3693, 3673, 3652, 3631, 3608, 3585, 3560, 3535,
		3509, 3482, 3454, 3425, 3396, 3365, 3334, 3302, 3269, 3236,
		3201, 3166, 3131, 3095, 3058, 3020, 2982, 2944, 2904, 2865,
		2825, 2784, 2743, 2701, 2659, 2617, 2574, 2531, 2488, 2445,
		2401, 2357, 2313, 2269, 2224, 2180, 2135, 2090, 2045, 2001,
		1956, 1911, 1867, 1822, 1778, 1734, 1690, 1646, 1603, 1560,
		1517, 1474, 1432, 1390, 1348, 1307, 1266, 1226, 1187, 1147,
		1109, 1071, 1033, 996, 960, 925, 890, 855, 822, 789,
		757, 726, 695, 666, 637, 609, 582, 556, 531, 506,
		483, 460, 439, 418, 398, 380, 362, 346, 330, 315,
		302, 290, 278, 268, 259, 250, 243, 237, 232, 228,
		226, 224, 223, 224, 226, 228, 232, 237, 243, 250,
		259, 268, 278, 290, 302, 315, 330, 346, 362, 380,
		398, 418, 439, 460, 483, 506, 531, 556, 582, 609,
		637, 666, 695, 726, 757, 789, 822, 855, 890, 925,
		960, 996, 1033, 1071, 1109, 1147, 1187, 1226, 1266, 1307,
		1348, 1390, 1432, 1474, 1517, 1560, 1603, 1646, 1690, 1734,
		1778, 1822, 1867, 1911, 1956, 2001,
	};
	// freq ctrl thru itvl between triggerings
	static constexpr uint16_t ARR[] {84, 155};
	// the same time slot for 2 symbols
	static constexpr uint16_t S[] {11 * sine12bit.size() / 6, sine12bit.size()};
	// state var
	using State_t = enum { READY, STARTUP, TXING };
	State_t state;
	Request_t req;
	// TXING
	uint8_t byte;
	uint8_t mask;
	CircularQueue<uint8_t, 255> q;

	void Tx_symbol(uint8_t symbol) {
		// config to Tx this symbol; assume preload enabled
		__HAL_TIM_SET_AUTORELOAD(dac_tim, ARR[symbol]);
	}

};

int AFSK_Generator::init(DAC_HandleTypeDef *hdac_, uint32_t Channel_, TIM_HandleTypeDef *htim_) {
	hdac = hdac_;
	Channel = Channel_;
	dac_tim = htim_;
	auto s1 = HAL_DAC_Start_DMA(hdac, Channel, (uint32_t *)sine12bit.begin(), sine12bit.size(), DAC_ALIGN_12B_R);
	__HAL_DAC_DISABLE(hdac, Channel);
	auto s2 = HAL_TIM_Base_Start(dac_tim);
	if (s1 == HAL_OK && s2 == HAL_OK) return 0;
	else return 1;
}

void AFSK_Generator::update() {
	static uint8_t symbol = 0, nb_1bit = 0, TxDelay = 0;
	// Moore FSM
	switch (state) {
		case TXING:
			// HDLC: bit stuffing
			if (nb_1bit == 5) {
				nb_1bit = 0;
				// HDLC: NRZI
				symbol = !symbol;
			}
			else {
				if (mask == 0x80) {
					// get next byte
					byte = q.front();
					q.pop();
				}
				mask = mask << 1 | mask >> 7; // rol
				// get next bit
				uint8_t bit = (byte & mask) != 0;
				if (bit) {
					nb_1bit++;
				}
				else {
					nb_1bit = 0;
					// HDLC: NRZI
					symbol = !symbol;
				}
			}
			// config to Tx this symbol; assume preload enabled
			Tx_symbol(symbol);
			break;
		case STARTUP:
			// Tx 0 for TxDelay slots
			symbol = !symbol;
			Tx_symbol(symbol);
			break;
		case READY:
			break;
		default:
			break;
	}
	// decide next state based on internals and requests
	switch (state) {
		case READY:
			if (req == SEIZE) {
				state = STARTUP;
				__HAL_DAC_ENABLE(hdac, Channel);
				TxDelay = 128;
			}
			req = NA;
			break;
		case STARTUP:
			if (--TxDelay == 0) {
				if (q.empty()) {
					state = READY;
					__HAL_DAC_DISABLE(hdac, Channel);
				}
				else {
					state = TXING;
				}
			}
			break;
		case TXING:
			// Tx cplt
			if (mask == 0x80 && q.empty()) {
				state = READY;
				__HAL_DAC_DISABLE(hdac, Channel);
			}
			break;
		default:
			break;
	}
}

#endif /* SRC_AFSKGENERATOR_ */
