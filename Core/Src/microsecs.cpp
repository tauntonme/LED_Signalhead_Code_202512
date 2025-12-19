/*
 * microsecs.cpp
 *
 *  Created on: Nov 12, 2025
 * 	Author	-	Jon Freeman  B Eng Hons MIET
 *
 *
 *	For use with STM32CubeIDE, put this file in Project/Core/Src/
 */

#include	"microsecs.hpp"

/**
 * 	bool	ticked	()
 * 	Returns true if a SysTick has occurred since last call to ticked()
 *
 * 	Use this e.g. to sync in forever_loop - if(ticked())	{	do regular stuff etc	}
 */
bool	ticked	()	{
	static	uint32_t	old_millisecs { 0L }	;
	uint32_t	latest = uwTick;	//HAL_GetTick();
	if	(old_millisecs == latest)
		return	(false);
	old_millisecs = latest;
	return	(true);
}


void	my_microsecond_timer::read_systick	(int	idx)	{	//	reads SysTick registers. idx = 0 start time, idx !=0 end time
	if	(idx != 0)	//	saves testing of idx for out of range
		idx = 1;
	do	{
		millisecs[idx] = uwTick;			//	millisecs since system powerup / reset
		rawsticks[idx] = SysTick->VAL;		// / (SystemCoreClock / 1000000);	//	24 bit downcounter clocked at SystemCoreClock
	}	while	(millisecs[idx] != uwTick)	;	//	Ensures no SysTick during this read, avoids rare 1ms errors
}


void	my_microsecond_timer::restart	()	{	//	reads and stores start time
	read_systick	(0);	//	Set new start integer millisecs and fractional part (microsecs * (SystemCoreClock / 1000000))
}


uint32_t	my_microsecond_timer::read	()	{	//	good only for periods up to about 71 minutes
	uint32_t	rv { 0L }	;
	uint32_t	us_diff	;
	read_systick	(1);	//	Set new end integer millisecs and fractional part (microsecs * (SystemCoreClock / 1000000))
	us_diff = (1 + SysTick->LOAD	//	1000 microseconds worth to ensure coming division always positive, so works as signed/unsigned
				+ rawsticks[0]		//	start time, negative due to SysTick being a down counter
				- rawsticks[1]		//	end time, neg as above
			  )
				/ (SystemCoreClock / 1000000);	//	diff + 1000 us makes it always positive
	rv = (millisecs[1] - millisecs[0]) - 1;	//	whole SysTick millisecs since most recent start, less the 1000us added to us_diff
	rv *= 1000	;	//	millisecs to microsecs conversion - rv may overflow if time > about 71 minutes
	rv += us_diff;
	return	(rv);
}

