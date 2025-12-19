/*
 * Lamp.hpp
 *
 *  Created on: Sep 7, 2024. Revised 18th April 2025
 *      Author: Jon Freeman  B. Eng. Hons MIET
 *
 *      class	LED_Lamp
 *
 *      Driver of four coloured (red green blue white) LED module used for signalling at
 *      West Buckland Miniature Railways. Also good for mood, and general lighting applications.
 *
 *      see	-	https://tauntonme.github.io/img/LED_Driver23_sch.png
 *      see	-	https://tauntonme.github.io/img/LED_Driver25_sch.png
 *
 *      LEDs are driven using AL8863SP-13 chips, see https://www.diodes.com/assets/Datasheets/AL8863.pdf
 *      Brightness of each colour controlled by 1 of 8 PWMs from STM32L431CBT6 controller.
 *      AL8863 data suggests PWM frequency "below 500Hz".
 *
 *	Used with 2025 Led_Driver Board which drives two lights.
 *
 *	Class	LED_Lamp controls one light.  Software may instantiate e.g. Lamp1 and Lamp2
 *
 *	Use : - To create object My_Light_1
 *
 *	Led_Lamp	My_Light_1	(t_red, t_grn, t_blu, t_whi)	;
 *
 *		where 't_' parameters are pointers to ARM Cortex timer registers TIMx->CCRy(1 to 4)
 *		Timers are configured as PWM (pulse width modulators)
 *		Some timers have four channels of PWM, making one timer per light a sensible option.
 *		However, to ease any PCB routing difficulties the four PWMs do not have to be from the same timer.
 *		Where different timers are used, keep prescaler and setup values the same.
 *
 *		Original prototype used one light split between TIM1 and TIM2 due to pinout limitations
 *		using Nucleo L432KC module.
 *		Subsequent designs use 'bare metal' e.g. STM32L431CBU6 (48 pins) or STM32L431RBT6 (64 pins)
 *		Dual lamp board uses 4 chans of TIM2, 2 chans of TIM1, and 2 chans of TIM15
 *
 */

#ifndef INC_LAMP_HPP_
#define INC_LAMP_HPP_
#include	"main.h"
#include	<cmath>
#include	<cstdio>			//	for sprintf
#include	<cstring>			//	for strlen

constexpr	double	PI 		{ (4.0 * atan2(1.0, 1.0)) };
constexpr	double	TWO_PI 	{ (8.0 * atan2(1.0, 1.0)) };

enum	class	LampUseModes	{
	NO_LAMP_USE,	//	defaults to all black
	RAILWAY_SIGNAL,
	MOOD_LIGHT,
	NORMAL_LAMP,
	WB_DEMO,
	NUMOF_LAMP_USE_MODES = WB_DEMO
}	;

enum	class Lamp_Commands	{
	NO_COMMAND,
	SET_SIGNAL_ASPECT,
	SET_BRIL_PERCENT,
	NUMOF_LAMP_COMMANDS = SET_BRIL_PERCENT
}	;

/*char	*	const	SigStatesText[]  {
					  {(char*)"BLACK"}
					, {(char*)"DANGER"}
					, {(char*)"CAUTION"}
					, {(char*)"CLEAR"}
					, {(char*)"BLUE"}
					, {(char*)"WHITE"}
					, {(char*)"OCCULTING_YELLOW"}
					, {(char*)"CYAN"}
					, {(char*)"MAGENTA"}
					, {(char*)"RGBW_PERCENT"}
					, {(char*)"NUMOF_SIG_STATES"}
};*/

enum	class SignalStates	{
	BLACK,	//	Uses 4 bits of space in tolog ?? Need to look further at this - see below
	DANGER,					//	binary 0 0 1
	CAUTION,				//	binary 0 1 0
	CLEAR,					//	binary 0 1 1
	BLUE,					//	binary 1 0 0
	WHITE,					//	binary 1 0 1
	OCCULTING_YELLOW = 6,	//	binary 1 1 0	in tolog, lose msb. Thus CAUTION and OCCULTING report same.
	CYAN,
	MAGENTA,
	NUMOF_SIG_STATES = MAGENTA
}	;//	States to which LED signals may be set. Semaphores to show DANGER when not set to CLEAR
	//	Note Occulting is flash where on_time > off_time. See https://www.railforums.co.uk/threads/flashing-signals.186537/

/**
class	LED_Lamp	{

*/
class	LED_Lamp	{
	float	user_brilliance_setting {0.1};		//	0.0 to 1.0	defaults to 10% brightness
//	LampUseModes	mode { LampUseModes::RAILWAY_SIGNAL };
//	LampUseModes	mode { LampUseModes::MOOD_LIGHT };
	LampUseModes	mode { LampUseModes::NO_LAMP_USE };
	volatile uint32_t * const pwm_r;		//	Pointers are constant, points to TIMx->CCR(1 to 4)
	volatile uint32_t * const pwm_g;
	volatile uint32_t * const pwm_b;
	volatile uint32_t * const pwm_w;	//	e.g. volatile uint32_t * const pwm_z {&(TIM1->CCR1)};
	uint32_t	loop_cnt	{ 0L }	;
	uint8_t		last_colours_percent	[4]	{0};
	SignalStates	last_aspect {SignalStates::DANGER} ;	//	Signal powers up defaulting to DANGER
	bool		occulting_mode	{ false } ;
	bool		occult_flag		{ false } ;
	bool		bril_updated	{ false } ;
	class	mood_lighting_specifics	{
	public:
		struct	colour_variables	{
			const double	delta { 0.0 } ;		//	angle to add at loop rate
			double			theta { 0.0 } ;		//	cumulative angle
			int				bril_percent;
		}
			red 	{0.01009,	0.0},	//	1009	499
			green 	{0.01013,	0.0},	//	1013	503
			blue	{0.01019,	0.0},	//	1019	509
			white	{0.01021,	0.0};	//	1021	521
		void	modulate_led	(struct colour_variables & c)	{
			c.theta += c.delta;
			if	(c.theta > PI)
				c.theta -= TWO_PI;
//			c.bril_pct = ((int)(50.0 + (49.99 * cos(c.theta))));
			c.bril_percent = ((int)(25.0 + (24.99 * cos(c.theta))));
		}	;
		void	colour_shift_all	()	{
			modulate_led	(red);
			modulate_led	(green);
			modulate_led	(blue);
			modulate_led	(white);
		}
	}	mood	;
public:
	LED_Lamp	(uint32_t & timr, uint32_t & timg, uint32_t & timb, uint32_t & timw)	;	// Constructor
	void	rgbw	(const uint8_t *const percent)	;	//	New Sept 2024					//	Parameters 0 to 100 percent in uint8_t array
	void	rgbw	(uint8_t red, uint8_t grn, uint8_t blu, uint8_t whi)	;	//	Parameters 0 to 100 percent in four uint8_t s
	void	off			()	;									//	All LEDs off without resetting 'last_colours_pct'
//	bool	command		(Lamp_Commands cmd, uint8_t * RxData)	;
	bool	command		(uint8_t * RxData)	;					//	Pointer to CAN bus RxData array
	bool	set_bril	(float normalised_bril)	;				//	0.0 to 1.0 Returns false if ((bril < 0.0) || (bril > 1.0))
	bool	set_aspect	(SignalStates aspect)	;				//	Returns false on invalid aspect error
	void	set_occulting	(int on_ms, int off_ms)	;
	bool	set_mode	(LampUseModes m)	;	//	Accessible only via 'Utils.cpp' serial commands
//	int		get_mode	();
	LampUseModes		get_mode	();
	void	timebase	()	;	//	Call this at forever loop repeat rate
	void	mood_update	()	{	//	mood lighting only
		mood.colour_shift_all	()	;
		rgbw	(mood.red.bril_percent, mood.green.bril_percent, mood.blue.bril_percent, mood.white.bril_percent);
	}	;
	SignalStates	get_aspect	()	{	return	(last_aspect)	;	}
}	;


extern	TIM_HandleTypeDef htim1;	//	from 'main.c'
extern	TIM_HandleTypeDef htim2;

#define	OCCULT_ON_MS	250		//	Occulting signal on for this many milli seconds
#define	OCCULT_OFF_MS	150		//	Occulting signal off for this many milli seconds

#endif /* INC_LAMP_HPP_ */
