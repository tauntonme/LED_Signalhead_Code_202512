/*
 * Project.hpp
 *
 *  Created on: Jan 25, 2024
 *      Author: Jon Freeman  B. Eng. Hons
 */
#ifndef INC_PROJECT_COMPONENTS_HPP_
#define INC_PROJECT_COMPONENTS_HPP_

#define	PROJECT_IS_CAN_LAMP
//#define	PROJECT_IS_SIGNALS_CONTROLLER

//#define	NODE_IS_CONTROLLER

#ifndef	NODE_IS_CONTROLLER
	#define	NODE_IS_PERIPHERAL
//	#define	NODE_IS_LED_SIGNAL
//	#define	NODE_IS_SEMA_SIGNAL
//	#define	NODE_IS_ONE_THING
//	#define	NODE_IS_OR_ANOTHER
#endif

//#define	USING_ANALOG
//#define	USING_ADC
//#define	USING_DAC
//#define	USING_RTC		//	Real Time Clock
#define	USING_USART1
//#define	USING_LM75		//	Temperature sensor
#define	USING_CAN		//	CAN bus

#endif


