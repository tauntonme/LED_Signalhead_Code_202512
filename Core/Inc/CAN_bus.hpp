/*
 * CAN_bus.hpp
 *
 *  Created on: Sep 2, 2024
 *      Author: Jon Freeman  B. Eng. Hons
 *  Using 'StdId' - Standard ID using 11 bit ID number 0x000 to 0x7ff (TxHeader.IDE = CAN_ID_STD;)
 *
 *      Controllers use variable 'StdId' values to address peripherals by broadcast or individually
 *      Example of 'broadcast' - entire system reset.
 *
 *      Peripherals use 'n' bit addresses using the 'n' LSBs.
 *      One particular value in the (11 - 'n') MSBs confirms CAN controller message to one particular peripheral.
 *      Other (11 - 'n') MS bit codes for system reset and any other 'broadcast' or 'narrowcast' uses to be defined.
 *
 *  CAN messages from controllers -> peripherals
 *  	Controllers may send DATA and REMOTE REQUEST frames.
 *  	** NOT USING REMOTE REQUEST frames as CAN FD does not support this.
 *
 *  CAN messages from peripherals -> controllers	When and how to respond?
 *
 *  Data - Packet length 0 to 8 bytes
 *
 *	Priority
 *		The message with the lowest identifier value has the highest priority
 *		according to the arbitration of the CAN protocol
 *
 *	Device Addressing
 *		Decided - Use 7 least significant bits to address 1 of up to 127 remote devices (numbered 1 to 127)
 *		High 4 bits - arbitrary choice 0x07 (0x380 in correct bit positions) - about half way up priority
 *


  * @brief  CAN Tx message header structure definition
*/
//typedef struct
//{
//  uint32_t StdId;    /*!< Specifies the standard identifier.
//                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x7FF. */

//  uint32_t ExtId;    /*!< Specifies the extended identifier.
//                          This parameter must be a number between Min_Data = 0 and Max_Data = 0x1FFFFFFF. */

//  uint32_t IDE;      /*!< Specifies the type of identifier for the message that will be transmitted.
//                          This parameter can be a value of @ref CAN_identifier_type */

//  uint32_t RTR;      /*!< Specifies the type of frame for the message that will be transmitted.
//                          This parameter can be a value of @ref CAN_remote_transmission_request */

//  uint32_t DLC;      /*!< Specifies the length of the frame that will be transmitted.
//                          This parameter must be a number between Min_Data = 0 and Max_Data = 8. */

//  FunctionalState TransmitGlobalTime; /*!< Specifies whether the timestamp counter value captured on start
//                          of frame transmission, is sent in DATA6 and DATA7 replacing pData[6] and pData[7].
//                          @note: Time Triggered Communication Mode must be enabled.
//                          @note: DLC must be programmed as 8 bytes, in order these 2 bytes are sent.
//                          This parameter can be set to ENABLE or DISABLE. */




#ifndef INC_CAN_BUS_HPP_
#define INC_CAN_BUS_HPP_

/*struct	can_send_data_struct	{
	int	node;
	int	len;
	uint8_t	data[8];
}	;
*/
//class	CAN_send_circ_buff	{	//	Added Sept 2024
class	CAN_send_circ_buff	{	//	Put CAN messages on circular buffer and check often from forever loop, avoids sending from interrupt context
	#define	CANSENDBUFSIZ	10
	struct	can_send_data_struct	{
		int	node;
		int	len;
		uint8_t	data[8];
	}	;

	CAN_TxHeaderTypeDef   	CAN_TxHeader;
	can_send_data_struct	ds[CANSENDBUFSIZ] {};

	uint32_t	on_ptr 		{0};
	uint32_t	off_ptr 	{0};
	uint32_t	msg_que_cnt {0};
	volatile	bool	full		{false};
	volatile	bool	empty		{true};
public:
	CAN_send_circ_buff		()	{	//	Constructor
		CAN_TxHeader.IDE = CAN_ID_STD;	//	CAN_ID_STD means that we are using the Standard ID (not extended)
		CAN_TxHeader.RTR = CAN_RTR_DATA;	//	CAN_RTR_DATA - Sending a data frame
		CAN_TxHeader.TransmitGlobalTime = DISABLE;
	}	;	//	Constructor
	bool		write			(int node, uint8_t * TxData, int len)	;	//	add some message to output buffer
	bool		send_any_queued	()	;	//	Check for queued messages, send one if there
	uint32_t	queued			()	{	return	(msg_que_cnt);	}
}	;





#endif /* INC_CAN_BUS_HPP_ */
