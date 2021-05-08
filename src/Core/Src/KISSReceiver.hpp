/*
 * KISSReceiver.hpp
 *
 *  Created on: May 7, 2021
 *      Author: jiao
 */
#ifndef SRC_KISSRECEIVER_HPP_
#define SRC_KISSRECEIVER_HPP_

#include "main.h"

class KISS_Receiver{
	UART_HandleTypeDef * huart_ = &huart1;
	static const uint32_t length_=1024;
	uint8_t buffer_[length_];
	uint16_t receive_count_ = 0;
	uint16_t handle_count_ = 0;

	uint16_t start_pos_ = 0;

	enum state{
		INIT, ING, END
	};
	uint8_t state = INIT;
public:
	//! KISS Special Characters
	//! http://en.wikipedia.org/wiki/KISS_(TNC)#Special_Characters
	//! http://k4kpk.com/content/notes-aprs-kiss-and-setting-tnc-x-igate-and-digipeater
	//! Frames begin and end with a FEND/Frame End/0xC0 byte
	char FEND = '\xC0';  //! Marks START and END of a Frame
	char FESC = '\xDB';  //! Escapes FEND and FESC bytes within a frame
	//! Transpose Bytes: Used within a frame-
	//! "Transpose FEND": An FEND after an FESC (within a frame)-
	//! Sent as FESC TFEND
	char TFEND = '\xDC';
	//! "Transpose FESC": An FESC after an FESC (within a frame)-
	//! Sent as FESC TFESC
	char TFESC = '\xDD';

	KISS_Receiver(): huart_(&huart1), FEND('\xC0'), FESC('\xDB'), TFEND('\xDC'), TFESC('\xDD') { }

	void receive_byte(uint8_t ch){
		buffer_[receive_count_] = ch;
		receive_count_ = (receive_count_+1)%length_;
	}

	//! TODO(JXR) further test
	void receive_multi_bytes(uint8_t* c, uint32_t len){
		for(uint32_t i = 0; i < len; i++){
			buffer_[receive_count_] = c[i];
			receive_count_ = (receive_count_+1)%length_;
		}
	}

	void handle_buffer(){
		if(receive_count_ == handle_count_) return;

		switch (state){
			case INIT:
				if(buffer_[handle_count_] == FEND){
					state = ING;
					start_pos_ = handle_count_;
				}
				break;
			case ING:
				if(buffer_[handle_count_] == FEND){
					state = INIT;
					send_package(handle_count_);
				}
				break;
			case END:
				break;
		}
		handle_count_ = (handle_count_+1)%length_;
	}

	void send_package(uint16_t end_pos){
		  char c[length_];
		  if(end_pos > start_pos_){
			  sprintf(c, "%s", buffer_+start_pos_);
			  HAL_UART_Transmit_DMA(huart_, (uint8_t *)(c), end_pos-start_pos_+1);
		  }
		  else{
			  char c[length_];
			  uint32_t k = 0;
			  while(start_pos_+k < length_){
				  c[k] = buffer_[start_pos_ + k];
				  k++;
			  }
			  while(k-(length_-start_pos_) <= end_pos){
				  c[k] = buffer_[k-(length_-start_pos_)];
				  k++;
			  }
			  HAL_UART_Transmit_DMA(huart_, (uint8_t *)(c), end_pos+length_-start_pos_+1);
		  }
	}
};



#endif /* SRC_KISSRECEIVER_HPP_ */
