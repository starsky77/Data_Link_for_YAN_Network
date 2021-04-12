/*
 * AFSKGenerator.hpp
 *
 *  Created on: Apr 3, 2021
 *      Author: Guan
 */

#ifndef SRC_AFSKGENERATOR_
#define SRC_AFSKGENERATOR_

#include "main.h"
#include <stdint.h>
#include <array>

class AFSK_Generator {
	// HW handle operated on
	DAC_HandleTypeDef *hdac;
	uint32_t Channel;
	TIM_HandleTypeDef *htim;
	// sine code table w/ val in range [0, 4096)
	static constexpr std::array<uint16_t, 256> sine12bit {
        2047, 2098, 2148, 2198, 2248, 2298, 2348, 2398, 2447, 2496,
        2545, 2594, 2642, 2690, 2737, 2785, 2831, 2877, 2923, 2968,
        3013, 3057, 3100, 3143, 3185, 3227, 3267, 3307, 3347, 3385,
        3423, 3460, 3496, 3531, 3565, 3598, 3631, 3662, 3692, 3722,
        3750, 3778, 3804, 3829, 3854, 3877, 3899, 3920, 3940, 3958,
        3976, 3992, 4007, 4021, 4034, 4046, 4056, 4065, 4073, 4080,
        4086, 4090, 4093, 4095, 4095, 4095, 4093, 4090, 4086, 4080,
        4073, 4065, 4056, 4046, 4034, 4021, 4007, 3992, 3976, 3958,
        3940, 3920, 3899, 3877, 3854, 3829, 3804, 3778, 3750, 3722,
        3692, 3662, 3631, 3598, 3565, 3531, 3496, 3460, 3423, 3385,
        3347, 3307, 3267, 3227, 3185, 3143, 3100, 3057, 3013, 2968,
        2923, 2877, 2831, 2785, 2737, 2690, 2642, 2594, 2545, 2496,
        2447, 2398, 2348, 2298, 2248, 2198, 2148, 2098, 2047, 1997,
        1947, 1897, 1847, 1797, 1747, 1697, 1648, 1599, 1550, 1501,
        1453, 1405, 1358, 1310, 1264, 1218, 1172, 1127, 1082, 1038,
        995, 952, 910, 868, 828, 788, 748, 710, 672, 635,
        599, 564, 530, 497, 464, 433, 403, 373, 345, 317,
        291, 266, 241, 218, 196, 175, 155, 137, 119, 103,
        88, 74, 61, 49, 39, 30, 22, 15, 9, 5,
        2, 0, 0, 0, 2, 5, 9, 15, 22, 30,
        39, 49, 61, 74, 88, 103, 119, 137, 155, 175,
        196, 218, 241, 266, 291, 317, 345, 373, 403, 433,
        464, 497, 530, 564, 599, 635, 672, 710, 748, 788,
        828, 868, 910, 952, 995, 1038, 1082, 1127, 1172, 1218,
        1264, 1310, 1358, 1405, 1453, 1501, 1550, 1599, 1648, 1697,
        1747, 1797, 1847, 1897, 1947, 1997
	};
	// freq ctrl thru itvl between triggerings
	static constexpr uint16_t ARR[] {84, 155};
	// the same time slot for 2 symbols
	static constexpr uint16_t S[] {11 * sine12bit.size() / 6, sine12bit.size()};
	// state var
	uint8_t symbol;
	uint16_t trigs;

	auto set_itvl() { // assume preload enabled
		__HAL_TIM_SET_AUTORELOAD(htim, ARR[symbol]);
	}

public:
	AFSK_Generator(): hdac(nullptr), Channel(0), htim(nullptr), symbol(1), trigs(0) { }
	AFSK_Generator(const AFSK_Generator&) = delete;
	AFSK_Generator& operator=(const AFSK_Generator&) = delete;
	auto init(DAC_HandleTypeDef *hdac_, uint32_t Channel_, TIM_HandleTypeDef *htim_) {
		hdac = hdac_;
		Channel = Channel_;
		htim = htim_;
		auto s1 = HAL_DAC_Start_DMA(hdac, Channel, (uint32_t *)sine12bit.begin(), sine12bit.size(), DAC_ALIGN_12B_R);
		auto s2 = HAL_TIM_Base_Start_IT(htim);
		return s1 != HAL_OK || s2 != HAL_OK;
	}

	auto resume_gen() {
		__HAL_DAC_ENABLE(hdac, Channel);
	}
	auto pause_gen() {
		__HAL_DAC_DISABLE(hdac, Channel);
	}

	auto update() {
		trigs++;
		if (trigs == S[symbol] - 1) {
			// next symbol
			symbol = !symbol;
			set_itvl();
			trigs = 0;
		}
	}
	auto cur_val() { return HAL_DAC_GetValue(hdac, Channel); }

	~AFSK_Generator() {
		if (HAL_DAC_Stop_DMA(hdac, Channel) != HAL_OK) Error_Handler();
	}
};

#endif /* SRC_AFSKGENERATOR_ */
