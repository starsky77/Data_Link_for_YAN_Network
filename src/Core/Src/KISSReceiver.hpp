/*
 * KISSReceiver.hpp
 *
 *  Created on: May 7, 2021
 *      Author: jiao
 */
#ifndef SRC_KISSRECEIVER_HPP_
#define SRC_KISSRECEIVER_HPP_

#include "main.h"

#include "AX25_TNC_Tx.hpp"

extern AX25_TNC_Tx ax25Tx;

// handle kiss frame flow
class KISS_Receiver{
	UART_HandleTypeDef *huart_;
	static constexpr uint32_t length_=256;
	// circular_buffer; content produced by DMA, consumed by ?
	uint8_t buffer_[length_];
	// linear buffer storing final result
	uint8_t result_[length_];
	//TODO space can be optimized

	uint16_t receive_count_ = 0; // DMA progress
	uint16_t handle_count_ = 0; // ? progress

	uint16_t end_pos_ = 0;

	enum state{
		INIT, ING, ESC
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


//	KISS_Receiver(): huart_(&huart1), FEND('\xC0'), FESC('\xDB'), TFEND('\xDC'), TFESC('\xDD') { }
	KISS_Receiver(UART_HandleTypeDef* huart){
		huart_ = huart;
	}

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
		while (receive_count_ != handle_count_) {
			switch (state){
				case INIT:
					if(buffer_[handle_count_] == FEND){
						state = ING;
						++handle_count_; // cmd byte
					}
					break;
				case ING:
					if(buffer_[handle_count_] == FEND){
						state = INIT;
						send_to_audio(end_pos_);
						end_pos_ = 0;
					}
					else if(buffer_[handle_count_] == FESC){
						state = ESC;
					}
					else{
						result_[end_pos_++] = buffer_[handle_count_];
					}
					break;
				case ESC:
					if(buffer_[handle_count_] == TFESC){
						state = ING;
						result_[end_pos_++] = FESC;
					}
					else if (buffer_[handle_count_] == TFEND){
						state = ING;
						result_[end_pos_++] = FEND;
					}
					break;
				default:
					break;
			}
			handle_count_ = (handle_count_+1)%length_;
		}
	}

	void send_to_audio(uint16_t end_pos){
		  ax25Tx.DATA_Request(result_, end_pos_);
		  ax25Tx.request(AX25_TNC_Tx::Request_t::SEIZE);
	}

	void send_to_host(char* in, uint32_t ilen){
	      static char out[length_];
	      uint32_t len = encapsulate(in, ilen, out);
	      HAL_UART_Transmit_DMA(huart_, (uint8_t *)out, len);

	      //JXR for test
	      ax25Tx.DATA_Request((uint8_t*)out, len);
	      ax25Tx.request(AX25_TNC_Tx::Request_t::SEIZE);
	}

	uint32_t encapsulate(char* in, uint32_t ilen, char* out)
	{
		uint8_t olen;
		uint8_t j;
		olen = 0;
		out[olen++] = FEND;
		for (j=0; j<ilen; j++) {

		  if (in[j] == FEND) {
		    out[olen++] = FESC;
		    out[olen++] = TFEND;
		  }
		  else if (in[j] == FESC) {
		    out[olen++] = FESC;
		    out[olen++] = TFESC;
		  }
		  else {
		    out[olen++] = in[j];
		  }
		}
		out[olen++] = FEND;
		return (olen);
	}

};

#endif /* SRC_KISSRECEIVER_HPP_ */
