/*
 * AX25_TNC_Rx.hpp
 *
 *  Created on: 2021年6月22日
 *      Author: Guan
 */

#ifndef SRC_AX25_TNC_RX_HPP_
#define SRC_AX25_TNC_RX_HPP_

#include "crc.h"
#include "main.h"
#include <functional>
#include <stdint.h>

/* Handle the reception of frames, including
 * preamble sequence and frame delimiter.
 * Handle states and coordinate;
 * No frame queue;
 */
class AX25_TNC_Rx {
public:
	AX25_TNC_Rx(): state(FEND), tail(0), byte(0), mask(0x80), bit(0) {}
	int Rx_symbol(uint8_t symbol);
	std::function<int(const uint8_t*, size_t)> DATA_Indication;

private:
	// state var
	using State_t = enum { FEND, FRAME, };
	State_t state;
	// frame
	uint8_t frame[256];
	// Rx input
	uint8_t tail; // idx pointing into frame
	uint8_t byte;
	uint8_t mask;
	uint8_t bit;

	int HDLC_decode(uint8_t symbol);
};

/* Handle removal of bit stuffing and NRZI line decoding.
 * return 0 means normal, bit valid
 * return 1 means a bit stuffing is removed
 * and Rx for this slot should be skipped;
 * return 2 means nb_1bit exceeded 5,
 * possibly indicating FEND;
 */
int AX25_TNC_Rx::HDLC_decode(uint8_t symbol) {
	static uint8_t prev_symbol = 0, nb_1bit = 0;
	if (state == FRAME) {
		// bit stuffing
		if (nb_1bit == 5) {
			if (symbol != prev_symbol) {
				nb_1bit = 0;
				prev_symbol = symbol;
				return 1;
			} else {
				nb_1bit++;
				return 2;
			}
		}
		// NRZI
		if (symbol == prev_symbol) {
			bit = 1;
			nb_1bit++;
		} else {
			bit = 0;
			nb_1bit = 0;
		}
	} else { // simple NRZI
		bit = (symbol == prev_symbol);
		nb_1bit = 0;
	}
	// update prev_symbol before every return
	prev_symbol = symbol;
	return 0;
}

int AX25_TNC_Rx::Rx_symbol(uint8_t symbol) {
	static uint8_t suspended = 0;

	// Moore FSM: input from AFSK_Demodulator
	switch (state) {
	case FEND:
		HDLC_decode(symbol);
		byte = byte >> 1;
		if (bit) byte |= 0x80;
		break;
	case FRAME:
		// in case of failure, this bit is skipped
		suspended = HDLC_decode(symbol);
		if (!suspended) {
			mask = mask << 1 | mask >> 7; // rol
			// put next bit
			if (bit) {
				byte |= mask;
			} else {
				byte &= ~mask;
			}
			if (mask == 0x80) {
				// put next byte
				frame[tail++] = byte;
			}
		}
		break;
	default:
		break;
	}
	// decide next state based on internals
	switch (state) {
	case FRAME:
		if (suspended == 2) {
			state = FEND;
			byte = 0xFC;
		}
		break;
	case FEND:
		// looking for AX25 FEND
		if (byte == 0x7E) {
			if (tail > 2) {
				// check crc
				tail -= 2;
				uint16_t calc_crc = fcs_calc(frame, tail);
				// assumed little-endian
				uint16_t frame_crc = *(uint16_t *)(frame + tail);
				if (calc_crc == frame_crc) {
					DATA_Indication(frame, tail);
				}
			}
			// start receiving next frame (or FEND)
			state = FRAME;
			// init FRAME
			tail = byte = bit = 0;
			mask = 0x80;
			suspended = 0;
		}
		break;
	default:
		break;
	}
	return 0;
}

#endif /* SRC_AX25_TNC_RX_HPP_ */
