/*
 * Lamp.cpp
 *
 *  Created on: Sep 7, 2024
 *      Author: Jon Freeman  B. Eng. Hons
 *
 *      Latest changes 21/04/2025	(checked 27/08/2025 JF)
 *
 *      See Lamp.hpp for hardware detail info
 */

#include	"Lamp.hpp"
#include	"Serial.hpp"

#include	<cstdio>	//	need for sprintf

extern	Serial	pc;			//	For test / debug only

/**
 * 		This is where we Create a single LED_Lamp object named 'Lamp'
 */
	LED_Lamp	Lamp2	((uint32_t&)(TIM2->CCR1)	//	Parameters are pointers to pwm timer registers
						,(uint32_t&)(TIM2->CCR2)
						,(uint32_t&)(TIM2->CCR3)
						,(uint32_t&)(TIM2->CCR4)
						);

	LED_Lamp	Lamp1	((uint32_t&)(TIM15->CCR1)	//	Parameters are pointers to pwm timer registers
						,(uint32_t&)(TIM15->CCR2)
						,(uint32_t&)(TIM1->CCR1)
						,(uint32_t&)(TIM1->CCR2)
						);

/*
enum	class	LampUseModes	{
	NO_LAMP_USE,
	RAILWAY_SIGNAL,
	MOOD_LIGHT,
	NORMAL_LAMP,
	NUMOF_LAMP_USE_MODES = NORMAL_LAMP
}	;
 */
const char	*	const	LampUseModesText[]  {
		  {(char*)"NO_LAMP_USE"}
		, {(char*)"RAILWAY_SIGNAL"}
		, {(char*)"MOOD_LIGHT"}
		, {(char*)"NORMAL_LAMP"}
		, {(char*)"WB_DEMO"}
		, {(char*)"NUMOF_LAMP_USES"}
}	;
/*
enum	class Lamp_Commands	{
	NO_COMMAND,
	SET_SIGNAL_ASPECT,
	SET_BRIL_PERCENT,
	NUMOF_LAMP_COMMANDS = SET_BRIL_PERCENT
}	;
*/
const char	*	const	LampCmdsText[]  {
		  {(char*)"NO_COMMAND"}
		, {(char*)"SET_SIGNAL_ASPECT"}
		, {(char*)"SET_BRIL_PERCENT"}
		, {(char*)"NUMOF_LAMP_COMMANDS"}
};
	/*
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
	RGBW_PERCENT,			//	Odd one out, 4 parameters to set R, G, B and W
	NUMOF_SIG_STATES = RGBW_PERCENT
}	;//	States to which LED signals may be set. Semaphores to show DANGER when not set to CLEAR
	//	Note Occulting is flash where on_time > off_time. See https://www.railforums.co.uk/threads/flashing-signals.186537/
*/

struct	signal_states_data	{
	const char * const txt;
	const uint8_t  rgbw[4];
};

struct	signal_states_data constexpr	SigStatesData	[]	{	//r  g  b  w percent 0 to 100
		{  (char*)"BLACK",				{0, 0, 0, 0}}	//	all leds off
		, {(char*)"DANGER",				{100, 0, 0, 0}}	//	red leds only. Check for 0<=x<=100 done later
		, {(char*)"CAUTION",			{80, 20, 00, 0}}
		, {(char*)"CLEAR",				{0, 100, 0, 0}}	//	green leds only
		, {(char*)"BLUE",				{0, 0, 100, 0}}
		, {(char*)"WHITE",				{0, 0, 0, 100}}
		, {(char*)"OCCULTING_YELLOW",	{50, 50, 0, 0}}
		, {(char*)"CYAN",				{0, 50, 50, 0}}
		, {(char*)"MAGENTA",			{50, 0, 50, 0}}
		, {(char*)"NUMOF_SIG_STATES",	{1, 1, 1, 1}}
}	;

LampUseModes	SigHeadMode = LampUseModes::NO_LAMP_USE;

//	Functions called from other code to access Lamp


extern		uint32_t	auto_intensity_percent	()	;	//
void	LampCore	()	{	//	Here at forever loop repeat rate (30Hz?, 50Hz? around there)
	static	size_t	loop_cnt { 0L };
	static	size_t	wb_mode_state { 0L };
	static	SignalStates	ss1 = SignalStates::DANGER;
	static	SignalStates	ss2 = SignalStates::BLACK;
//	loop_cnt++;
//	Lamp.timebase();	//	Lamp2.timebase();
	Lamp1.timebase();	//	Lamp2.timebase();
	Lamp2.timebase();	//	Lamp2.timebase();
	if	(SigHeadMode == LampUseModes::WB_DEMO)	{

		float tmpf = (float)auto_intensity_percent	()	/ 100.0;
		Lamp1.set_bril	(tmpf);
		Lamp2.set_bril	(tmpf);
		if	((loop_cnt++ % 500) == 0)	{
			wb_mode_state++;
			if	(wb_mode_state > 2)
				wb_mode_state = 0;
			switch	(wb_mode_state)	{
			case	0:
				ss1 =	(SignalStates::DANGER);
				ss2 =	(SignalStates::BLACK);
				pc.write	("0 DAN BLK\r\n", 11);
				break;
			case	1:
				ss1 =	(SignalStates::CAUTION);
				ss2 =	(SignalStates::BLACK);
				pc.write	("1 CAU BLK\r\n", 11);
				break;
			case	2:
				ss1 =	(SignalStates::BLACK);
				ss2 =	(SignalStates::CLEAR);
				pc.write	("2 BLK CLR\r\n", 11);
				break;
			default:
				ss1 =	(SignalStates::DANGER);
				ss2 =	(SignalStates::DANGER);
				pc.write	("x DAN DAN\r\n", 11);
				break;
			}
		}
		Lamp1.set_aspect	(ss1);
		Lamp2.set_aspect	(ss2);
	}
}


bool	set_lamp1_bril	(float	b)	{	//	Called from utils 'bpc' bril percent command. Use 0.0 to 1.0 here
	return	(Lamp1.set_bril	(b));	//	Lamp2.set_bril	(b);
}

bool	set_lamp2_bril	(float	b)	{	//	Called from utils 'bpc' bril percent command. Use 0.0 to 1.0 here
	return	(Lamp2.set_bril	(b));	//	Lamp2.set_bril	(b);
}


const char *	get_mode_txtptr	(int m)	{
	int numof_options = sizeof (LampUseModesText)/ sizeof(char *);
//	char	t[32];
//	int	len = sprintf	(t, "Numof options = %d\r\n", numof_options);
//	pc.write	(t, len);
	if	((m >=0) && (m < numof_options))
		return	(LampUseModesText[m]);
	return	("get_mode_txtptr failed");
}

void	show_lamp_mode	()	{
	char	t[22];
	int	len, rv;
	LampUseModes	tmp = Lamp1.get_mode ();
	rv = static_cast<int>(tmp);
	len = sprintf	(t, " %d, ", rv);
	pc.write	(t, len);
	pc.write	(LampUseModesText[rv], strlen(LampUseModesText[rv]));
	pc.write	("\r\n", 2);
}


bool	set_lamp_aspect	(int asp)	{
	return	(Lamp1.set_aspect(static_cast<SignalStates>(asp)));
}


int		get_lamp_aspect	()	{
	char	t[22];
	int	len;
	int	rv = static_cast<int>(Lamp1.get_aspect ());
	len = sprintf	(t, " %d, ", rv);
	pc.write	(t, len);
	pc.write	(SigStatesData[rv].txt, strlen(SigStatesData[rv].txt));
	pc.write	("\r\n", 2);
	return	(rv);
}


bool	set_lamp_mode	(LampUseModes m)	{	//	Accessible only via 'Utils.cpp' serial commands
	SigHeadMode = m;
		(Lamp1.set_mode (m));
	return	(Lamp2.set_mode (m));
}


bool	set_lamp_mode	(int32_t m)	{	//	Accessible only via startup and 'Utils.cpp' serial commands
	return	set_lamp_mode	(static_cast<LampUseModes>(m));
}


void	set_lamp_colours	(int r, int g, int b, int w)	{
	Lamp1.rgbw	(r, g, b, w);
}

//	End of Functions called from other code to access Lamp


LED_Lamp::LED_Lamp	(uint32_t & r, uint32_t & g, uint32_t & b, uint32_t & w)	//	Constructor
	:	pwm_r 	{&r}	//	Initialise four constant pointers
	,	pwm_g	{&g}
	,	pwm_b	{&b}
	,	pwm_w	{&w}
	{	}	;			//	Empty constructor function body. Leave pwms as setup in STM3Cube startup code


/*int		LED_Lamp::get_mode()	{
	return	static_cast<int>(mode);
}*/

LampUseModes		LED_Lamp::get_mode()	{
	return	mode;
}


bool	LED_Lamp::set_mode	(LampUseModes m)	{
	if	((m <= LampUseModes::NO_LAMP_USE) || (m > LampUseModes::NUMOF_LAMP_USE_MODES))
		return	(false);
	mode = m;
	return	(true);
}	;	//	Accessible only via 'Utils.cpp' serial commands


constexpr	uint8_t	wb_demo_states_data []	{	//r  g  b  w percent 0 to 100
		 	 100, 0, 0, 0	,0 ,0 ,0 ,0	//	bottom red
		, 	 0, 0, 0, 0		,50 ,50 ,0 ,0	//	top yellow
		, 	 0, 0, 0, 0		,0 ,100 ,0 ,0	//	top green
}	;

void	LED_Lamp::timebase()	{	//	Call this at forever loop repeat rate
//	static	uint32_t	wb_mode_state { 0L }	;
//	float		tmpf;
	loop_cnt++;
	switch	(mode)	{

	case	LampUseModes::RAILWAY_SIGNAL:
		if	(bril_updated)	{
			bril_updated = false;
			rgbw	(last_colours_percent);
		}
		if	(occulting_mode)	{
			bool	occult_on_off_flag = ((HAL_GetTick() % (OCCULT_ON_MS + OCCULT_OFF_MS)) < OCCULT_ON_MS);
			if	(occult_flag != occult_on_off_flag)	{	//	do something
				occult_flag = occult_on_off_flag;		//	prevent pointless repeat, act only on transitions
				if	(occult_flag)	{	//	Turn light to Yellow
					rgbw(last_colours_percent);		//	Restore latest non black
				}
				else	{				//	Turn light to Black
					off();
				}
			}
		}		//	Endof if (occulting_mode)
		break;

	case	LampUseModes::MOOD_LIGHT:
		mood_update	();
		break;

	case	LampUseModes::NORMAL_LAMP:
		break;
	case	LampUseModes::WB_DEMO:
		break;
	default:
		break;
	}
}


bool	LED_Lamp::set_bril(float	b)	{
	bool	rv { true } ;
	if	(b < 0.0)	{	b = 0.0;	rv = false;	}
	if	(b > 1.0)	{	b = 1.0;	rv = false;	}
	user_brilliance_setting = b;
	bril_updated = true;
	return	(rv);
}



void	LED_Lamp::rgbw	(const uint8_t *const percent)	{	//	New Sept 2024
//	static float	constexpr	bril_const_factor	{ (((float)1024) / 100.1) };	//	htim1.Init.Period = 1024;
	static float	constexpr	bril_const_factor	{ (((float)60000) / 100.1) };	//	htim1.Init.Period = 65000; provides safety margin
	float	adjust = user_brilliance_setting * bril_const_factor;	//	Only use of 'bril_const_factor'
	float	sum {0.0};
	float	la[4] {0.0};
	for	(int i = 0; i < 4; i++)	{
		if	(percent[i] > 100)
			last_colours_percent[i] = 100;					//	Forced any rogues into range 0 to 100
		else
			last_colours_percent[i] = percent[i];
		la[i] = (float)	last_colours_percent[i];
		sum += la[i];
	}
	//	sum in range 0 to 400 here, but LED rated for one LED @ 100%, all 4 @ 25% etc pro rata
	sum /= 100.0;		//	normalise sum of brils to 0.0 to 4.0
	if	(sum > 1.0)		//	Users demand would overload and risk damaging LEDs
		adjust /= sum;	//	Factor now scaled to not allow LED device to be over-driven
	*pwm_r	= (uint32_t)(la[0] * adjust);
	*pwm_g	= (uint32_t)(la[1] * adjust);
	*pwm_b	= (uint32_t)(la[2] * adjust);
	*pwm_w	= (uint32_t)(la[3] * adjust);
}


void	LED_Lamp::rgbw	(uint8_t r, uint8_t g, uint8_t b, uint8_t w)	{
	uint8_t	colours[4] = {r, g, b, w};
	rgbw	(colours);
}


void	LED_Lamp::off	()	{	//	Useful in flash or occult. All LEDs off without resetting 'last_colours_pct[]'
	*pwm_r	= 0L;	*pwm_g	= 0L;	*pwm_b	= 0L;	*pwm_w	= 0L;
}


void	LED_Lamp::set_occulting	(int on_ms, int off_ms)	{
	if	((on_ms == 0) || (off_ms == 0))
		occulting_mode = false;
	else	{
		occulting_mode = true;
	}
}


/**
 * bool	LED_Lamp::set_aspect	(SignalStates  asp)	;
 *
 * RAILWAY_SIGNAL is what this hardware was designed for. Other uses are incidental.
 * Therefore, using 'set_aspect' will force unit mode to 'LampUseModes::RAILWAY_SIGNAL'
 */
bool	LED_Lamp::set_aspect	(SignalStates  asp)	{
	bool	rv { true } ;
	if	((asp < SignalStates::BLACK) || (asp > SignalStates::NUMOF_SIG_STATES))	{
		pc.write	("Invalid SignalState in set_aspect\r\n", 35);
		return	(false);
	}
#if 0
	if	(mode !=	LampUseModes::RAILWAY_SIGNAL)	{
		pc.write	("Forcing LampUseModes::RAILWAY_SIGNAL\r\n", 38);
		if	(!(set_mode	(LampUseModes::RAILWAY_SIGNAL)))	{
			pc.write	("set_mode error in set_aspect\r\n", 30);
			return	(false);
		}
	}
#endif
#if 0
	char	t[66];
	int	len = sprintf	(t, "%ld, %ld\r\n", htim1.Init.Period, htim2.Init.Period);
	pc.write	(t, len);		//	Assume htim1.Init.Period == htim2.Init.Period
#endif
	last_aspect	=	asp;	//	last_aspect is global

	if	(last_aspect == SignalStates::OCCULTING_YELLOW)
		set_occulting	(OCCULT_ON_MS, OCCULT_OFF_MS);	//	Occulting On
	else
		set_occulting	(0, 0);							//	Occulting Off

	rgbw	(SigStatesData[static_cast<int>(last_aspect)].rgbw)	;

	return	(rv);
}

/*
bool	LED_Lamp::command	(Lamp_Commands cmd, uint8_t * RxData)	{
	switch	(cmd)	{
	case	Lamp_Commands::NORMAL_LAMP:
		mode = LampUseModes::NORMAL_LAMP;
		break;

	case	Lamp_Commands::MOOD_LIGHT:
		mode = LampUseModes::MOOD_LIGHT;
		break;

	case	Lamp_Commands::RAILWAY_SIGNAL:
		mode = LampUseModes::RAILWAY_SIGNAL ;
		break;

	default:
		break;
	}
	return	(true);
}*/

bool	LED_Lamp::command	(uint8_t * RxData)	{
	bool	rv { true } ;
	Lamp_Commands	cmd = static_cast<Lamp_Commands>(RxData[0]);		//	Command byte

	switch	(cmd){	//MyCAN_RxData[fifonum][0]

	case	Lamp_Commands::SET_SIGNAL_ASPECT:		//MyCAN_RxData[fifonum][0]	Set Aspect
		set_aspect	(static_cast<SignalStates>(RxData[1]))	;	//	Set all 4 led drivers accordingly taking 'bril' into account
		break;

	case	Lamp_Commands::SET_BRIL_PERCENT:		//MyCAN_RxData[fifonum][0]	Set Brilliance 0 to 100 %
		set_bril	(((float)RxData[1]) / 100.0);	//	cnc	node 2 50 sets bril to 50%
		break;

/*	case	Lamp_Commands::SET_RUN_MODE:		//MyCAN_RxData[fifonum][0]	set_run_mode to normal, i.e. switch off auto-change
		//	cnc node 3 0	normal mode
		//	cnc node 3 4	mood lighting
		set_run_mode	(RxData[1]);
		break;

	case	Lamp_Commands::MOOD_LIGHT:
		*cmd_dat = static_cast<uint8_t>(Lamp_Commands::MOOD_LIGHT);
//		command	(Lamp_Commands::MOOD_LIGHT, nullptr);
		break;

	case	Lamp_Commands::NORMAL_LAMP:
		command	(Lamp_Commands::NORMAL_LAMP, nullptr);
		break;
*/
	default:		//MyCAN_RxData[fifonum][0]	Set DANGER (reset)
		set_aspect	(SignalStates::DANGER)	;	//	Default to DANGER
		rv = false;
		break;
	}
//	pc.write	(RxData, 8);
	return	(rv);
}


