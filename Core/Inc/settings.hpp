/*
 * settings.hpp
 *
 *  Created on: Jan 7, 2024
 *      Author: Jon Freeman B Eng Hons
 *
 */
#define	EEROM_SETTINGS_COUNT	25	//	varies with number of settings, **Can create lists longer than this!!**
#if 0
	class	i2eeprom_settings	used with user generated list of 'struct cli_menu_entry_set' objects,
	see 'parameters.hpp' for struct cli_menu_entry_set.
	End of table delimited using 'nullptr' in cmd_word element of last entry.
		e.g.
struct  cli_menu_entry_set const  settings_data[EEROM_SETTINGS_COUNT]
	{    // Can not form pointers to member functions.
		{"0",     	"Lists all user settings, alters none", 	null_cmd}, //	Examples of use follow
		{"board",	"Board ID ascii '1' to '9'", 				board_cmd,	'1', '9', '0'}, //  BOARD_ID    defaults to '0' before eerom setup for first time 12
		{"defaults","Reset settings to factory defaults", 		set_defaults_cmd},     //	restore factory defaults to all settings
		{"ramp",	"Low Volts Cutoff Ramp (bottom end) V times 10", set_one_wrapper_cmd,	100, 650, 220, 0.1},    //
		{"range",	"Low Volts Cutoff Ramp Range, V times 10", 	set_one_wrapper_cmd,	1, 100, 20, 0.1}, //
		{"chargedv","Bat volts required to start times 10", 	set_one_wrapper_cmd, 	200, 600, 260, 0.1},   //
		{"lvsecs",	"Bat Lo for this time before WARNING", 		set_one_wrapper_cmd, 	2, 60, 6, 1.0},   //
		{nullptr},	//	June 2023 new end of list delimiter. No need to pass sizeof
	}	;

	i2eeprom_settings	MySettings	(settings_data, I2C_HandleTypeDef &hi2cn)	;
#endif
#include	"parameters.hpp"

class	i2eeprom_settings	{
	const	cli_menu_entry_set * const set_list {};	//	Pointer to Table built in user project code
	I2C_HandleTypeDef * myi2c;
	int32_t 	ivalues[EEROM_SETTINGS_COUNT + 2] {0};  	//  These get loaded with stored values read from EEPROM
	float	 	fvalues[EEROM_SETTINGS_COUNT + 2] {0.0};  	//  These get loaded with stored values read from EEPROM
	uint32_t	set_list_position {0};			//	internal use only
	bool		m_eeprom_status { false };	//	Set true only in 'load' if eeprom found good
	bool	write_eeprom	(uint32_t address, const char *buffer, uint32_t size)	const	;
	bool	fcwi_set_list	(const char * wrd, uint32_t & position)	;	//	Used in 'edit' and 'read1'
	bool	find_word_in_string	(const char * s_str, const char * targ_wrd, uint32_t & posn)	;
	const int		list_len	()const	;
  public:
	i2eeprom_settings	(struct cli_menu_entry_set const * list
			, I2C_HandleTypeDef &hi2cn)	//	User built table, see e.g. above
		:	set_list	{list}
		,	myi2c		{&hi2cn}
	{	//	Constructor
	}	;
	bool	set_one	(struct parameters & par);	//	This is only way to write a setting value
	bool	save	()	const;	//	Write settings to EEPROM
	bool	load	()	;	//	Load settings from EEPROM
	bool	edit	(struct parameters & par)	;	//	edit one setting value if found
	bool	set_defaults	()	;				//	All settings to factory defaults
    bool	read1 (const char *, int32_t & ival, float & fval)	;  //  Read one setup int32 and one float mpy value
}	;


