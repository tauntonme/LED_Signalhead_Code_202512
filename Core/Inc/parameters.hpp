/*
 * parameters.hpp
 *
 *  Created on: Jan 19, 2024
 *      Author: Jon Freeman B Eng Hons
 */

#include	"main.h"
#ifndef INC_PARAMZ_HPP_
#define INC_PARAMZ_HPP_

#include	"Serial.hpp"

#define	MAX_CLI_PARAMS	20	//	MAX_CLI_PARAMS max number of command line numeric parameters to look for
#define	MAX_2ND_WORD_LEN	22

enum	class	MenuType	{MENU, SETTINGS}	;	//	To ease common menu listing function


struct  parameters  {   //  Used in serial comms with pc and other controller (e.g. touch-screen)
    struct  	cli_menu_entry_set   const * command_list {nullptr};
    uint32_t	position_in_list {0L};
    uint32_t	numof_floats {0L};//, target_unit, numof_menu_items;
    float		flt[MAX_CLI_PARAMS + 1] {0.0};//values read from input line stored here, e.g. "fred 1 2 3.142 -99.5"
    char		second_word[MAX_2ND_WORD_LEN + 2] {0};	//	used in settings
    const char * command_line {nullptr};	//	to make input command line available to functions
    bool		function_returned;	//	New Jan 2024
    Serial * 	com	;
//    bool    	respond;	//	Used in STM motor control, individual board select in multi-board systems
}   ;



struct cli_menu_entry_set  {	//  Commands tabulated as list of these structures as seen in CmdLine.cpp
  const char * cmd_word {nullptr};		//  points to command text e.g. "command_name"
  const char * description {nullptr};		//  very brief explanation or clue as to purpose of function
  bool (*func)(struct parameters &) {nullptr};   //  points to function code for this menu choice
  //	Above three used in CommandLineHandler and 'User Settings'.
  //	Four below used by user settings code, see settings.cpp ffi
  const	int32_t	min {0L};			//	setting 'min' and 'max' used to prevent setting outside limits
  const	int32_t	max {0L};
  const	int32_t	de_fault {0L};		//	used in 'settings', all settings put to factory defaults with 'us defaults' cmd
  const	float	mpy { 1.0 };		//	Multiplier stored to, e.g. convert A-D reading to '32.45 Volts'
  int32_t		value { 0L };
}  ;

#endif /* INC_PARAMZ_HPP_ */


