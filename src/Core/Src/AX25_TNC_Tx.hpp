/*
 * AX25_TNC_Tx.hpp
 *
 *  Created on: Apr 3, 2021
 *      Author: Guan
 */

#ifndef SRC_AX25_TNC_TX_HPP_
#define SRC_AX25_TNC_TX_HPP_

#include "main.h"
#include "CircularQueue.hpp"
#include "crc.h"
#include <stdint.h>


/* Handle the DAC to emit the actual sine wave,
 * with symbols defined in AFSK1200
 */
class AFSK_Modulator {
private:
	// HW handle operated on
	DAC_HandleTypeDef *hdac;
	uint32_t Channel;
	TIM_HandleTypeDef *dac_tim;
	// sine code table w/ val in range [0, 4096)
	static constexpr std::array<uint16_t, 256> sine12bit {
		699, 708, 717, 725, 734, 742, 751, 759, 768, 776,
		785, 793, 801, 809, 817, 825, 833, 841, 849, 857,
		864, 872, 879, 887, 894, 901, 908, 915, 922, 928,
		935, 941, 947, 953, 959, 965, 970, 975, 981, 986,
		991, 995, 1000, 1004, 1008, 1012, 1016, 1019, 1023, 1026,
		1029, 1032, 1034, 1037, 1039, 1041, 1043, 1044, 1046, 1047,
		1048, 1049, 1049, 1049, 1049, 1049, 1049, 1049, 1048, 1047,
		1046, 1044, 1043, 1041, 1039, 1037, 1034, 1032, 1029, 1026,
		1023, 1019, 1016, 1012, 1008, 1004, 1000, 995, 991, 986,
		981, 975, 970, 965, 959, 953, 947, 941, 935, 928,
		922, 915, 908, 901, 894, 887, 879, 872, 864, 857,
		849, 841, 833, 825, 817, 809, 801, 793, 785, 776,
		768, 759, 751, 742, 734, 725, 717, 708, 699, 691,
		682, 674, 665, 657, 648, 640, 631, 623, 614, 606,
		598, 590, 582, 574, 566, 558, 550, 542, 535, 527,
		520, 512, 505, 498, 491, 484, 477, 471, 464, 458,
		452, 446, 440, 434, 429, 424, 418, 413, 408, 404,
		399, 395, 391, 387, 383, 380, 376, 373, 370, 367,
		365, 362, 360, 358, 356, 355, 353, 352, 351, 350,
		350, 350, 349, 350, 350, 350, 351, 352, 353, 355,
		356, 358, 360, 362, 365, 367, 370, 373, 376, 380,
		383, 387, 391, 395, 399, 404, 408, 413, 418, 424,
		429, 434, 440, 446, 452, 458, 464, 471, 477, 484,
		491, 498, 505, 512, 520, 527, 535, 542, 550, 558,
		566, 574, 582, 590, 598, 606, 614, 623, 631, 640,
		648, 657, 665, 674, 682, 691,
	};
	// freq ctrl thru itvl between triggerings
	static constexpr uint16_t ARR[] {84, 155};
	// the same time slot for 2 symbols
	static constexpr uint16_t S[] {11 * sine12bit.size() / 6, sine12bit.size()};
public:
	// state var
	using State_t = enum { OFF, ON };

	AFSK_Modulator(): hdac(nullptr), Channel(0), dac_tim(nullptr) {}
	AFSK_Modulator(const AFSK_Modulator&) = delete;
	AFSK_Modulator& operator=(const AFSK_Modulator&) = delete;

	int init(DAC_HandleTypeDef *hdac_, uint32_t Channel_, TIM_HandleTypeDef *htim_);
	bool dac_enabled() {
		return (hdac->Instance->CR) & (DAC_CR_EN1 << Channel) != 0;
	}
	uint32_t cur_val() { return HAL_DAC_GetValue(hdac, Channel); }

	void switches(State_t state) {
		if (state == OFF) {
			__HAL_DAC_DISABLE(hdac, Channel);
		} else if (state == ON) {
			__HAL_DAC_ENABLE(hdac, Channel);
		} else {
			assert(false);
		}
	}
	void Tx_symbol(uint8_t symbol) {
		// config to Tx this symbol; assume preload enabled
		__HAL_TIM_SET_AUTORELOAD(dac_tim, ARR[symbol]);
	}

	~AFSK_Modulator() {
		if (HAL_DAC_Stop_DMA(hdac, Channel) != HAL_OK) Error_Handler();
	}
};

int AFSK_Modulator::init(DAC_HandleTypeDef *hdac_, uint32_t Channel_, TIM_HandleTypeDef *htim_) {
	hdac = hdac_;
	Channel = Channel_;
	dac_tim = htim_;
	auto s1 = HAL_DAC_Start_DMA(hdac, Channel, (uint32_t *)sine12bit.begin(), sine12bit.size(), DAC_ALIGN_12B_R);
	switches(OFF);
	auto s2 = HAL_TIM_Base_Start(dac_tim);
	if (s1 == HAL_OK && s2 == HAL_OK) return 0;
	else return 1;
}

/* Handle the transmission of frames, including
 * preamble sequence and frame delimiter.
 * Handle states and coordinate;
 * Maintain frame queue;
 */
class AX25_TNC_Tx {
public:
	AX25_TNC_Tx(): state(IDLE), req(NA), head(0), byte(0), mask(0x80), symbol(0) {}

	int init(DAC_HandleTypeDef *hdac_, uint32_t Channel_, TIM_HandleTypeDef *htim_) {
		return mod.init(hdac_, Channel_, htim_);
	}
	using Request_t = enum { NA, SEIZE, RELEASE };
	void request(Request_t req_) {
		req = req_;
	}
	void DATA_Request(const uint8_t* buf, size_t len) {
		// append FCS on req
		uint32_t crc = fcs_calc(buf, len);
		fbegins.push(frames.tail);
		assert(frames.fifo_put(buf, len) == len);
		frames.push((uint8_t)(crc & 0xff));
		crc >>= 8;
		frames.push((uint8_t)(crc & 0xff));
	}
	// invoked in the middle of every slot
	int update();

	friend struct UnitTest_DA; // more flexible manipulation
private:
	AFSK_Modulator mod;
	// state var
	using State_t = enum { IDLE, STARTUP, FBEGIN, FRAME, FEND, };
	State_t state;
	Request_t req;
	// frame queue
	CircularQueue<uint8_t, 255> frames;
	CircularQueue<size_t, 15> fbegins;
	// Tx output
	uint8_t head; // idx pointing into frames
	uint8_t byte;
	uint8_t mask;
	uint8_t symbol;

	int HDLC_encode(uint8_t bit);
};

/* Handle bit stuffing and NRZI line encoding.
 * non-zero rv means bit stuffing is triggered
 * and Tx for this slot should be suspended
 */
int AX25_TNC_Tx::HDLC_encode(uint8_t bit) {
	static uint8_t nb_1bit = 0;
	if (state == FRAME) {
		// bit stuffing
		if (nb_1bit == 5) {
			nb_1bit = 0;
			symbol = !symbol;
			return 1;
		}
		// NRZI
		if (bit) {
			nb_1bit++;
		} else {
			nb_1bit = 0;
			symbol = !symbol;
		}
	} else { // simple NRZI
		if (bit == 0) symbol = !symbol;
		nb_1bit = 0;
	}
	return 0;
}

// direwolf: 8 FEND --- 3 FEND
int AX25_TNC_Tx::update() {
	static uint8_t suspended = 0, bit = 0, // FRAME
			FEND_cnt = 0, // FBEGIN/FEND
			TxDelay = 0; // STARTUP
	// Moore FSM: output to AFSK_Modulator
	switch (state) {
	case IDLE:
		mod.switches(AFSK_Modulator::OFF);
		break;
	case STARTUP:
		// Tx 0 for TxDelay slots
		if (TxDelay == 0) {
			// refresh; preamble: 0.2 s
			TxDelay = 240;
			mod.Tx_symbol((symbol = 0));
			mod.switches(AFSK_Modulator::ON);
		} else {
			HDLC_encode(0);
			mod.Tx_symbol(symbol);
		}
		break;
	case FBEGIN:
	case FEND:
		if (mask == 0x80) {
			// get next byte
			byte = 0x7E;
		}
		mask = mask << 1 | mask >> 7; // rol
		// get next bit
		bit = (byte & mask) != 0;
		HDLC_encode(bit);
		mod.Tx_symbol(symbol);
		break;
	case FRAME:
		if (!suspended) {
			if (mask == 0x80) {
				// get next byte
				byte = frames.elements[head];
				head = (head+1) % (frames.max_size()+1);
			}
			mask = mask << 1 | mask >> 7; // rol
			// get next bit
			bit = (byte & mask) != 0;
		}
		// in case of failure, this bit is not Tx'd
		suspended = HDLC_encode(bit);
		// config to Tx this symbol; assume preload enabled
		mod.Tx_symbol(symbol);
		break;
	default:
		break;
	}
	// decide next state based on internals and requests
	// The point is DO NOT touch AFSK_Modulator in this sect.
	switch (state) {
	case IDLE:
		if (req == SEIZE) {
			state = STARTUP;
		}
		req = NA;
		break;
	case STARTUP:
		if (--TxDelay == 0) {
			if (frames.empty()) {
				state = IDLE;
			} else {
				state = FBEGIN;
				FEND_cnt = 3;
				fbegins.pop();
			}
		}
		break;
	case FBEGIN:
		if (mask == 0x80) {
			--FEND_cnt;
		}
		if (FEND_cnt == 0) {
			head = frames.head;
			state = FRAME;
		}
		break;
	case FRAME:
		if (!suspended) {
			// current byte Tx cplt
			if (mask == 0x80) {
				// current frame Tx cplt
				if (fbegins.empty()) {
					if (head == frames.tail) {
						state = FEND;
						FEND_cnt = 3;
						frames.head = head;
					}
				} else {
					if (head == fbegins.front()) {
						state = FEND;
						FEND_cnt = 3;
						frames.head = head;
					}
				}
			}
		}
		break;
	case FEND:
		if (mask == 0x80) {
			--FEND_cnt;
		}
		if (FEND_cnt == 0) {
			if (frames.empty()) {
				state = IDLE;
			} else {
				state = FBEGIN;
				FEND_cnt = 3;
				fbegins.pop();
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

#endif /* SRC_AFSKGENERATOR_ */
