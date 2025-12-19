/*
 * ForeverLoop.cpp
 *
 *  Created on: Jun 6, 2023
 *      Author: Jon Freeman B Eng Hons
 *
 *		Latest changes 19/12/2025 - using SysTick, updated Serial
 *      Earlier Latest changes 21/04/2025	(checked 27/08/2025 JF)
 */
#include 	"main.h"

#include	<cstdio>


#include	"Project.hpp"
#include	"Serial.hpp"
#include	"CmdLine.hpp"
#include	"microsecs.hpp"

extern	Serial	pc;

extern	void	check_commands	()	;	//	Looks after all command line input

bool	eeprom_valid_flag = false;


#define	FOREVER_LOOP_REPEAT_MS	20
#define	SLOW_LOOP_REPEAT_MS		500

extern	bool	adc_updates	()	;

uint32_t	sigs_time;


void	do_even_halfsec_stuff	()	{
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);	//	led off
}

void	do_odd_halfsec_stuff	()	{
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);		//	led on
}


void	do_fastest_stuff	()	{
	extern	bool	send_CAN_que_msg	()	;
	send_CAN_que_msg	()	;
	check_commands	()	;			//	Read and execute any commands rec'd from pc
#ifdef	USING_ANALOG
	if	(adc_updates	())	{	//	Checks flags, updates averaging buffers when data available
//		CHARGE_PUMP;			//	Macro in Project.hpp
	}
#endif
}

my_microsecond_timer	tim_us	;	//	see microsecs.hpp

extern	void	ProjectCore	()	;
//extern	bool	can_respond	()	;	//	Peripherals respond using this

void	do_forever_loop_rate_stuff	()	{
	ProjectCore	()	;

//	can_respond	()	;	//	Do faster than this, once per millisec

/*	if	(can_flag)	{
		can_flag = false;
//		ce_show	();
	}*/
	tim_us.restart	();

//		signals_engine	();

	sigs_time = tim_us.read();
}

void	one_ms_stuff	()	{
	pc.tx_any_buffered();
}


void	do_slow_loop_rate_stuff	()	{
	static	bool	oddeven;
	oddeven = !oddeven;
	if	(oddeven)
		do_even_halfsec_stuff	();
	else
		do_odd_halfsec_stuff	();
}


extern	bool	System_Setup24	()	;	//	see 'Project.cpp', possibly the best place for this
extern	bool	ticked	()	;	//	Returns true if SysTick has ticked since previous call

extern "C" void	ForeverLoop	()	{	// Jumps to here from 'main.c'

//	bool	health = System_Setup24	();
	System_Setup24	();

	while	(true)	{				//	Always forever loop top
		do_fastest_stuff	();		//	While waiting for timer to sync slower stuff
		if	(ticked())	{
			one_ms_stuff	();		//	Do all the once per millisec stuff
			if	((HAL_GetTick() % FOREVER_LOOP_REPEAT_MS) == 0)	{
				do_forever_loop_rate_stuff	();
				if	((HAL_GetTick() % SLOW_LOOP_REPEAT_MS) == 0)	{
					do_slow_loop_rate_stuff	();					//	once per half second
				}	//	endof if	(slow_loop_timer > SLOW_LOOP_REPEAT_MS)
			}		//	endof if	(forever_loop_timer >= FOREVER_LOOP_REPEAT_MS)
		}			//	endof if	(timer_1ms_flag)
	}				//	endof while	(true)
}					//	endof void	ForeverLoop(). Never gets to here, function never returns.


