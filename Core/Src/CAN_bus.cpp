/*
 * CAN_bus.cpp
 *
 *  Created on: Dec 26, 2023
 *      Author: Jon Freeman  B. Eng. Hons
 *
 */
#include 	"main.h"
#include	"Can_bus.hpp"
#include	"Serial.hpp"

#include	<cstdio>	//	sprintf

//#include	"Project.hpp"	//	Need this to turn on correct sections of interrupt handler
extern	void	ISR_can_is_led_signal_core	(CAN_RxHeaderTypeDef   CAN_RxHeader,uint8_t	* MyCAN_RxData,  int fifo)	;	//	NODE_IS_LED_SIGNAL
extern	CAN_HandleTypeDef 	hcan1;
extern	Serial				pc;

#define	CAN_RESET_ALL_ID	0x007	//	Arbitrary low number for high priority - 3 LSBs not filtered

CAN_RxHeaderTypeDef   CAN_RxHeader;

uint8_t	MyCAN_TxData[8] 	{};		//	Does initialise all to 0
uint8_t	MyCAN_RxData[2][8] 	{};		//	allow for both fifos

uint32_t	can_errors = 0;
uint32_t	cancount = 0;


bool	CAN_send_circ_buff::write	(int node, uint8_t * TxData, int len)	{	//	add some message to output buffer
	if	(full)
		return	(false);		//	Buffer is full, lose message, do not overwrite
	if	((len < 0) || (len > 8))
		return	(false);		//	Data length is impossible. Lose message, do not overwrite
								//	Passed length and full tests, proceed to buffer message
	ds[on_ptr].node	= node;		//	post increment pointer
	ds[on_ptr].len	= len;		//	write length of message 0 to 8 bytes
	while	(len)	{
		len--;
		ds[on_ptr].data[len] = TxData[len];
	}
	on_ptr++;					//	Message is on buffer, post increment pointer
	if	(on_ptr >= CANSENDBUFSIZ)
		on_ptr = 0;
	full =	(on_ptr == off_ptr);	//	If on and off pointers now equal, buffer is full
	empty = false;					//	Either way, buffer is not empty
	msg_que_cnt++;					//	One more message has been added to the queue -
	return	(true);					//	successfully !
}	//	returns false when buffer already full, or on length error


bool	CAN_send_circ_buff::send_any_queued	()	{	//	Check for queued messages, send one if there
	if	(empty)				//	Nothing to send at the moment
		return	(false);

//#define	TME0	(1<<26)	//	bit set when mailbox empty
//#define	TME1	(1<<27)
//#define	TME2	(1<<28)
	uint32_t	mboxnum {0};	//	HAL_CAN_AddTxMessage writes number of MailBox used here
	char	t[66];
	int		len;
	bool	rv;
	CAN_TxHeader.StdId 	= ds[off_ptr].node;	//	.IDE and .RTR constants, set in filters init
	CAN_TxHeader.DLC 	= ds[off_ptr].len;

	/**	HAL_CAN_AddTxMessage
	  * @brief  Add a message to the first free Tx mailbox and activate the
	  *         corresponding transmission request.
	  * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
	  *         the configuration information for the specified CAN.
	  * @param  pHeader pointer to a CAN_TxHeaderTypeDef structure.
	  * @param  aData array containing the payload of the Tx frame.
	  * @param  pTxMailbox pointer to a variable where the function will return
	  *         the TxMailbox used to store the Tx message.
	  *         This parameter can be a value of @arg CAN_Tx_Mailboxes.
	  * @retval HAL status
	  */
#define	CAN_SEND_FAIL_RETRY
	rv =	(HAL_OK == HAL_CAN_AddTxMessage(&hcan1, &CAN_TxHeader, ds[off_ptr].data, &mboxnum));	//	rv set true on success, false otherwise
	if	(!rv)	{												//	Can dig further into diagnostics here
		len = sprintf	(t, "HAL_CAN_AddTxMessage False\r\n");	//	But what to do when CAN message send fails ?
		pc.write	(t, len);									//	Ignore? Or keep message for retry?
#ifdef	CAN_SEND_FAIL_RETRY
		return	(false);
#endif
	}
	msg_que_cnt--;
	off_ptr++;
	if	(off_ptr >= CANSENDBUFSIZ)
		off_ptr = 0;
	full = false;
	empty =	(on_ptr == off_ptr);

#if 0
	len = sprintf	(t, "Pop CAN, que %ld, onptr %ld, offptr %ld, %s\r\n", msg_que_cnt, on_ptr, off_ptr, rv ? "T":"F");
		pc.write	(t, len);
#endif
	return	(rv);
}


CAN_send_circ_buff	can_send	;


bool	send_CAN_que_msg	()	;	//	Call this often from forever loop
bool	send_CAN_que_msg	()	{
	return	(can_send.send_any_queued())	;
}

//	End of Added Sept 2024



#ifdef	NODE_IS_CONTROLLER
bool	can_reset_all	()	{	//	Controller only issues this
	CAN_TxHeader.StdId = CAN_RESET_ALL_ID;		//
//	CAN_TxHeader.IDE = CAN_ID_STD;	//	CAN_ID_STD means that we are using the Standard ID (not extended)
//	CAN_TxHeader.RTR = CAN_RTR_DATA;	//	CAN_RTR_DATA - Sending a data frame, no response expected
	CAN_TxHeader.DLC = 0;			//	DLC is the Length of data bytes
	return (HAL_CAN_AddTxMessage(&hcan1, &CAN_TxHeader, MyCAN_TxData, &CAN_TxMailbox[0]) == HAL_OK);
}


bool	can_node_command	(int node, int command)	{	//	Controllers issue commands to peripherals
	if	((node < 1) || (node > 127) || (command < 0) || (command > 255))	{
		char t[64];
		int len = sprintf	(t, "Param error in can_node_command, node %d, command %d\r\n", node, command);
		pc.write	(t, len);
		return	(false);
	}
	CAN_TxLog[node]++;
	MyCAN_TxData[0] = command;
//	CAN_TxHeader.StdId = (0x200 | node);		//
	CAN_TxHeader.StdId = (0x300 | node);		//
//	CAN_TxHeader.IDE = CAN_ID_STD;	//	CAN_ID_STD means that we are using the Standard ID (not extended)
//	CAN_TxHeader.RTR = CAN_RTR_REMOTE;	//	CAN_RTR_REMOTE - Sending no data, YES, response expected
//	CAN_TxHeader.RTR = CAN_RTR_DATA;	//	CAN_RTR_DATA - Sending a data frame, response not expected
	CAN_TxHeader.DLC = 1;			//	DLC is the Length of data bytes
	return (HAL_CAN_AddTxMessage(&hcan1, &CAN_TxHeader, MyCAN_TxData, &CAN_TxMailbox[0]) == HAL_OK);
}
#endif



void	ce_show	()	{	//	works only with RxFifo0
	char	t[126];
	int	len;
#ifdef	NODE_IS_CONTROLLER	//	Defined in 'Project.hpp'
	pc.write	("CONTROLLER Node Type\r\n", 22);
#endif
#ifdef	NODE_IS_PERIPHERAL
	pc.write	("PERIPHERAL Node Type\r\n", 22);
#endif
#ifndef	NODE_IS_CONTROLLER
#ifndef	NODE_IS_PERIPHERAL
	pc.write	("No Node Type\r\n", 14);
#endif
#endif
	len = std::sprintf(t, "Sid 0x%lx, Eid 0x%lx, IDE 0c%lx, DLC 0x%lx, FMI 0x%lx, RTR 0x%ld\r\n",
			/*can_errors, cancount, */
			CAN_RxHeader.StdId,				//	Std ID of transmitter -
			CAN_RxHeader.ExtId, 			//	Not using extended ID
			CAN_RxHeader.IDE, 				//	specifies if we are using Standard ID or Extended ID CAN_ID_STD
			CAN_RxHeader.DLC, 				//	Length of data frame in bytes 0 to 8
			CAN_RxHeader.FilterMatchIndex, 	//
			CAN_RxHeader.RTR				//	Remote Transmission Request
			);
	pc.write(t, len);
	len = sprintf	(t, "MyCAN_RxData [0][0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x], cnt %ld, errs %ld\r\n"
		,	MyCAN_RxData[0][0]		,	MyCAN_RxData[0][1]		,	MyCAN_RxData[0][2]		,	MyCAN_RxData[0][3]
		,	MyCAN_RxData[0][4]		,	MyCAN_RxData[0][5]		,	MyCAN_RxData[0][6]		,	MyCAN_RxData[0][7]
		,	cancount		,	can_errors	 );
	pc.write(t, len);
}


int32_t	My_CAN_ID = 0;
bool	My_CAN_set = false;

void	can_filter_setup_core	()	;	//
void	can_filter_setup	(int32_t my_can_addr)	{	//
	My_CAN_ID = my_can_addr;
	My_CAN_set = true;
	can_filter_setup_core	();
}


void	can_filter_setup_core	()	{	//	was called from can init, now later in setup to get MyCANaddr from eeprom
	//  can_filter_setup();
	CAN_FilterTypeDef	canfilterconfig;	//	Need to configure filters here

	canfilterconfig.FilterActivation 		= CAN_FILTER_ENABLE;
	canfilterconfig.FilterMode 				= CAN_FILTERMODE_IDMASK;
	canfilterconfig.FilterScale 			= CAN_FILTERSCALE_32BIT;
	canfilterconfig.FilterIdLow 			= 0x0000;
	canfilterconfig.FilterMaskIdLow 		= 0x0000;			//	'0' do not test bit in this position

	canfilterconfig.FilterBank 				= 0;		//	Two new filters March 2024
	canfilterconfig.FilterIdHigh = CAN_RESET_ALL_ID << 5;
	canfilterconfig.FilterMaskIdHigh = 0x7f8 << 5;
	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);	//	Accepts 0x000 to 0x007 through filter 0

//	canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;
//	canfilterconfig.FilterFIFOAssignment = CAN_FILTER_FIFO1;

	canfilterconfig.FilterBank 				= 1;	//	Use for PING
//	canfilterconfig.FilterIdHigh = 0x0446 << 5;
//	canfilterconfig.FilterMaskIdHigh = 0x0444 << 5;	//	'1' bit to compare filter bit to incoming bit
	canfilterconfig.FilterIdHigh = ((My_CAN_ID & 0x1ff) | 0x100) << 5;		//0x100 to 0x17f		0x0100 << 5;
	canfilterconfig.FilterMaskIdHigh = 0x07ff << 5;	//	'1' bit to compare filter bit to incoming bit
	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);	//	filter 1

	canfilterconfig.FilterBank 				= 2;
	canfilterconfig.FilterIdHigh = My_CAN_ID << 5;	//	0x201 to 0x27f
	//	  canfilterconfig.FilterMaskIdHigh = 0x780 << 5;	//	Use 0x780 to ignore 7 LSB bits
	canfilterconfig.FilterMaskIdHigh = 0x7ff << 5;	//	Use 0x7ff to test all 11 bits
	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);

	canfilterconfig.FilterBank 				= 4;
	canfilterconfig.FilterIdHigh = (My_CAN_ID | 0x100) << 5;	//	0x301 to 0x37f
	//	  canfilterconfig.FilterMaskIdHigh = 0x780 << 5;	//	Use 0x780 to ignore 7 LSB bits
	canfilterconfig.FilterMaskIdHigh = 0x7ff << 5;	//	Use 0x7ff to test all 11 bits

////	canfilterconfig.FilterFIFOAssignment 	= CAN_RX_FIFO1;

	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);	//	Accepts 0x300 to 0x37f through filter 5

	canfilterconfig.FilterBank 				= 6;		//	System wide global commands
//	canfilterconfig.FilterIdHigh = 0x0123 << 5;		//	0x120 to 0x12f
	canfilterconfig.FilterIdHigh = 0x0680 << 5;		//	0x680 to 0x6ff
	canfilterconfig.FilterMaskIdHigh = 0x0780 << 5;
	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);	//	Accepts 0x680 to 0x6ff through filter 8

	canfilterconfig.FilterBank 				= 10;
	canfilterconfig.FilterIdHigh = 0x0700 << 5;
	canfilterconfig.FilterMaskIdHigh = 0x0700 << 5;
	HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);	//	Accepts 0x700 to 0x7ff through filter 16

//	CAN_TxHeader.IDE = CAN_ID_STD;	//	CAN_ID_STD means that we are using the Standard ID (not extended)
//	CAN_TxHeader.RTR = CAN_RTR_DATA;	//	CAN_RTR_DATA - Sending a data frame
//	CAN_TxHeader.TransmitGlobalTime = DISABLE;

	HAL_CAN_Start(&hcan1);

	if	(HAL_OK != HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING))
	  can_errors++;

	if	(HAL_OK != HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO1_MSG_PENDING))
	  can_errors++;
	if	(can_errors)
	  pc.write	("CAN errors in startup\r\n", 23);
}

#if 0

See "rm0432-stm32l4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf"
Section 55 Controller area network

Last Error Codes :
	000: No error
	001: Stuff error
	010: Form error
	011: Acknowledgment error
	100: Bit recessive error
	101: Bit dominant error
	110: CRC error
	111: Set by software
#endif
void	CAN_status_report	()	{	//	6th March 2024
	const	char	* lectab[] ={
			{"No Error"},
			{"Stuff error"},
			{"Form error"},
			{"Acknowledgement error"},
			{"Bit recessive error"},
			{"Bit dominant error"},
			{"CRC error"},
			{"Set by software"},
	}	;
	char	t[100];
	int	len;
	uint32_t	stat = CAN1->ESR;
	CAN->ESR	= 0x70;		//	Set last_err_code bits.
	uint32_t	tx_errs 		= (stat >> 16) & 0x0ff;
	uint32_t	rx_errs 		= (stat >> 24) & 0x0ff;
	uint32_t	last_err_code	= (stat >> 4) & 0x07;
	bool		boff			= (stat & 4);	//	Bus off
	bool		epvf			= (stat & 2);	//	Error passive flag
	bool		ewgf			= (stat & 1);	//	Error waning flag
	if	((tx_errs > 0) || (rx_errs > 0) || (last_err_code > 0) || boff || epvf || ewgf)	{
		pc.write("\r\nCAN Errors\r\n", 14);
		len = sprintf	(t, "tx_errors %ld\r\n", tx_errs);
		pc.write	(t, len);
		len = sprintf	(t, "rx_errors %ld\r\n", rx_errs);
		pc.write	(t, len);
		len = sprintf	(t, "%s\r\n", lectab[last_err_code]);
		pc.write	(t, len);
		len = sprintf	(t, "Bus off\t%s\r\n", boff ? "true" : "false");
		pc.write	(t, len);
		len = sprintf	(t, "Error passive\t%s\r\n", epvf ? "true" : "false");
		pc.write	(t, len);
		len = sprintf	(t, "Error warning\t%s\r\n", ewgf ? "true" : "false");
		pc.write	(t, len);

	}
	else	{
		len = sprintf	(t, "No known CAN errors\r\n");
		pc.write	(t, len);
	}
	len = sprintf	(t, "cancount %ld, can_errors %ld\r\n", cancount, can_errors);
	pc.write	(t, len);
}





void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)	//	Yes this one
{
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &CAN_RxHeader, &MyCAN_RxData[0][0]);
	cancount++;
//#ifdef	PROJECT_IS_SIGNALS_CONTROLLER
//	ISR_can_is_led_signal_controller	(0)	;
//#endif
//#ifdef	PROJECT_IS_CAN_LAMP
	ISR_can_is_led_signal_core	(CAN_RxHeader, &MyCAN_RxData[0][0], 0)	;	//	NODE_IS_LED_SIGNAL
//#endif
}

void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)	//	Yes this one
{
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO1, &CAN_RxHeader, &MyCAN_RxData[1][0]);
//	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO1, &CAN_RxHeader, MyCAN_RxData[0]);
	cancount++;
//#ifdef	PROJECT_IS_SIGNALS_CONTROLLER
//	ISR_can_is_led_signal_controller	(1)	;
//#endif
//#ifdef	PROJECT_IS_CAN_LAMP
	ISR_can_is_led_signal_core	(CAN_RxHeader, &MyCAN_RxData[1][0], 1)	;	//	NODE_IS_LED_SIGNAL
//#endif
}

void HAL_CAN_RxFifo0FullCallback(CAN_HandleTypeDef *hcan)	//	Not this one
{
	cancount++;
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &CAN_RxHeader, MyCAN_RxData[0]);	//	addr of 'MyCAN_RxData[0][0]'
//	stdid[0] = RxHeader.StdId;
}

void HAL_CAN_RxFifo1FullCallback(CAN_HandleTypeDef *hcan)	//	Not this one
{
	cancount++;
	HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO1, &CAN_RxHeader, MyCAN_RxData[1]);	//	addr of 'MyCAN_RxData[1][0]'
//	stdid[1] = RxHeader.StdId;
}


