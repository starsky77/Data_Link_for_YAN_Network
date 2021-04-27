/*
 * CircularQueue.hpp
 *
 *  Created on: Apr 10, 2021
 *      Author: Guan
 */

#ifndef SRC_CIRCULARQUEUE_HPP_
#define SRC_CIRCULARQUEUE_HPP_

#include "main.h"
#include <algorithm>
#include <string.h>

constexpr unsigned roundup_pow_of_two(unsigned x) {
	unsigned pow = 1;
	while (pow < x) {
		pow <<= 1;
	}
	return pow;
}

template<typename T, size_t N>
struct CircularQueue
{
	static_assert((N+1)==roundup_pow_of_two(N+1), "require (N+1) to be pow of 2");
    T elements[N+1]; // one slot redundant to create bijection diff <=> sz
    size_t head, tail;

    CircularQueue(): head(0), tail(0) { }
    constexpr size_t max_size() const noexcept {
    	return N;
    }
    size_t size() const {
    	return tail >= head ? tail - head
    			: tail + (N+1) - head;
    }
    size_t continuous_sgmt_size() const {
    	return tail >= head ? tail - head
    			: (N+1) - head;
    }
    bool empty() {
        return head == tail;
    }
    bool full() {
        return (tail+1) % (N+1) == head;
    }
    T& front() {
        if (empty()) {
        	Error_Handler();
        }
        return elements[head];
    }
    T& rear() {
        if (empty()) {
        	Error_Handler();
        }
        return elements[tail];
    }
    void push(const T& value) {
        if (full()) {
        	Error_Handler();
        }
        elements[tail] = value;
        tail = (tail+1) % (N+1);
    }
    void pop() {
        if (empty()) {
        	Error_Handler();
        }
        head = (head+1) % (N+1);
    }
    unsigned fifo_put(const T* buf, size_t len) {
    	// actual nb of elems put
    	len = std::min(len, max_size() - size());
    	// first put the data starting from tail to buffer end
    	size_t l = std::min(len, (N+1) - tail);
    	memcpy(elements+tail, buf, l*sizeof(T));
    	// then put the rest (if any) at the beginning of the buffer
    	memcpy(elements, buf+l, (len-l)*sizeof(T));
    	// put data BEFORE updating ptr to ensure concurrency safety
    	tail = (tail + len) % (N+1);

    	return len;
    }
};


#endif /* SRC_CIRCULARQUEUE_HPP_ */
