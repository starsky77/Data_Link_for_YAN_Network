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


namespace {
	// ref LeetCode
    constexpr auto POW2(int n) { return ((uint32_t)1) << n; };
    constexpr auto MASK = (uint32_t)(-1);
    // ROUND(n) + ROUND(n) << POW2(n) = MASK
    constexpr auto ROUND(int n) { return MASK / (POW2(POW2(n))+1); };
    uint32_t reverseBits(uint32_t n) {
        for (int i = 0; i < 5; i++) {
            n = ((n & ROUND(i)) << POW2(i)) + ((n >> POW2(i)) & ROUND(i));
        }
        return n;
    }

    // ref ymodem
    uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte) {
      uint32_t crc = crc_in;
      uint32_t in = byte | 0x100;

      do {
        crc <<= 1;
        in <<= 1;
        if (in & 0x100)
          ++crc;
        if (crc & 0x10000)
          crc ^= 0x1021;
      } while (!(in & 0x10000));

      return crc & 0xffffu;
    }
};

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
		uint32_t crc = 0;
		for (int i = 0; i < len; i++) {
			crc = UpdateCRC16(crc, buf[i]);
		}
		crc = reverseBits(crc & 0xffffu) >> 16;
		fbegins.push(frames.tail);
		assert(frames.fifo_put(buf, len) == len);
		frames.push((uint8_t)(crc & 0xff));
		crc >>= 8;
		frames.push((uint8_t)(crc & 0xff));
	}
	// invoked in the middle of every slot
	int update();
	int HDLC_encode(uint8_t bit);

	friend struct UnitTest_DA;
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
	case STARTUP:
		// Tx 0 for TxDelay slots
		HDLC_encode(0);
		mod.Tx_symbol(symbol);
		break;
	case FBEGIN:
	case FEND:
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
				byte = frames[head];
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
	switch (state) {
	case IDLE:
		if (req == SEIZE) {
			state = STARTUP;
			TxDelay = 240; // preamble: 0.2 s
			mod.Tx_symbol(symbol);
			mod.switches(AFSK_Modulator::ON);
		}
		req = NA;
		break;
	case STARTUP:
		if (--TxDelay == 0) {
			if (frames.empty()) {
				state = IDLE;
				mod.switches(AFSK_Modulator::OFF);
			} else {
				state = FBEGIN;
				FEND_cnt = 3;
				byte = 0x7E;
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
						byte = 0x7E;
						frames.head = head;
					}
				} else {
					if (head == fbegins.front()) {
						state = FEND;
						FEND_cnt = 3;
						byte = 0x7E;
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
				fbegins.pop();
				FEND_cnt = 3;
				byte = 0x7E;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

#endif /* SRC_AFSKGENERATOR_ */
