/*
 * CmdLine.cpp
 *
 *  Created on: Jun 30, 2023
 *      Author: Jon Freeman B Eng Hons
 *
 *KNOWN ISSUE	- Treats negative user input values as positive
 */

#include	<cctype>	//	isdigit
#include	<cstring>	//	strlen	strncmp
#include	<cstdlib>	//	strtof
//#include	<cstdio>

#include	"CmdLine.hpp"


bool	CommandLineHandler::CommandExec(const char * inbuff)	{
	uint32_t	list_pos = 0;
	uint32_t 	cmd_wrd_len;
	par.command_line = inbuff;	//	copy for use by any functions called
	const char * pEnd;
	bool	got_numerics;
	bool	found_cmd = false;
	for	(int j = 0; j < MAX_CLI_PARAMS; j++)	//	zero all float parameter variables
		par.flt[j] = 0.0;

	while	(!found_cmd && pcmdlist[list_pos].cmd_word)	{
		cmd_wrd_len = strlen(pcmdlist[list_pos].cmd_word);
		if	((strncmp(inbuff, pcmdlist[list_pos].cmd_word, cmd_wrd_len) == 0) && (!isalpha(inbuff[cmd_wrd_len])))	{	//	Don't find 'fre' in 'fred'
			found_cmd = true;
			par.numof_floats = 0;
			pEnd = inbuff + cmd_wrd_len - 1;
			got_numerics = false;
			while	(!got_numerics && *(++pEnd))//	Test for digits present in command line
				got_numerics = isdigit(*pEnd);	//	if digits found, have 1 or more numeric parameters to read

			if	(got_numerics)	{
				while   (*pEnd && (par.numof_floats < MAX_CLI_PARAMS))  {          //  Assemble all numerics as doubles
					par.flt[par.numof_floats++] = strtof    ((const char *)pEnd, (char **)&pEnd);
					while   (*pEnd && !isdigit(*pEnd) && ('.' != *pEnd) && ('-' != *pEnd) && ('+' != *pEnd))  {   //  Can
						pEnd++;
					}   //
					if  (((*pEnd == '-') || (*pEnd == '+')) && (!isdigit(*(pEnd+1))) && ('.' !=*(pEnd+1)))
						pEnd = inbuff + strlen(inbuff);   //  fixed by aborting remainder of line
				}		//	Endof while	(*pEnd ...
			}			//	Endof if	(got_numerics)

			par.function_returned = pcmdlist[list_pos].func(par);	//	Execute code for this command

		}
		else	//	Command word not matched at list position 'cnt'
			list_pos++;
	}			//	End of while	(!found_cmd ...
	return	(true);
}



