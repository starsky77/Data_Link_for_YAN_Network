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

    // ref direwolf
    // from http://www.ietf.org/rfc/rfc1549.txt
    const uint16_t ccitt_table[256] = {
       0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
       0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
       0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
       0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
       0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
       0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
       0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
       0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
       0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
       0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
       0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
       0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
       0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
       0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
       0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
       0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
       0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
       0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
       0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
       0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
       0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
       0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
       0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
       0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
       0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
       0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
       0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
       0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
       0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
       0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
       0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
       0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
    };
    // Use this for an AX.25 frame.
    uint16_t fcs_calc (const unsigned char *data, int len)
    {
    	unsigned short crc = 0xffff;
    	int j;

    	for (j=0; j<len; j++) {

      	  crc = ((crc) >> 8) ^ ccitt_table[((crc) ^ data[j]) & 0xff];
    	}

    	return ( crc ^ 0xffff );
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
				mod.switches(AFSK_Modulator::OFF);
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
