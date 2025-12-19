/*
 * rtc.cpp		Real Time Clock / Calender
 *
 *  Created on: Jan 19, 2024
 *      Author: Jon Freeman
 */
/*
You must call HAL_RTC_GetDate() after HAL_RTC_GetTime() to unlock the values in the higher-order
calendar shadow registers to ensure consistency between the time and date values. Reading RTC current
time locks the values in calendar shadow registers until Current date is read to ensure consistency
between the time and date values.
(p877, dm00173145-description-of-stm32l4l4-hal-and-lowlayer-drivers-stmicroelectronics-1.pdf)
*/
#include	"Project.hpp"

#ifdef	USING_RTC

#include	"parameters.hpp"	//
#include	"Serial.hpp"
#include	<cstdio>			//	sprintf

extern	Serial pc;			//	Serial Port
extern	RTC_HandleTypeDef hrtc;	//	RTC Real Time Clock

#define	COM_PORT	pc

RTC_DateTypeDef	rtc_date;
RTC_TimeTypeDef	rtc_time;

/**		char * get_date	(char * dest)	{
 * Reads controller RTC Real Time Clock Calender,
 * and assembles text string, e.g. "Mon 01 Feb 2024" in 'dest' character string.
 *
 * Returns 'dest' for easiest use in sprintf, strcat, pc.write etc.
 */
char * get_date	(char * dest)	{	//	get text string in 'dest', e.g. "Mon 01 Feb 2000"
	const char * months = "NAM\0Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec\0";
	const char * days 	= "NAD\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat\0Sun\0";
	HAL_RTC_GetDate	(&hrtc, & rtc_date, RTC_FORMAT_BIN);
	uint8_t	mon	=	rtc_date.Month;
	uint8_t	day	=	rtc_date.WeekDay;
	if	((mon < 1) || (mon > 12))
		mon = 0;
	if	((day < 1) || (day > 7))
		day = 0;
	sprintf	(dest, "%s %02d %s 20%02d", days + (day << 2), rtc_date.Date, months + (mon << 2), rtc_date.Year);
	return	(dest);
}


/**		int32_t get_date	()	{
 * Reads controller RTC Real Time Clock Calender,
 * and packs three bytes for Year, Month, Date into 'packed_date'.
 *
 * Returns 'int32_t packed_date'.
 */
int32_t	get_date	()	{	//	return binary. From Hi to Lo bytes - 0x00, Year, Month, Date
	int32_t	packed_date = 0L;
	if	(HAL_OK != HAL_RTC_GetDate	(&hrtc, &rtc_date, RTC_FORMAT_BIN))	//
		pc.write	("Error reading RTC date\r\n", 19);						//
	packed_date	 = ((int32_t)rtc_date.Year << 16);	//	Year binary in byte 2
	packed_date	|= ((int32_t)rtc_date.Month << 8);	//	Month binary in byte 1
	packed_date	|= ((int32_t)rtc_date.Date);			//	Date binary in byte 0
	return	(packed_date);
}


/**		char * get_time	(char * dest)	{
 * Reads controller RTC Real Time Clock Calender,
 * and assembles text string for time, e.g. "16:50:19" in 'dest' character string.
 *
 * Returns 'dest' for easiest use in sprintf, strcat, pc.write etc.
 */
char * get_time	(char * dest)	{	//	get e.g. "16:50:11"
	if	(HAL_OK != HAL_RTC_GetTime	(&hrtc, &rtc_time, RTC_FORMAT_BIN))	//
		pc.write	("Error reading RTC time\r\n", 19);						//
//	HAL_RTC_GetTime	(&hrtc, &rtc_time, RTC_FORMAT_BIN);
	sprintf	(dest, "%02d:%02d:%02d", rtc_time.Hours, rtc_time.Minutes, rtc_time.Seconds);
	return	(dest);
}


/**		int32_t get_time	()	{
 * Reads controller RTC Real Time Clock Calender,
 * and packs three bytes for Hours, Minutes, Seconds into 'packed_time'.
 *
 * Returns 'int32_t packed_time'.
 */
int32_t	get_time	()	{	//	return binary. From Hi to Lo bytes - 0x00, Hour, Minutes, Seconds
	int32_t	packed_time = 0L;
	if	(HAL_OK != HAL_RTC_GetTime	(&hrtc, &rtc_time, RTC_FORMAT_BIN))
		pc.write	("Error reading RTC\r\n", 19);						//
	packed_time	 = rtc_time.Hours << 16;	//	Hours binary in byte 2
	packed_time	|= rtc_time.Minutes << 8;	//	Minutes binary in byte 1
	packed_time	|= rtc_time.Seconds;		//	Seconds binary in byte 0
	return	(packed_time);
}


/**	set_time and set_date are reached using the command line interpreter.
 * These write to the controller RTC, assuming your user input passes the tests below
 *
 */
bool	set_time	(struct parameters & a)	{	//	returns false with bad input data, true on success
	uint8_t	time[4];	//	Choice of using H:M, or H:M:S
	int	errors = 0;
	int len;
	char	t[100];
	if	((a.numof_floats < 2) || (a.numof_floats > 3))	{
		len = sprintf	(t, "To set Clock Time, include 2 or 3 numbers for Hour 0-23, Mins 0-59, optional Sec 0-59\r\n");
		COM_PORT.write	(t, len);
		return	(false);
	}
	for	(int i = 0; i < 3; i++)
		time[i] = (uint8_t)a.flt[i];	//	two or three parameters found, convert to integer
	if	((time[0] < 0) || (time[0] > 23))	{
		len = sprintf	(t, "Time set error - Hour %d not in range 0 - 23\r\n", time[0]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	((time[1] < 0) || (time[1] > 59))	{
		len = sprintf	(t, "Time set error - Minute %d not in range 0 - 59\r\n", time[1]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	((time[2] < 0) || (time[2] > 59))	{
		len = sprintf	(t, "Time set error - Seconds %d not in range 0 - 59\r\n", time[2]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	(errors)
		return	(false);	//	can set time if we pass here
	rtc_time.Hours 		= time[0];
	rtc_time.Minutes 	= time[1];
	rtc_time.Seconds 	= time[2];
	HAL_RTC_SetTime	(&hrtc, &rtc_time, RTC_FORMAT_BIN);	//	Can also chose _BCD
	return	(true);
}


bool	set_date	(struct parameters & a)	{	//	returns false with bad input data, true on success
	//	Params binary Year 00-99, Month 1-12, Date 01-31, WeekDay 0-6
	uint8_t	date[4];
	int	errors = 0;
	int len;
	char	t[100];
	if	(a.numof_floats != 4)	{
		COM_PORT.write	("To set date, include 4 numbers for Year 0-99, Month 1-12, Day 1-7, Date1-31\r\n", 77);
		return	(false);
	}
	for	(int i = 0; i < 4; i++)
		date[i] = (uint8_t)a.flt[i];	//	four parameters found, convert to integer
	if	((date[0] < 0) || (date[0] > 99))	{
		len = sprintf	(t, "Date set error - Year %d not in range 0 - 99\r\n", date[0]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	((date[1] < 1) || (date[1] > 12))	{
		len = sprintf	(t, "Date set error - Month %d not in range 1 - 12\r\n", date[1]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	((date[2] < 0) || (date[2] > 99))	{
		len = sprintf	(t, "Date set error - Day %d not in range 1 - 7\r\n", date[2]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	((date[3] < 0) || (date[3] > 99))	{
		len = sprintf	(t, "Date set error - Date %d not in range 1 - 31\r\n", date[3]);
		COM_PORT.write(t, len);
		errors++;
	}
	if	(errors)
		return	(false);	//	can set date if we pass here
	rtc_date.Year	=	date[0];
	rtc_date.Month	=	date[1];
	rtc_date.WeekDay=	date[2];
	rtc_date.Date	=	date[3];
	HAL_RTC_SetDate	(&hrtc, &rtc_date, RTC_FORMAT_BIN);	//	Can also chose _BCD
	return	(true);
}

//extern	uint32_t	millisecs;
void	rtc_buggery	()	{	//	Test all 4 clock read functions
	char	t[100];
	char	datestr[20];
	char	timestr[20];
	int len = sprintf	(t, "%s  %s, d%06lx, t%06lx\r\n", get_date(datestr), get_time(timestr), get_date(), get_time());
	COM_PORT.write	(t, len);
//	len = sprintf(t, "millisecs %ld\r\n", millisecs);
//	pc.write(t, len);
}


#endif

