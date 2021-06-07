/*
 * UnitTest-DA.hpp
 *
 *  Created on: Jun 7, 2021
 *      Author: Guan
 */

#ifndef SRC_UNITTEST_DA_HPP_
#define SRC_UNITTEST_DA_HPP_

#include "main.h"
#include "AFSKGenerator.hpp"
#include <stdint.h>

struct UnitTest_DA {
	AFSK_Generator &uut;
	UnitTest_DA(AFSK_Generator &uut): uut(uut) {}

	int test1() {
		static uint32_t tick = 0, symbol = 0;
		if (tick == 0) {
			__HAL_DAC_ENABLE(uut.hdac, uut.Channel);
			uut.Tx_symbol(symbol);
		}
		uint32_t t = HAL_GetTick();
		if (t >= tick + 1000) {
			tick = t;
			symbol = !symbol;
			uut.Tx_symbol(symbol);
		}
		return 0;
	}

};



#endif /* SRC_UNITTEST_DA_HPP_ */
