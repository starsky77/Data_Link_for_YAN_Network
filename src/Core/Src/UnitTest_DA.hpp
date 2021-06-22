/*
 * UnitTest-DA.hpp
 *
 *  Created on: Jun 7, 2021
 *      Author: Guan
 */

#ifndef SRC_UNITTEST_DA_HPP_
#define SRC_UNITTEST_DA_HPP_

#include "AX25_TNC_Tx.hpp"
#include "main.h"
#include <stdint.h>

struct UnitTest_DA {
	AX25_TNC_Tx &uut;
	UnitTest_DA(AX25_TNC_Tx &uut): uut(uut) {}

	int test1() {
		static uint32_t tick = 0, symbol = 0;
		if (tick == 0) {
			uut.mod.switches(AFSK_Modulator::State_t::ON);
			uut.mod.Tx_symbol(symbol);
		}
		uint32_t t = HAL_GetTick();
		if (t >= tick + 1000) {
			tick = t;
			symbol = !symbol;
			uut.mod.Tx_symbol(symbol);
		}
		return 0;
	}

};



#endif /* SRC_UNITTEST_DA_HPP_ */
