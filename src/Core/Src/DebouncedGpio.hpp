/*
 * DebouncedGpio.hpp
 *
 *  Created on: Apr 10, 2021
 *      Author: Guan
 */

#ifndef SRC_DEBOUNCEDGPIO_HPP_
#define SRC_DEBOUNCEDGPIO_HPP_

#include "main.h"

// poll version relying on HAL_GPIO_ReadPin
class DebouncedGpio {
	GPIO_TypeDef *GPIOx;
	uint16_t GPIO_Pin;
	GPIO_PinState last = GPIO_PIN_SET, out = last;
	int hold_timeout = 0;
	// input filter delay in polls
	static constexpr int MAX_CNT = 20;
public:
	DebouncedGpio(GPIO_TypeDef *GPIOx_, uint16_t GPIO_Pin_) : GPIOx(GPIOx_), GPIO_Pin(GPIO_Pin_) {}
	DebouncedGpio(const DebouncedGpio&) = delete;
	DebouncedGpio& operator=(const DebouncedGpio&) = delete;
	GPIO_PinState poll() {
		auto cur = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
		if (cur != last) {
			// reset filter stage on detection of edge
			hold_timeout = MAX_CNT;
		}
		else {
			// filter stage cplt
			if (hold_timeout == 0) {
				out = cur;
			}
			else {
				hold_timeout--;
			}
		}
		last = cur;
		return out;
	}
};

// Interrupt version, state updated async by EXTI ED
class DebouncedGpio_IT {
	GPIO_TypeDef *GPIOx;
	uint16_t GPIO_Pin;
	GPIO_PinState last = GPIO_PIN_SET, out = last;
	uint32_t hold_timeout = 0;
	// input filter delay in ticks
	static constexpr int HOLD_DELAY = 20;
public:
	DebouncedGpio_IT(GPIO_TypeDef *GPIOx_, uint16_t GPIO_Pin_) : GPIOx(GPIOx_), GPIO_Pin(GPIO_Pin_) {}
	DebouncedGpio_IT(const DebouncedGpio_IT&) = delete;
	DebouncedGpio_IT& operator=(const DebouncedGpio_IT&) = delete;
	GPIO_PinState update() {
		auto cur = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
		auto now = HAL_GetTick();
		// filter expired
		if (now > hold_timeout) {
			out = cur;
		}
		if (cur != last) {
			// reset filter stage on detection of edge
			hold_timeout = now + HOLD_DELAY;
		}
		last = cur;
		return out;
	}
};

class EdgeDetector {
	int8_t last;
public:
	EdgeDetector(int8_t last_) : last(last_) {}
	int8_t detect(int8_t cur) {
		int8_t rv = 0;
		if (cur > last) {
			rv = 1;
		}
		else if (cur < last) {
			rv = -1;
		}
		last = cur;
		return rv;
	}
};

#endif /* SRC_DEBOUNCEDGPIO_HPP_ */
