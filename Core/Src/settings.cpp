/*
 * settings.cpp
 *  Created on: Jun 13, 2023
 *      Author: Jon Freeman B Eng Hons
 */
/*	Non-volatile "settings" variables stored in external EEPROM 24LC64
 * 	One 'int32_t' saved in EEPROM per 'setting'
 *
 * 	"settings.cpp / .hpp" evolved from Electronic Motor Control projects.
 * 				Using 'i2eeprom_settings', e.g.
 *
 *	i2ceeprom_settings	MySettings	(cli_menu_entry_set const * MyList);
 *
 *	where 'MyList' identifies your list of settings, example below
 *
 *struct  cli_menu_entry_set const  MyList[]
 *	{    // Can not form pointers to member functions.
 *		{"0",     	"Lists all user settings, alters none", null_cmd}, //	Examples of use follow
 *		{"board",	"Board ID ascii '1' to '9'", board_cmd,	'1', '9', '0'}, //  BOARD_ID    defaults to '0' before eerom setup for first time 12
 *		{"defaults","Reset settings to factory defaults", set_defaults_cmd},     //	restore factory defaults to all settings
 *		{"ramp",	"Low Volts Cutoff Ramp (bottom end) V times 10", set_one_wrapper_cmd,	100, 650, 220, 0.1},    //
 *		{"range",	"Low Volts Cutoff Ramp Range, V times 10", set_one_wrapper_cmd,	1, 100, 20, 0.1}, //
 *		{"chargedv",	"Bat volts required to start times 10", set_one_wrapper_cmd, 	200, 600, 260, 0.1},   //
 *		{"lvsecs",	"Bat Lo for this time before WARNING", set_one_wrapper_cmd, 	2, 60, 6, 1.0},   //
 *		{nullptr},	//	June 2023 new end of list delimiter. No need to pass sizeof
 *	}	;
 *
 *	'cli_menu_entry_set' defined in 'parameters.hpp'
 *
 *	Reworked Feb 2024
 *	Settings now brought into line with Command Line Interpreter
 *	us numbers replaced by text, e.g. "us defaults".
 *	In use, initial handler from CLI extracts word following "us", e.g. "defaults".
 *	This word is searched in 'set_list'
 *	If match found at position 'n', then function at nth position in set_list is executed, as for CLI.
 *	This should prove easier to maintain as well as being more user intuitive.
 */

#include	<cctype>
#include	<cstring>
#include	<cstdio>

#include	"parameters.hpp"
#include	"settings.hpp"	//	settings only alterable from Command Line

//#define	LM75_ADDR	0x90	//	Temperature sensor (optional). List other i2c devices here
#define	LC64_ADDR	0xa0	//	EEPROM memory
#define	EEPROM_PAGE_SIZE	32	//	Can write to EEPROM only this many bytes per write (page write)
#define	SETTINGS_BASE_ADDRESS	1024	//	To fit around any other uses you may have for EEPROM

//extern	I2C_HandleTypeDef hi2c1;	//	I2C


/*
template	<typename T>	//	This is untested
bool	in_range	(T value, T position)	{
	return	((value >= set_list[position].min) && (value <= set_list[position].max));
}
*/
const int		i2eeprom_settings::list_len	()const	{
	int	i = 0;
	while	(set_list[i].cmd_word)
		i++;
	return	(i)	;
}


#include	"Serial.hpp"
extern	Serial	pc;
bool	i2eeprom_settings::set_one	(struct parameters & par)	{	//	????search for any 2nd word in user input command
	int32_t	new_val = (int32_t)par.flt[0];	//	no, this has been done in edit, this func called from edit
	pc.write	("At set_one\r\n", 12);
	if	(par.numof_floats != 1)	{
		return	(false);
	}
	if	(new_val == ivalues[set_list_position])
		return	(true);	//	setting unchanged, nothing to do
	if	((new_val >= set_list[set_list_position].min)
			&& (new_val <= set_list[set_list_position].max))	{
		ivalues[set_list_position] = new_val;
		save	();	//	Write modified settings to EEPROM
		return	(true);
	}
	pc.write	("In set_one, Bad Settings Value\r\n", 32);
	return	(false);
}


/**	bool	find_word_in_string	(const char * s_str, const char * targ_wrd, uint32_t & posn)	;
 *
 * Searches search string 's_str' for first occurrence of target string "targ_wrd"
 * Returns 'true' if target found, with 'posn' set to start position within search string
 * Returns 'false' if target not found.
 *
 * Checks for non-alpha leading and trailing characters to prevent finding e.g. "and" in "wander"
 *
 */

bool	i2eeprom_settings::find_word_in_string	(const char * s_str, const char * targ_wrd, uint32_t & posn)	{
	uint32_t	s_len = strlen	(s_str);
	uint32_t	w_len = strlen	(targ_wrd);
	uint32_t	max_poss_positions;
if	(w_len <= s_len)
	{	//	Target word can not be longer than search string, return false if it is
		max_poss_positions = s_len - w_len;
		for	(uint32_t i = 0; i <= max_poss_positions; i++)	{//	'i' is search start position in s_str
			if	((strncmp	(s_str + i, targ_wrd, w_len) == 0)	//	Found, (but may have found "and" in "wander")
				&&	((i == 0) || !(isalpha(s_str[i - 1])))		//	and any char before is not alpha
				&&	(			 !(isalpha(s_str[i + w_len]))) )	{//	and any char after is not alpha
					posn = i;
					return	(true);
			}	//	endof if	((strncmp	(s_str + i, targ_wrd, w_len) == 0) && .. && .. )
		}	//		endof for	(uint32_t i = 0; i < max_poss_positions; i++)
	}	//			endof if	(w_len <= s_len)
	return	(false);
}

/*
bool	fwistest	(struct parameters & a)	{
	uint32_t	pis;	//	position in string
	find_word_in_string	("wander", "and", pis);
	find_word_in_string	("wo ander", "and", pis);
	find_word_in_string	("wand er", "and", pis);
	find_word_in_string	("wo and er", "and", pis);
	find_word_in_string	("first second word", "first", pis);
	find_word_in_string	("first second word", "second", pis);
	find_word_in_string	("first second word", "word", pis);
	find_word_in_string	("first second word", "second word", pis);
	find_word_in_string	("first second word", "first second ", pis);
	find_word_in_string	("freda", "freda", pis);
	find_word_in_string	("freda", "zfredaz", pis);
	find_word_in_string	("zfredaz", "freda", pis);
	pc.write	("\r\n", 2);
	return	(true);
}
*/

/**	bool	i2eeprom_settings::fcwi_set_list	(const char * word, uint32_t & position)	{
 * 	fcwi_set_list	find_command_word_in_set_list
 *
 *	CLI and 'settings' menu data now has unified structure, being a list of
 *	 "struct cli_menu_entry_set" structures.
 *	 First entry of each struct is "cmd_word", the '?', 'menu' or whatever text input by user to invoke action.
 *
 *	Returns false if word not found in list, int position unaffected
 *	Returns true if match is found, int position set to position in list
 */
bool	i2eeprom_settings::fcwi_set_list	(const char * wrd, uint32_t & position)	{//Used in 'edit' and 'read1'
	int i = 0;
	uint32_t	junk;	//	not interested in how far along string found. Need position in list, not line
	while	(set_list[i].cmd_word)	{
		if	(find_word_in_string(set_list[i].cmd_word, wrd, junk))	{
			position = i;
			return	(true);
		}
		i++;
	}
	return	(false);
}


//	bool	i2eeprom_settings::edit    (struct parameters & a) {
//	Redirected Here from CLI having found	"us "
//	Returns true on success, false if any 'second word' on command line not found or not recognised

//	Extract any word following "us " or other command word
//	Test for word in list
//	Update setting if apt
bool	i2eeprom_settings::edit    (struct parameters & par) {	//	Redirected Here from CLI having found "us "
//	bool	rv = eeprom_status;
	char	t[100];
	int	len = sprintf	(t, "%sAt settings::edit with [%s]\r\n", m_eeprom_status ? "" : "Bad or Missing EEPROM! - ", par.command_line);
	pc.write	(t, len);
	uint32_t i = 0;
	const char * p = par.command_line;	//	start of user input
	//	Read any second word entered on the command line
	par.second_word[0] = 0;


	if	(!(m_eeprom_status))	{	//
//		pc.write	("Can't set, EEPROM bad or missing\r\n", 23);
//|PUT BACK		return	(false);
	}

	while	(isalpha(*p))	{	p++;	}	//	read over "cmdwrd "	//	User setting commands begin "us "
	while	(isspace(*p))	{	p++;	}	//	read over " "
	while	(isalpha(*p) && *p && (i < MAX_2ND_WORD_LEN))	{	//	assemble word, to max length
		par.second_word[i++] = *p++;
		par.second_word[i] = 0;
	}				//	Have extracted any second word [fred] from "anycommand fred 4" etc user command
	if	(i == 0)	{	//	length of second word found
		return	(false);
	}	//	To pass here a second word was found, written to a.second_word[]
	len = sprintf	(t, "Found 2nd word [%s]\r\n", par.second_word);
	pc.write	(t, len);
	if	(fcwi_set_list	(par.second_word, set_list_position)) {	//	Sets 'set_list_position' if match found
		par.function_returned = set_list[set_list_position].func(par)	;	//	Execute function, returns bool
		return	(true);
	}
	return	(false);
}



bool	i2eeprom_settings::set_defaults    () {         //  Put default settings into EEPROM and local buffer
//void	i2eeprom_settings::set_defaults    () {         //  Put default settings into EEPROM and local buffer
	bool	rv;
    for (unsigned int i = 0; set_list[i].cmd_word; i++)  {
        ivalues[i] = set_list[i].de_fault;       //  Load defaults and 'Save Settings'
        fvalues[i] = ((float)ivalues[i]) * set_list[i].mpy;
    }
    rv =  save    ();	//    rv ? pc.write	("save GOOD\r\n", 11) : pc.write	("save BAD!", 11);
    return	(rv);
}


bool	i2eeprom_settings::read1  (const char * which, int32_t & ival, float & fval)	{	//  Read one setup char value from private buffer 'settings'
	uint32_t pos_in_list {0};	//	Could also report position in list
	if	(fcwi_set_list	(which, pos_in_list))	{
		ival = ivalues[pos_in_list];
		fval = fvalues[pos_in_list];
//		mult = set_list[pos_in_list].mpy;
		return	(true);
	}
	return	(false);
}


/*
KEEP THIS FOR REFERENCE
bool	i2eeprom_settings::read_eeprom	(uint32_t address, const char *buffer, uint32_t size)	{
	HAL_StatusTypeDef	ret;	//	Used to test return results of HAL functions
	uint8_t	addr[4];
	addr[0] = address >> 8;
	addr[1] = address & 0xff;
	ret = HAL_I2C_Master_Transmit	(&hi2c1, LC64_ADDR, addr, 2, 10);	//	write 2 byte address
	if	(ret != HAL_OK)
		return	(false);
	HAL_Delay(1);
  	ret = HAL_I2C_Master_Receive	(&hi2c1, LC64_ADDR, (uint8_t*)buffer, size, 100);
	if	(ret != HAL_OK)
		return	false;
	return	(true);
}*/


bool	i2eeprom_settings::write_eeprom	(uint32_t address, const char *buffer, uint32_t size)	const	{
		HAL_StatusTypeDef	ret;	//	Used to test return results of HAL functions
		uint8_t	values[40];
		// Check the address and size fit onto the chip.
		if	((address + size) >= 8192)
			return	(false);			//	Fail - Attempt to write something too big to fit
	    const char *page = buffer;
	    uint32_t 	left = size;
	    // While we have some more data to write.
	    while (left != 0) {
	        // Calculate the number of bytes we can write in the current page.
	        // If the address is not page aligned then write enough to page
	        // align it.
	        uint32_t toWrite;
	        if ((address % EEPROM_PAGE_SIZE) != 0) {
	            toWrite = (((address / EEPROM_PAGE_SIZE) + 1) * EEPROM_PAGE_SIZE) - address;
	            if (toWrite > size) {
	                toWrite = size;
	            }
	        } else {
	            if (left <= EEPROM_PAGE_SIZE) {
	                toWrite = left;
	            } else {
	                toWrite = EEPROM_PAGE_SIZE;
	            }
	        }
	        //printf("Writing [%.*s] at %d size %d\n\r", toWrite, page, address, toWrite);
	        // Start the page write with the address in one write call.
	        values[0] = /*(char)*/(address >> 8);	//	these are uint8_t
	        values[1] = /*(char)*/(address & 0x0FF);

	        for (uint32_t count = 0; count != toWrite; ++count)
	        	values[count + 2] = *page++;
	        ret = HAL_I2C_Master_Transmit	(myi2c, LC64_ADDR, values, toWrite + 2, 100);	//	write 2 byte address followed by n data
	        if	(ret != HAL_OK)
	        	return	(false);

	        HAL_Delay(5);			//        waitForWrite();
	        left -= toWrite;        // Update the counters with the amount we've just written
	        address += toWrite;
	    }
		return	(true);
}



/**bool	i2eeprom_settings::load	()	{
 *
 * Reads int32 settings values from EEPROM into ivalues[x]
 * If eeprom error or missing, loads defaults (25 Feb 2024)
 * Calculates and fills float fvalues[x] using 'fvalues[i] = ((float)ivalues[i]) * set_list[i].mpy;'
 */
bool	i2eeprom_settings::load	()	{
	uint8_t		tmp[4] = {0};
	m_eeprom_status = true;
	tmp[0] = (SETTINGS_BASE_ADDRESS >> 8);
	tmp[1] = (SETTINGS_BASE_ADDRESS & 0xff);
	HAL_StatusTypeDef	ret = HAL_I2C_Master_Transmit	(myi2c, LC64_ADDR, tmp, 2, 10);	//	write 2 byte address
	if	(ret != HAL_OK)
		m_eeprom_status = (false);
	HAL_Delay(1);
//	ret = HAL_I2C_Master_Receive	(&hi2c1, LC64_ADDR, (uint8_t*)values, OPTION_COUNT * 4, 100);
	ret = HAL_I2C_Master_Receive	(myi2c, LC64_ADDR, (uint8_t*)ivalues, list_len() * 4, 100);
	if	(ret != HAL_OK)	{
		m_eeprom_status = (false);
	}
	for	(int i = 0; i < list_len(); i++)	{
		if	(!m_eeprom_status)
			ivalues[i] = set_list[i].de_fault;		//	If eeprom problem load defaults (25 Feb 2024)
		fvalues[i] = ((float)ivalues[i]) * set_list[i].mpy;
	}
	return	(m_eeprom_status);
}


bool	i2eeprom_settings::save	()const	{
	char * buff = (char *)ivalues;
//	return	(write_eeprom	(SETTINGS_BASE_ADDRESS, buff, OPTION_COUNT * 4));	//	4 = sizeof(int32_t)
	return	(write_eeprom	(SETTINGS_BASE_ADDRESS, buff, list_len() * 4));	//	4 = sizeof(int32_t)
}

//	Useful only if LM75 temperature sensor connected
#ifdef	USING_LM75
bool	read_LM75_temperature	(float & temperature)	{	//	Can not detect sensor. Check by temperature reported for sanity
	HAL_StatusTypeDef	ret;	//	Used to test return results of HAL functions
	uint8_t	i2c_tx_buff[10];
	uint8_t	i2c_rx_buff[10];
	  i2c_tx_buff[0] = i2c_tx_buff[1] = 0;	//	pointer to temperature data
	  ret = HAL_I2C_Master_Transmit	(myi2c, LM75_ADDR, i2c_tx_buff, 1, 10);
	  if	(ret != HAL_OK)
		  return	(false);
	  int16_t tmp = 0;	//	i2c send addr to LM75 worked
	  ret = HAL_I2C_Master_Receive	(myi2c, LM75_ADDR, i2c_rx_buff, 2, 10);
	  if	(ret != HAL_OK)
		  return	(false);
	  tmp = ((int16_t)i2c_rx_buff[0] << 3) | (i2c_rx_buff[1] >> 5);	//	read two temperature bytes
	  if(tmp > 0x3ff)	{	//	temperature is below 0
		  tmp |= 0xf800;	//	sign extend
	  }
	  temperature = ((float) tmp) / 8.0;	//
	return	(true);
}
#endif



