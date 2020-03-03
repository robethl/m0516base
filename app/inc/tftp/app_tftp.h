 /******************************************************************************
 * @file     app_tftp.h
 * @brief    This is the header file of app_tftp.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		16/01/06
*****************************************************************************/
#ifndef _APP_TFTP_H_
#define _APP_TFTP_H_

#include "typedef.h"

#define TFTP_BLOCK_SIZE		(512)

#define TFTP_SUCCESS	(0)
#define TFTP_TRANSMITTING		(1)
#define TFTP_DATA_INCOMPLETE	(2)

#define TFTP_DEBUG		(1)		/*CPLD flash模块打印开关*/
#define TFTP_DEBUG_HEARSTR	"Tftp:"

#endif

