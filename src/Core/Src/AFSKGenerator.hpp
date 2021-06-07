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
		return (hdac->Instance->CR) & (DAC_CR_EN1 << Channel) != 0;
	}
	uint32_t cur_val() { return HAL_DAC_GetValue(hdac, Channel); }

	// invoked in the middle of every slot
	void update();
	friend struct UnitTest_DA;

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
		999, 1012, 1024, 1036, 1049, 1061, 1073, 1085, 1097, 1109,
		1121, 1133, 1145, 1156, 1168, 1179, 1191, 1202, 1213, 1224,
		1235, 1246, 1257, 1267, 1277, 1287, 1297, 1307, 1317, 1326,
		1335, 1344, 1353, 1362, 1370, 1378, 1386, 1394, 1401, 1408,
		1415, 1422, 1428, 1435, 1440, 1446, 1451, 1457, 1461, 1466,
		1470, 1474, 1478, 1481, 1485, 1487, 1490, 1492, 1494, 1496,
		1497, 1498, 1499, 1499, 1499, 1499, 1499, 1498, 1497, 1496,
		1494, 1492, 1490, 1487, 1485, 1481, 1478, 1474, 1470, 1466,
		1461, 1457, 1451, 1446, 1440, 1435, 1428, 1422, 1415, 1408,
		1401, 1394, 1386, 1378, 1370, 1362, 1353, 1344, 1335, 1326,
		1317, 1307, 1297, 1287, 1277, 1267, 1257, 1246, 1235, 1224,
		1213, 1202, 1191, 1179, 1168, 1156, 1145, 1133, 1121, 1109,
		1097, 1085, 1073, 1061, 1049, 1036, 1024, 1012, 999, 987,
		975, 963, 950, 938, 926, 914, 902, 890, 878, 866,
		854, 843, 831, 820, 808, 797, 786, 775, 764, 753,
		742, 732, 722, 712, 702, 692, 682, 673, 664, 655,
		646, 637, 629, 621, 613, 605, 598, 591, 584, 577,
		571, 564, 559, 553, 548, 542, 538, 533, 529, 525,
		521, 518, 514, 512, 509, 507, 505, 503, 502, 501,
		500, 500, 499, 500, 500, 501, 502, 503, 505, 507,
		509, 512, 514, 518, 521, 525, 529, 533, 538, 542,
		548, 553, 559, 564, 571, 577, 584, 591, 598, 605,
		613, 621, 629, 637, 646, 655, 664, 673, 682, 692,
		702, 712, 722, 732, 742, 753, 764, 775, 786, 797,
		808, 820, 831, 843, 854, 866, 878, 890, 902, 914,
		926, 938, 950, 963, 975, 987,
	};
	// freq ctrl thru itvl between triggerings
	static constexpr uint16_t ARR[] {84, 155};
	// the same time slot for 2 symbols
	static constexpr uint16_t S[] {11 * sine12bit.size() / 6, sine12bit.size()};
	// state var
	using State_t = enum { READY, STARTUP, TXING, ENDING };
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
	// Moore FSM output
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
		case ENDING:
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
				TxDelay = 240; // guidance: 0.2 s
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
				state = ENDING;
				TxDelay = 12; // shutting down: 0.01 s
			}
			break;
		case ENDING:
			if (--TxDelay == 0) {
				state = READY;
				__HAL_DAC_DISABLE(hdac, Channel);
			}
			break;
		default:
			break;
	}
}

#endif /* SRC_AFSKGENERATOR_ */
