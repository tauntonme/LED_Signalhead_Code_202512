/*
 * microsecs.hpp
 *
 *  Created on: Nov 12, 2025
 * 	Author	-	Jon Freeman  B Eng Hons MIET
 *
 *	millisecond and microsecond timing functions
 *
 *	For use with STM32CubeIDE, put this file in Project/Core/Inc/
 *
 * 	bool	ticked	()
 * 		Eliminates need for timer_1ms_flag by reading from SysTick
 * 		Returns true if a SysTick has occurred since last call to ticked()
	bool	ticked	()	{


 * 	Class	"my_microseconds_timer"
 * 	Useful for timing events, measuring short intervals to sub-millisecond resolution
 *	Use uint32_t ms = HAL_GetTick(); to read millisecs since reset
 */

#ifndef INC_MICROSECS_HPP_
#define INC_MICROSECS_HPP_

#include	"main.h"

class	my_microsecond_timer	{		//	Use to create microsecond resolution event timing capability
	uint32_t	millisecs[2]	{ 0L };
	uint32_t	rawsticks[2]	{ 0L };	//	Stores raw SysTick->VAL, which is microsecs * (SystemCoreClock / 1000000)

	void	read_systick	(int	idx);

public	:
	my_microsecond_timer	()	{	read_systick(0);	}	;	//	Constructor, sets start time upon instantiation
	void		restart		()	;	//	Reads 32 bit uwTick and 24 bit SysTick->VAL
	uint32_t	read		()	;	//	Returns microseconds since restart(). Good only for periods up to about 71 minutes
}	;


#endif /* INC_MICROSECS_HPP_ */

