//数据帧格式，并且做了对齐
/* /-------- Packet in IAP memory ------------------------------------------\
 * | 0      |  1    |  2     |  3   |  4      | ... | n+4     | n+5  | n+6  |
 * |------------------------------------------------------------------------|
 * | unused | start | number | ~num | data[0] | ... | data[n] | crc0 | crc1 |
 * \------------------------------------------------------------------------/
 * the first byte is left unused for memory alignment reasons                 */

#include "main.h"
#include "usart.h"
#include "xmodem.h"
#include <stdio.h>
#include <string.h>

static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length);

uint8_t PacketData[PACKET_SIZE + PACKET_DATA_INDEX + PACKET_TRAILER_SIZE];

HAL_StatusTypeDef Xmodem_Receive (uint32_t dst)
{
  uint8_t errors = 0;
  uint32_t packet_length, file_done = 0;
  uint8_t seq = 1;
  uint8_t response = NAK;
  HAL_StatusTypeDef result = HAL_OK;

	// 开始信号 累加和校验
	Serial_PutByte(response);

	//开始接受数据
	while ((file_done == 0) && (result == HAL_OK))
	{
		// Rx a packet, num & csum validation included
		switch (ReceivePacket(PacketData, &packet_length))
		{
			//接收成功
			case HAL_OK:
				switch (packet_length)
				{
					case 0:
						// EOT: 回 ACK
						response = ACK;
						++file_done;
						break;
					default:
						// i.e. 128
						// validate seq
						if (PacketData[PACKET_NUMBER_INDEX] == seq) // expected
						{
							response = ACK;
							//写数据
							memcpy((uint8_t *)dst, &PacketData[PACKET_DATA_INDEX], packet_length);
							++seq;
						}
						else if (PacketData[PACKET_NUMBER_INDEX] == seq-1) // prev recved
						{
							response = ACK;
						}
						else // 失序
						{
							response = CAN;

						}
						break;
				}
				break;
			default:  // HAL_ERROR: 帧头错，补码误差，校验错; HAL_TIMEOUT, HAL_BUSY
				if (++errors > MAX_ERRORS) {
					response = CAN;
				}
				else {
					response = NAK;
				}
				break;
		}
		Serial_PutByte(response);
		if (response == CAN) {
			Serial_PutByte(CAN);
			result |= HAL_ERROR;
		}
	}
	return result;
}

/* Rx a xmodem packet, possibly EOT
 * return status, data, len
 */
static HAL_StatusTypeDef ReceivePacket(uint8_t *p_data, uint32_t *p_length)
{
  uint8_t csum = 0, packet_size = 0;
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t byte;

  // get 1st byte of the packet
  status = HAL_UART_Receive(&huart1, &byte, 1, FRAME_TIMEOUT);

  if (status == HAL_OK)
  {
    switch (byte)
    {
      case SOH:
        packet_size = PACKET_SIZE;  //开始帧头，接收128byte数据
        break;
      case EOT:
		packet_size = 0;  //接收完成，size置零
        break;
      default:
        status |= HAL_ERROR;  //接收错误，size本来就为零
        break;
    }

		//第一位不适用，为了内存对齐，从第二位开始
    p_data[PACKET_START_INDEX] = byte;

		//判断是否需要发送一帧数据过来
    if (packet_size == PACKET_SIZE )
    {
    	//从第三位开始接受，总共2+128+1，采用累加和的校验
      status = HAL_UART_Receive(&huart1, &p_data[PACKET_NUMBER_INDEX], packet_size + PACKET_OVERHEAD_SIZE, FRAME_TIMEOUT);

      /* 判断接收是否成功 Simple packet sanity check */
      if (status == HAL_OK )
      {
		//检查数据包序号是否正确
        if (p_data[PACKET_NUMBER_INDEX] != (p_data[PACKET_CNUMBER_INDEX] ^ 0xFF))
        {
          status |= HAL_ERROR;
        }
        else
        {
          //累加和校验
          csum = p_data[ packet_size + PACKET_DATA_INDEX ];
          if (CalcChecksum(&p_data[PACKET_DATA_INDEX], packet_size) != csum )
          {
            status |= HAL_ERROR;
          }

//          //CRC16校验
//          crc = p_data[ packet_size + PACKET_DATA_INDEX ] << 8;
//          crc += p_data[ packet_size + PACKET_DATA_INDEX + 1 ];
//          if (Cal_CRC16(&p_data[PACKET_DATA_INDEX], packet_size) != crc )
//          {
//            status = HAL_ERROR;
//          }
        }
      }
    }
  }
	else if(status == HAL_TIMEOUT)
	{
		Serial_PutString("\r\n=================== Receive Package TIMEOUT ===========\r\n\n");
	}
	else
	{
		Serial_PutString("\r\n=================== Receive Package ERROR ===========\r\n\n");
	}
  *p_length = packet_size;
  return status;
}

/**
  * @brief  Update CRC16 for input byte
  * @param  crc_in input value
  * @param  input byte
  * @retval None
  */
uint16_t UpdateCRC16(uint16_t crc_in, uint8_t byte)
{
  uint32_t crc = crc_in;
  uint32_t in = byte | 0x100;

  do
  {
    crc <<= 1;
    in <<= 1;
    if(in & 0x100)
      ++crc;
    if(crc & 0x10000)
      crc ^= 0x1021;
  }

  while(!(in & 0x10000));

  return crc & 0xffffu;
}

/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
  * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* p_data, uint32_t size)
{
  uint32_t crc = 0;
  const uint8_t* dataEnd = p_data+size;

  while(p_data < dataEnd)
    crc = UpdateCRC16(crc, *p_data++);

  crc = UpdateCRC16(crc, 0);
  crc = UpdateCRC16(crc, 0);

  return crc&0xffffu;
}

/**
  * @brief  Calculate Check sum for YModem Packet
  * @param  p_data Pointer to input data
  * @param  size length of input data
  * @retval uint8_t checksum value
  */
uint8_t CalcChecksum(const uint8_t *p_data, uint32_t size)
{
  uint32_t sum = 0;
  const uint8_t *p_data_end = p_data + size;

  while (p_data < p_data_end )
  {
    sum += *p_data++;
  }

  return (sum & 0xffu);
}

/**
  * @brief  Transmit a byte to the HyperTerminal
  * @param  param The byte to be sent
  * @retval HAL_StatusTypeDef HAL_OK if OK
  */
HAL_StatusTypeDef Serial_PutByte(uint8_t param)
{
  /* May be timeouted... */
  putchar(param);
  return HAL_OK;
}

/**
  * @brief  Print a string on the HyperTerminal
  * @param  p_string: The string to be printed
  * @retval None
  */
void Serial_PutString(uint8_t *p_string)
{
	fputs(p_string, stdout);
}
