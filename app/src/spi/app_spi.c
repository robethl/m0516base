/******************************************************************************
 * @file     app_spi.c
 * @brief   spi interface source file
 * @history
 *	=version=	=author=		=date= 
 *	   v1.0		  huanglv		14/03/12	create
 *	   v1.1		  huanglv		16/01/18	add analogic spi interface
*****************************************************************************/
#include <stdio.h>
#include <string.h>
#include "typedef.h"
#include "M051Series.h"
#include "device.h"
#include "app_spi.h"

/***********************************变量定义区*******************************************************/
static Spi_t gSpi;

#if SWSPI_SUPPORT
static SwSpi_t gSwSpi[SWSPI_NUM] = 
{
	{&P40, &P22, &P24, &P23, 5, 1},	/*1#*/
	{&P42, &P11, &P13, &P12, 5, 1}	/*2#*/
};
#endif

/***********************************接口定义区*******************************************************/
#if SWSPI_SUPPORT
static VOID SwSpiByteInterval(U08 spi)
{
	gpDev->time->delaynUs(gSwSpi[spi].byteInterval);
}

static VOID SwSpiBitInterval(U08 spi)
{
	gpDev->time->delaynUs(gSwSpi[spi].bitInterval);
}

static VOID SwSpiEnableCs(U08 spi)
{
	*(gSwSpi[spi].cs) = 0;
}

/*
* brief: SPI1 片选SPI0 片选禁止
*/
static VOID SwSpiDisableCs(U08 spi)
{
	*(gSwSpi[spi].cs) = 1;
}

/*
* brief: SPI 写
*/
static VOID SwSpiWrite(U08 spi, const U08* pBytes, U16 len)
{
	U08	 bit = 0, byte = 0;	
	
	while(len--){
		byte = *pBytes++;
		for(bit = 0; bit < 8; ++bit) {
			if(byte & 0x80) {
				*(gSwSpi[spi].mosi) = 1;
			} else {
				*(gSwSpi[spi].mosi) = 0;
			}
			SwSpiBitInterval(spi);
			*(gSwSpi[spi].clk) = 1;
			SwSpiBitInterval(spi);
			*(gSwSpi[spi].clk) = 0;
			byte <<= 1;
		}
	}
	return;
}

/*
* brief: SPI 读
*/
static VOID SwSpiRead(U08 spi, pU08 pBytes, U16 len)
{
	U08	 bit = 0, byte = 0;	
	
	while(len--){
		byte = 0;	
		for(bit = 0; bit < 8; ++bit){
			byte <<= 1;
			SwSpiBitInterval(spi);
			*(gSwSpi[spi].clk) = 1;
			SwSpiBitInterval(spi);
			if(*(gSwSpi[spi].miso)){
				byte |= 1;
			}
			*(gSwSpi[spi].clk) = 0;
		}
		*pBytes = byte;	
		++pBytes;
	}
	return;
}

static VOID SwSpiSetEnvironment(VOID)
{
	U08 i = SWSPI_0;
	
	//analog spi, only define the MISO pin, refrence [ gSwSpi ] defined in app_spi.c
	GPIO_SetMode(P4, BIT0, GPIO_PMD_OUTPUT);//spi1 cs
	GPIO_SetMode(P2, BIT2, GPIO_PMD_OUTPUT);//spi1 clk
	GPIO_SetMode(P2, BIT4, GPIO_PMD_INPUT); //spi1 miso
	GPIO_SetMode(P2, BIT3, GPIO_PMD_OUTPUT);//spi1 mosi
	
	GPIO_SetMode(P4, BIT2, GPIO_PMD_OUTPUT);//spi2 cs
	GPIO_SetMode(P1, BIT1, GPIO_PMD_OUTPUT);//spi2 clk
	GPIO_SetMode(P1, BIT3, GPIO_PMD_INPUT); //spi2 miso
	GPIO_SetMode(P1, BIT2, GPIO_PMD_OUTPUT);//spi2 mosi
	
	for(; i <  SWSPI_NUM; ++i){
		*(gSwSpi[i].cs) = 1;
		*(gSwSpi[i].clk) = 0;
	}
}
#endif

#if HDSPI_SUPPORT
static VOID SpiEnableCs(VOID *spi)
{
	SPI_SET_SS_LOW((SPI_T*)spi);
}

/*
* brief: SPI1 片选SPI0 片选禁止
*/
static VOID SpiDisableCs(VOID *spi)
{
	SPI_SET_SS_HIGH((SPI_T*)spi);
}

/*
* brief: SPI 写
*/
static VOID SpiWrite(VOID *spi, const U08* pBytes, U16 len)
{
	assert_param(pBytes!=NULL);
	
	while(len--){
		SPI_WRITE_TX0((SPI_T*)spi, *pBytes++);
		SPI_TRIGGER((SPI_T*)spi);
		while(SPI_IS_BUSY((SPI_T*)spi));
	}
}

/*
* brief: SPI 读
*/
static VOID SpiRead(VOID *spi, pU08 pBytes, U16 len)
{
	assert_param(pBytes!=NULL);
	
	while(len--){
		SPI_WRITE_TX0((SPI_T*)spi, 0xFF);
		SPI_TRIGGER((SPI_T*)spi);
		while(SPI_IS_BUSY((SPI_T*)spi));
		*pBytes++ = SPI_READ_RX0((SPI_T*)spi);
	}
}
#endif

/***********************************接口定义区*******************************************************/
/*
* brief: SPI 初始化
*/
 VOID InitSpi(VOID)
{
#if HDSPI_SUPPORT
	/*---------------------------------------------------------------------------------------------------------*/
	/* SPI0 : CPLD																	     */
	/* SPI1 : FPGA																					     */	
	/*---------------------------------------------------------------------------------------------------------*/
	/*reset spi*/
	SYS_ResetModule(SPI0_RST);
	SYS_ResetModule(SPI1_RST);
	
	/* Config SPI*/
	SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 4000000);
 	SPI_Open(SPI1, SPI_MASTER, SPI_MODE_0, 8, 16000000);
	
	/* Enable the automatic hardware slave select function. Select the SS0 pin and configure as low-active. */
	/* cs controlled by user */
	//SPI_EnableAutoSS(SPI0, SPI_SS, SPI_SS_ACTIVE_LOW);
	//SPI_EnableAutoSS(SPI1, SPI_SS, SPI_SS_ACTIVE_LOW);
	gSpi.spiEnableCs = SpiEnableCs;
	gSpi.spiDisableCs = SpiDisableCs;
	gSpi.spiWrite = SpiWrite;
	gSpi.spiRead = SpiRead;
#endif

#if SWSPI_SUPPORT
	gSpi.swSpiByteInterval = SwSpiByteInterval;
	gSpi.swSpiEnableCs = SwSpiEnableCs;
	gSpi.swSpiDisableCs = SwSpiDisableCs;
	gSpi.swSpiWrite = SwSpiWrite;
	gSpi.swSpiRead = SwSpiRead;

	SwSpiSetEnvironment();
#endif	
	gpDev->spi = &gSpi;
}

