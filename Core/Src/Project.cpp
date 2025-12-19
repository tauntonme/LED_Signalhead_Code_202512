/*
 * Project.cpp
 *
 * Project Non-Library Functions
 *
 *  Created on: Apr 15, 2025
 *      Author: Jon Freeman  B. Eng. Hons MIET
 *
 *	Keep this file, 'Project.cpp', for the main logical flow of your project
 *
 *
 *
 */
#include	<cstdio>			//	for sprintf
#include	<cstring>			//	for strlen
#include	<cmath>

#include	"Project.hpp"
#include	"Lamp.hpp"
#include	"CAN_bus.hpp"
#include	"parameters.hpp"
#include	"Serial.hpp"
#include	"CmdLine.hpp"
#include	"settings.hpp"

extern	Serial	pc;
extern	CAN_send_circ_buff	can_send	;
extern	LED_Lamp	Lamp;
extern	void	LampStart	()	;
extern	i2eeprom_settings	my_settings;
extern	CommandLineHandler	command_line_handler;	//	(pc_command_list);	//	Nice and clean

extern	const char * get_version();

extern	const char *	get_mode_txtptr	(int m)	;
bool	set_lamp_mode	(int32_t m)	;	//	Accessible only via startup and 'Utils.cpp' serial commands


bool	System_Setup24	()	{	//	Here at start of programme.
	char	t[100];
	int		len;
	bool	rv = true;
	const char * vs = get_version()	;	//	In Utils.cpp
	pc.start_rx();
	pc.write("\r\n\n\n\n", 5);
	pc.write(vs, strlen(vs));
	pc.write("\r\n\n", 3);

	int32_t	seti = 0;
	float	mpy = 0.0;
	if	(!(my_settings.load()))	{
		pc.write	("EEPROM Read Failed\r\n", 20);
		rv = false;
	}
	else	{	//	Reading settings from eeprom good
		if	(my_settings.read1	("mca", seti, mpy))	{	//	Read My CAN address
			len = sprintf	(t, "Got My CAN Address - %04lx\r\n", seti);
//			pc.write	(t, len);
		}
		else	{		//	read_one_setting failed
			rv = false;
		}
		  extern	void  can_filter_setup(int32_t);
		  can_filter_setup(seti);	//	Also starts CAN
		if	(!rv)
			len = sprintf	(t, "Failed to read 1 setting\r\n");
		pc.write	(t, len);
	}
	if	(my_settings.read1("bootmode", seti, mpy))	{
		len = sprintf	(t, "Got from eeprom, mode = %ld, %s\r\n", seti, (char*)get_mode_txtptr(seti));
		//	set modes from here
		set_lamp_mode	(seti);
	}
	else	{
		len = sprintf	(t, "my_settings.read1 failed\r\n");
	}
	pc.write	(t, len);

//	len = sprintf	(t, "Got EE vals %lx, %lx, %lx, %lx, %lx\r\n", );
	len = sprintf	(t, "System Startup, health %s\r\n", rv ? "Good" : "Bad");
	pc.write	(t, len);
//	const char * tp = get_mode_txtptr	(seti)	;
//	pc.write	(tp, strlen(tp));
	return	(rv);
}					//	End of bool	System_Setup	()	;


extern	"C" uint32_t	get_adcval	(size_t	i)	;

size_t	get_ambient_light_intensity	()	{
	return	((size_t)	get_adcval	(0));	//	0 as using only 1 adc chan
}

uint32_t	auto_intensity_percent	()	{	//		Lookup table to determine lamp brightness in relation to ambient light level
	uint32_t	constexpr	intensity_lut	[] {
			20		//	Darkest
			, 25
			, 30
			, 35
			, 40
			, 45
			, 55
			, 65
			, 75
			, 82
			, 90
			, 99	//	Brightest
	}	;
	size_t	constexpr	tab_size = (sizeof(intensity_lut) / sizeof(uint32_t)) - 1;
	size_t	constexpr	brightest_sensor_output = 30000;
//	size_t	constexpr	sens_divider = brightest_sensor_output / tab_size;

	size_t	incident_intens = get_adcval(0);	//	With 470R optosensor load, use 30000 max intens, 0 dark
	incident_intens *= tab_size;
	incident_intens /= brightest_sensor_output;
//	incident_intens /= 3000;	//	leaves range 0 - 10
	if	(incident_intens > tab_size)
		incident_intens = tab_size;
	return	(intensity_lut[incident_intens]);
}

extern	void	LampCore	()	;

void	ProjectCore	()	{	//	Here at forever loop repeat rate (30Hz?, 50Hz? around there)

	LampCore	()	;

}		//	End of function ProjectCore()

void	ISR_can_is_led_signal_core	(CAN_RxHeaderTypeDef   CAN_RxHeader,
		uint8_t	* MyCAN_RxData, int fifonum)	{	//	!! Called in Interrupt Context !!
	uint8_t	dumdat[8] {0};
	char	t[52];
	int	len;
//	pc.write	("Q", 1);
//	len = sprintf	(t, "fifo %d, filindex %ld, id 0x%lx\r\n", fifonum, CAN_RxHeader.FilterMatchIndex, CAN_RxHeader.StdId);
//	pc.write	(t, len);
//	char	cmd = MyCAN_RxData[fifonum][0];		//	Command byte
//	CAN_Commands	cmd = static_cast<CAN_Commands>(MyCAN_RxData[fifonum][0]);		//	Command byte
	//uint8_t	last_aspect = static_cast<uint8_t>(SignalStates::DANGER);	//	GLOBAL
//	SignalStates	new_state = static_cast<SignalStates>(MyCAN_RxData[fifonum][1]);		//	Data byte
//	len = sprintf(t, "Got CAN 0x%lx, len %ld, filt %ld, fifo %d\r\n", CAN_RxHeader.StdId, CAN_RxHeader.DLC, CAN_RxHeader.FilterMatchIndex, fifonum);
//	pc.write	(t, len);
	switch	(CAN_RxHeader.FilterMatchIndex)	{
	case	0:	//CAN_RxHeader.FilterMatchIndex	System reset codes 0 to 7 as 3 lsbs of StdId
		len = sprintf	(t, "Global Reset Command %ld\r\n", CAN_RxHeader.StdId);
		pc.write	(t, len);
		break;
	case	1:		//CAN_RxHeader.FilterMatchIndex	PING request.  If controller, need to put this on list or something
		dumdat[2]++;
		if	(0 == dumdat[2])
			dumdat[1]++;
		can_send.write(CAN_RxHeader.StdId, dumdat, 5);
		len = sprintf	(t, "Got PING request 0x%lx, data 0x%x, 0x%x\r\n", CAN_RxHeader.StdId, dumdat[2], dumdat[1]);
		pc.write	(t, len);
		break;

//	case	3:	//CAN_RxHeader.FilterMatchIndex	Code for response required
	case	5:	//CAN_RxHeader.FilterMatchIndex	Code for response required
//		response_required = true;
		len = sprintf	(t, "Got 'filter 5, fifo%d', response required %lx\r\n", fifonum, CAN_RxHeader.StdId);
		pc.write	(t, len);

//		set_param	(cmd, new_state, &MyCAN_RxData[fifonum][0])	;

//bool	send_can_msg	(int node, uint8_t * TxData, int len)	{
//		MyCAN_TxData[0] = cmd;
		dumdat[0] = MyCAN_RxData[0];
//		MyCAN_TxData[1] = get_last_aspect();
		dumdat[1] = 0x55;
//		if	(HAL_OK != (send_can_msg	(CAN_RxHeader.StdId, MyCAN_TxData, 6)))
//				pc.write	("BadCAN5\r\n", 9);
		can_send.write(CAN_RxHeader.StdId, dumdat, 6);
		can_send.write(CAN_RxHeader.StdId, dumdat, 6);	//	repeats give greater chance of picoscope capture
//		can_send.write(CAN_RxHeader.StdId, dumdat, 6);
//		can_send.write(CAN_RxHeader.StdId, dumdat, 6);	//	Too many, causes "HAL_CAN_AddTxMessage False\r\n" due to only 3 Mailboxes
		break;

	case	2:	//CAN_RxHeader.FilterMatchIndex	Code for response not required
		//	if	(CAN_RxHeader.RTR == CAN_RTR_DATA)	{			//	New command arrived from controller
//		set_param	(cmd, new_state, &MyCAN_RxData[fifonum][0])	;
		Lamp.command	( &MyCAN_RxData[0])	;	//	10th Sepr 2024 - only call
//		pc.write	("Z", 1);
		break;

	default:	//CAN_RxHeader.FilterMatchIndex
		len = sprintf	(t, "Got UNHANDLED request %lx, filter %ld\r\n", CAN_RxHeader.StdId, CAN_RxHeader.FilterMatchIndex);
		pc.write	(t, len);
		break;
	}	//	CAN FD does not support Remote Frame
}



