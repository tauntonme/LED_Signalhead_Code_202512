/*
 * Utils.cpp	INTENDED TO BE PROJECT SPECIFIC
 *
 *  Created on: Feb 11, 2024
 *      Author: Jon Freeman  B. Eng. Hons
 *
 *	Latest changes 27/08/2025 - imported updated Serial
 *
 *	For menus, and functions executed through menus.
 *	Put such clutter here,
 *
 *	Keep 'Project.cpp' for the main logical flow of your project
 */
#include	<cstdio>			//	for sprintf
#include	<cstring>			//	for strlen

#include	"Project.hpp"
#include	"parameters.hpp"
#include	"Serial.hpp"
#include	"CmdLine.hpp"
#include	"settings.hpp"

//constexpr	char	const	version_str[] = "Info About Project Here," __DATE__;
constexpr	char	const	version_str[] = "Dual Multicolour CanLamp LED Driver, Jon Freeman, " __DATE__;
const 	char * 	get_version	()	{	//	Makes above available throughout code.
	return	(version_str);
}


//	Prototypes for functions included in 'settings_data' menu structure
bool	null_cmd	(parameters &);
bool	set_defaults_cmd	(parameters &);
bool	set_one_wrapper_cmd	(parameters &);

//enum	class	MenuType	{MENU, SETTINGS}	;	//	in 'parameters.hpp'

struct cli_menu_entry_set	const  settings_data[]
{    // Can not form pointers to member functions.
	{"?",     	"Lists all user settings, alters none", null_cmd, static_cast<int32_t>(MenuType::SETTINGS)}, //	Examples of use follow
	{"defaults","Reset settings to factory defaults", set_defaults_cmd},     //	restore factory defaults to all settings
	{"mca",		"My CAN Address", 		set_one_wrapper_cmd, 	0, 0x7ff, 6, 1.0},   //
	{"bootmode",  "Set Boot Mode ", 		set_one_wrapper_cmd, 	0, 5, 4, 1.0},   //
//	{"ramp",	"Low Volts Cutoff Ramp (bottom end) V times 10", set_one_wrapper_cmd,	100, 650, 220, 0.1},    //
//	{"range",	"Low Volts Cutoff Ramp Range, V times 10", 	set_one_wrapper_cmd,	1, 100, 20, 0.1}, //
//	{"chargedv","Bat volts required to start times 10", 	set_one_wrapper_cmd, 	200, 600, 260, 0.1},   //
	{nullptr},	//	June 2023 new end of list delimiter. No need to pass sizeof
}	;

//	Prototypes for functions included in 'pc_command_list' menu structure
bool	menucmd	(parameters &);
bool    ce_cmd (struct parameters & par)	;	//
bool	rtc_cmd	(parameters &);
bool	adc_cmd	(parameters &);
bool	st_cmd	(parameters &);
bool	sd_cmd	(parameters &);
bool	edit_settings_cmd	(parameters &);

bool	can_report_cmd	(parameters &)	;
bool	mycan_cmd	(parameters &)	;
bool	bpc_cmd	(parameters &);	//	Set brilliance percent
bool	rgbw_cmd	(parameters &);	//	Set brilliance of all, requires FOUR parameters
bool    set_runmode_cmd (struct parameters & )     ;
bool    get_mode_cmd (struct parameters & )     ;
bool    get_aspect_cmd (struct parameters & )     ;
bool    set_aspect_cmd (struct parameters & )     ;
bool	show_ambient_light_intensity	(struct parameters & par)	;
/**
struct  cli_menu_entry_set      const loco_command_list[] = {
List of commands accepted from external pc through non-opto isolated com port 115200, 8,n,1
*/
struct  cli_menu_entry_set	const pc_command_list[] = {
    {"?", "Lists available commands", 	menucmd, static_cast<int32_t>(MenuType::MENU)},
	{"?m", "Get Mode", get_mode_cmd},
	{"?a", "Get Aspect", get_aspect_cmd},
	{"li", "Get Ambient Light Intensity", show_ambient_light_intensity},
	{"asp", "Set Aspect", set_aspect_cmd},
	{"rtc", "real time clock buggery", 	rtc_cmd},
#ifdef	USING_ANALOG
	{"adc", "check adc dma working", 	adc_cmd},
#endif
	{"mycan", "Show My Can Addr 0x", 	mycan_cmd},
	{"canrep", "Test for CAN bus errors", 	can_report_cmd},
	  {"ce", "can errors", ce_cmd},
	{"rm", "run_mode, default 0", 		set_runmode_cmd},
	{"bpc", "Bril percent 0 - 100", 		bpc_cmd},
	{"rgbw", "Set R, G, B, (W) 0 - 9 - 3 or 4 params", 		rgbw_cmd},
	{"st", "real time clock Time", 		st_cmd},
	{"sd", "real time clock Date", 		sd_cmd},
	{"us", "user settings", 			edit_settings_cmd},
    {"nu", "do nothing", null_cmd},
    {nullptr},	//	June 2023 new end of list delimiter. No need to pass sizeof
}   ;


//	************* Create Utilities *****************
extern	UART_HandleTypeDef	huart2;	//	uarts used in this project
extern	I2C_HandleTypeDef 	hi2c1;	//	I2C

Serial				pc(huart2);		//, * Com_ptrs[];
i2eeprom_settings	my_settings	(settings_data, hi2c1)	;	//	Create one i2eeprom_settings object named 'j_settings'
CommandLineHandler	command_line_handler	(pc_command_list, &pc);	//	Nice and clean

//#define	COM_PORT	pc


extern	void	ce_show	()	;

//bool	find_word_in_list	(const struct cli_menu_entry_set * list, char * word, int & position)	{
//extern	bool	test_fwil (struct parameters & a)     ;
extern	bool	edit_settings (struct parameters & a)     ;	//	Here from CLI

extern	void	show_lamp_mode	()	;
bool    get_mode_cmd (struct parameters & )     {
	show_lamp_mode	()	;
	return	(true);
}

extern	int		get_lamp_aspect	()	;
extern	bool	set_lamp_aspect	(int)	;

bool	get_aspect_cmd	(struct parameters & par)	{
	get_lamp_aspect	();
	return	(true);
}

bool	set_aspect_cmd	(struct parameters & par)	{
	set_lamp_aspect	((int)par.flt[0]);
	return	(true);
}


bool	ce_cmd (struct parameters & a)     {
	ce_show();
    return	(true);
}

extern	bool	set_lamp1_bril	(float)	;
extern	bool	set_lamp2_bril	(float)	;

bool	bpc_cmd	(parameters & par)	{	//	Set brilliance percent 0 to 100, checked downstream
	set_lamp1_bril	(((float)par.flt[0]) / 100.0);
	return	(set_lamp2_bril	(((float)par.flt[0]) / 100.0));
}


extern	bool	set_lamp_colours	(int r, int g, int b, int w)	;

bool	rgbw_cmd	(parameters & par)	{	//	Set brilliance of all, requires FOUR parameters
//	char	t[40];
//	int	len;
	set_lamp_colours	((int)par.flt[0], (int)par.flt[1], (int)par.flt[2], (int)par.flt[3]);
	return	(true);
}


bool	mycan_cmd	(parameters &)	{
	float	f = 0.0;
	int32_t	iv = 0;
	char	t[40];
	int	len;
	my_settings.read1	("mca", iv, f);
	len = sprintf	(t, "My CAN Address 0x%3lx\r\n", iv);
	pc.write	(t, len);
	return	(true);
}


extern	void	CAN_status_report	()	;	//	6th March 2024
bool	can_report_cmd	(parameters & par)	{
	CAN_status_report	()	;	//	6th March 2024
	return	(true);
}



/**
*   void    menucmd 		(struct parameters & a)
*	void	list_settings	(const menu_entry_set * list)
*   List available terminal commands to pc terminal. No sense in touch screen using this
*/
void	list_settings	(const cli_menu_entry_set * list)	{
	int i = 0;
	int len;
	int32_t	ival;
	float	fval;
	char	t[200];
	char	ins_tab[2] {0,0};

//	extern 	char * 	get_version	();//	{	return	(version_str);	}
	pc.write	(get_version(), strlen(get_version()));
	pc.write	("\r\n", 2);

	len = sprintf	(t, "Listing %s Functions and Values :\r\n", list[0].min ? "SETTINGS" : "MENU");
	pc.write	(t, len);
	while	(list[i].cmd_word)	{
		(6 > strlen(list[i].cmd_word)) ? ins_tab[0] = '\t' : ins_tab[0] = 0;
		len = sprintf	(t, "[%s]\t%s%s"
			, list[i].cmd_word
			, ins_tab
			, list[i].description	);
		pc.write	(t, len);	//	This much common to MENU and SETTINGS
		if	(list[0].min)	{	//	is SETTINGS, not MENU
			if	(my_settings.read1(list[i].cmd_word, ival, fval))	{
				(6 > strlen(list[i].cmd_word)) ? ins_tab[0] = '\t' : ins_tab[0] = 0;
				len = sprintf	(t, "\tmin%ld, max%ld, def%ld\t[is %ld]\t(float mpr %.2f)"
					, list[i].min
					, list[i].max
					, list[i].de_fault
					, ival
					, fval	);
				pc.write	(t, len);
			}
			else	pc.write	("Settings Read Error\r\n", 21);
		}	//	Endof is SETTINGS, not MENU
		pc.write	("\r\n", 2);
		i++;
	}	//	Endof 	while	(list[i].cmd_word)
	pc.write("End of List of Commands\r\n", 25);
}


bool    menucmd (struct parameters & par)     {
	list_settings	(par.command_list)	;
    return	(true);
}


bool	edit_settings_cmd (struct parameters & par)     {	//	Here from CLI having found "us "
	bool	rv =	(my_settings.edit	(par));
	list_settings	(settings_data)	;
	return	(rv)	;
}


bool    set_one_wrapper_cmd (struct parameters & par)     {	//	Called via edit, a.second_word found in edit
	return	(my_settings.set_one	(par));
}


bool    null_cmd (struct parameters & par)     {
	const char t[] = "null command - does nothing useful!\r\n";
//	COM_PORT.write(t, strlen(t));
	pc.write(t, strlen(t));
    return	(true);
}


bool    set_defaults_cmd (struct parameters & par)     {
	return	(my_settings.set_defaults());
}


//extern	bool	set_lamp_mode	(LampUseModes m)	;	//	Accessible only via 'Utils.cpp' serial commands
extern	bool	set_lamp_mode	(int32_t m)	;	//	Accessible only via 'Utils.cpp' serial commands
bool    set_runmode_cmd (struct parameters & par)     {
	bool	rv;
	char	t[66];
	int	len;
	rv = set_lamp_mode ( (int32_t)par.flt[0]);
	len = sprintf	(t, "set_lamp_mode %.1f, %s\r\n", par.flt[0], rv ? "Good" : "Bad");
	pc.write	(t, len);

	return	(rv);
}


void	check_commands	()	{	//	Called from ForeverLoop
/**
 * bool	Serial::test_for_message	()	{
 *
 * Called from ForeverLoop at repetition rate
 * Returns true when "\r\n" terminated command line has been assembled in lin_inbuff
 */
//	char * buff_start = COM_PORT.test_for_rx_message();
	char * buff_start = pc.test_for_rx_message();
	if	(buff_start)
		command_line_handler.CommandExec(buff_start);
}


extern	uint32_t	can_errors;
extern	void	rtc_buggery	()	;
extern	void	adc_cnt_report	()	;

bool	adc_cmd	 (struct parameters & par)     {
#ifdef	USING_ANALOG
	adc_cnt_report();
#endif
	return	(true);
}


extern	uint32_t	auto_intensity_percent	()	;	//
extern	"C" uint32_t	get_adcval	(size_t	i)	;

bool	show_ambient_light_intensity	(struct parameters & par)	{
	char	t[66];
	size_t	len = sprintf	(t, "Light Intens = %ld, set to pct %ld\r\n", get_adcval((size_t)par.flt[0]), auto_intensity_percent());
	pc.write	(t, len);
	return	(true);
}




#ifdef	USING_RTC
extern	bool	set_time	(struct parameters & par)	;
extern	bool	set_date	(struct parameters & par)	;

bool	st_cmd	 (struct parameters & par)     {
	return	(set_time	(par));
}


bool	sd_cmd	 (struct parameters & par)     {
	return	(set_date	(par));
}

bool	rtc_cmd	 (struct parameters & par)     {
	rtc_buggery();
	return	(true);
}


#else
bool	st_cmd	 (struct parameters & par)     {
	return	(false);
}


bool	sd_cmd	 (struct parameters & par)     {
	return	(false);
}


bool	rtc_cmd	 (struct parameters & par)     {
	return	(false);
}


#endif






