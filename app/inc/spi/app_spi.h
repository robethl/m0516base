/******************************************************************************
 * @file     app_spi.h
 * @brief    This is the header file of app_spi.c
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12
 *	   v1.1		  huanglv		16/01/18	add analogic spi interface
*****************************************************************************/
#ifndef _APP_SPI_H_
#define _APP_SPI_H_

#include "typedef.h"

#define HDSPI_SUPPORT	(0)
#define SWSPI_SUPPORT	(0)

#pragma pack(push)
#pragma pack(1)

#if SWSPI_SUPPORT
typedef struct
{
	volatile U32 *cs;
	volatile U32 *clk;
	volatile U32 *miso;
	volatile U32 *mosi;
	U32 byteInterval;//unit: us
	U32 bitInterval;	//unit: us 理论支持最高1Mbps
}SwSpi_t, *pSwSpi_t;

typedef enum
{
	SWSPI_0 = 0,
	SWSPI_1,
	SWSPI_NUM,
}SwSpiPort_e;
#endif 

#pragma pack(pop)

#endif

