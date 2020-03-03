 /******************************************************************************
 * @file     app_uart.h
 * @brief    This is the header file of app_uart.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12
*****************************************************************************/
#ifndef _APP_UART_H_
#define _APP_UART_H_

#include "typedef.h"

#define UART0_BAUDRATE							(115200)
#define UART1_BAUDRATE							(115200)

#define UART_BUFLEN		(128)
typedef struct 
{	
	U08 buf[UART_BUFLEN];
	U32 head;
	U32 tail;
}UartBuffer_t, *pUartBuffer_t;

#endif

