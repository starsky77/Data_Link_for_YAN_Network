#ifndef _XMODEM_H_
#define _XMODEM_H_

#include "main.h"

#define APPLICATION_ADDRESS     (uint32_t)0x20000000

#define SOH                     ((uint8_t)0x01)  /* start of 128-byte data packet */
#define STX                     ((uint8_t)0x02)  /* start of 1024-byte data packet */
#define EOT                     ((uint8_t)0x04)  /* end of transmission */
#define ACK                     ((uint8_t)0x06)  /* acknowledge */
#define NAK                     ((uint8_t)0x15)  /* negative acknowledge */
#define CAN                     ((uint32_t)0x18) /* two of these in succession aborts transfer */
#define CRC16                   ((uint8_t)0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  ((uint8_t)0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  ((uint8_t)0x61)  /* 'a' == 0x61, abort by user */

#define PACKET_HEADER_SIZE      ((uint32_t)3)
#define PACKET_DATA_INDEX       ((uint32_t)4)
#define PACKET_START_INDEX      ((uint32_t)1)
#define PACKET_NUMBER_INDEX     ((uint32_t)2)
#define PACKET_CNUMBER_INDEX    ((uint32_t)3)
#define PACKET_TRAILER_SIZE     ((uint32_t)1)
#define PACKET_OVERHEAD_SIZE    (PACKET_HEADER_SIZE + PACKET_TRAILER_SIZE - 1)

#define PACKET_SIZE             128

#define FRAME_TIMEOUT				3000
#define TX_TIMEOUT          		((uint32_t)100)
#define RX_TIMEOUT          		((uint32_t)100)

#define MAX_ERRORS              15

HAL_StatusTypeDef Xmodem_Receive (uint32_t dst);

extern HAL_StatusTypeDef Serial_PutByte(uint8_t param);
uint8_t CalcChecksum(const uint8_t *p_data, uint32_t size);
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size);
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte);
void Serial_PutString(uint8_t *p_string);

#endif
