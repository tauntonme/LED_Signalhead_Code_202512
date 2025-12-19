/*
 * CmdLine.hpp
 *
 *  Created on: Jun 30, 2023
 *      Author: Jon Freeman B Eng Hons
 *
 *      Handles commands received from serial ports.
 *
 *      Menu of commands is written as an array of 'struct cli_menu_entry_set' (see below)
 *      Essential entry example : -
 *
 *  struct  cli_menu_entry_set       const My_Menu_list[] = {
 *
 *      {"fred", "sound alarm to wake fred", pointer_to_my_fred_function},
 *		...
 *		{nullptr},	//	final entry nullptr is end of array delimiter
 *	}	;
 *
 *      The command is "fred",
 *      a brief text description of what fred does follows, "sound alarm to wake fred",
 *      the third entry is the function written to carry out this action, e.g. 'MyFredFunction'
 *
 *      Write your function elsewhere in this file, the format must conform to : -
 *
 *      bool	MyFredFunction	(struct parameters & a)	{
 *      	code ...
 *      	return	(true or false);
 *      }
 *
 *      A number of numeric parameters can be extracted from the input command line.
 *      "fred 1 2 3.142 -93.25" will result in setting
 *      a.flt[0] = 1.0
 *      a.flt[1] = 2.0
 *      a.flt[2] = 3.142
 *      a.flt[3] = -93.25
 *			etc.
 *      All available to your function code
 */

#ifndef INC_CMDLINE_HPP_
#define INC_CMDLINE_HPP_

#include 	"main.h"
#include	"parameters.hpp"


class	CommandLineHandler	{
//	  uint32_t	state;
	  struct parameters par;
	  struct cli_menu_entry_set const * pcmdlist;
  public:
	  CommandLineHandler	(struct cli_menu_entry_set const * pcml, Serial * prt)	{	//	Constructor
		  par.command_list = pcml;
		  pcmdlist = pcml;
		  par.com = prt;
	  }	;
	  bool	CommandExec	(const char * t)	;
}	;





#endif /* INC_CMDLINE_HPP_ */
